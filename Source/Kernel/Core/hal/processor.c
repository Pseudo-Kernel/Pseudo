
/**
 * @file processor.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements processor related routines.
 * @version 0.1
 * @date 2021-10-24
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <base/base.h>
#include <ke/ke.h>
#include <ke/kprocessor.h>
#include <ke/interrupt.h>
#include <mm/mm.h>
#include <mm/pool.h>
#include <init/bootgfx.h>
#include <init/preinit.h>
#include <hal/acpi.h>
#include <hal/ioapic.h>
#include <hal/apic.h>
#include <hal/halinit.h>
#include <hal/processor.h>


extern U8 HalLegacyIrqToGSIMappings[16];
extern U8 HalGSIToLegacyIrqMappings[256];


VOID
KERNELAPI
HalApplicationProcessorStart(
    VOID)
{
    KiInitializeProcessor();
    KiInitializeIrqGroups();
    HalInitializeProcessor();

    // Acknowledge to BSP that processor is successfully started
    _InterlockedExchange8((volatile char *)&HalAPInitPacket->Status, 4);

    for(;;)
    {
        __halt();
    }
}

KINTERRUPT_RESULT
KERNELAPI
HalIsrSpurious(
    IN PKINTERRUPT Interrupt,
    IN PVOID InterruptContext)
{
    // Spurious interrupt does not require EOI
    return InterruptAccepted;
}

KINTERRUPT_RESULT
KERNELAPI
HalApicIsrTimer(
    IN PKINTERRUPT Interrupt,
    IN PVOID InterruptContext)
{
    HalApicSendEoi(HalApicBase);
    return InterruptAccepted;
}

VOID
KERNELAPI
HalStartProcessor(
    IN U8 ApicId,
    IN U32 PML4TPhysicalBase,
    IN PKPROCESSOR_START_ROUTINE StartRoutine,
    IN U64 StackBase,
    IN U64 StackSize)
{
    HalAPInitPacket->Status = 0;
    HalAPInitPacket->PML4Base = PML4TPhysicalBase;
    HalAPInitPacket->LM64StartAddress = (U64)StartRoutine;
    HalAPInitPacket->LM64StackAddress = StackBase;
    HalAPInitPacket->LM64StackSize = StackSize;

    HalAPInitPacket->SegmentDescriptors[0] = 0; // null
    HalAPInitPacket->SegmentDescriptors[1] = 0x00af9a000000ffff; // code64
    HalAPInitPacket->SegmentDescriptors[2] = 0x00cf92000000ffff; // data64
    _mm_mfence();

    HalApicStartProcessor(HalApicBase, ApicId, HAL_PROCESSOR_RESET_VECTOR);

    do
    {
        BGXTRACE("\rWaiting AP (%hhd)", HalAPInitPacket->Status);
        _mm_pause();
    }
    while (HalAPInitPacket->Status < 4);
}

VOID
KERNELAPI
HalStartProcessors(
    VOID)
{
    ACPI_MADT *Madt = HalAcpiMadt;

    ACPI_LOCAL_APIC *Apic = HalAcpiGetFirstProcessor(Madt);
    while (Apic)
    {
        BGXTRACE("LocalAPIC: APIC_ID %hhu, AcpiProcessorId %hhu, Flags 0x%08x ", 
            Apic->ApicId, Apic->AcpiProcessorId, Apic->Flags);

        BOOLEAN BSP = FALSE;

        if (Apic->ApicId == HalApicGetId(HalApicBase))
        {
            BGXTRACE("[BSP]");
            BSP = TRUE;
        }

        BGXTRACE("\n");

        if (Apic->ProcessorEnabled && !BSP)
        {
            PHYSICAL_ADDRESSES_R128 Addresses;
            INITIALIZE_PHYSICAL_ADDRESSES(&Addresses, 0, PHYSICAL_ADDRESSES_MAXIMUM_COUNT(sizeof(Addresses)));

            ESTATUS Status = MmAllocateAndMapPagesGather(&Addresses.Addresses, KERNEL_STACK_SIZE_DEFAULT, 
                ARCH_X64_PXE_WRITABLE, PadInUse, VadInUse);
            
            if (!E_IS_SUCCESS(Status))
            {
                FATAL("Failed to allocate/map kernel stack");
            }

            BGXTRACE("Starting processor (APIC_ID %hhu) ...\n", Apic->ApicId);

            HalStartProcessor(
                Apic->ApicId, 
                (U32)(__readcr3() & ~PAGE_MASK), 
                HalApplicationProcessorStart, 
                Addresses.Addresses.StartingVirtualAddress, 
                KERNEL_STACK_SIZE_DEFAULT);

            BGXTRACE(" -> OK\n");
        }

        Apic = HalAcpiGetNextProcessor(Madt, Apic);
    }

}

HAL_PRIVATE_DATA *
KERNELAPI
HalGetPrivateData(
    VOID)
{
    return (HAL_PRIVATE_DATA *)KeGetCurrentProcessor()->HalPrivateData;
}

VOID
KERNELAPI
HalInitializePrivateData(
    VOID)
{
    KPROCESSOR *Processor = KeGetCurrentProcessor();

    HAL_PRIVATE_DATA *PrivateData = MmAllocatePool(PoolTypeNonPaged, sizeof(*PrivateData), 0x10, 0);
    if (!PrivateData)
    {
        FATAL("Failed to allocate memory for HAL");
    }

    Processor->HalPrivateData = PrivateData;
}

ESTATUS
KERNELAPI
HalRegisterInterrupt(
    IN OUT KINTERRUPT *Interrupt,
    IN PKINTERRUPT_ROUTINE InterruptRoutine,
    IN PVOID InterruptContext,
    IN ULONG Vector)
{
    ULONG ResultVector = 0;

    ESTATUS Status = KeAllocateIrqVector(VECTOR_TO_IRQL(Vector), 1, Vector, 
        INTERRUPT_IRQ_HINT_EXACT_MATCH, &ResultVector, TRUE);
    if (!E_IS_SUCCESS(Status))
    {
        return Status;
    }

    DASSERT(Vector == ResultVector);

    Status = KeInitializeInterrupt(Interrupt, InterruptRoutine, InterruptContext, 0);
    if (!E_IS_SUCCESS(Status))
    {
        DASSERT(E_IS_SUCCESS(KeFreeIrqVector(ResultVector, 1, TRUE)));
        return Status;
    }

    Status = KeConnectInterrupt(Interrupt, Vector, 0);
    if (!E_IS_SUCCESS(Status))
    {
        DASSERT(E_IS_SUCCESS(KeFreeIrqVector(ResultVector, 1, TRUE)));
        return Status;
    }

    return Status;
}

VOID
KERNELAPI
HalRegisterApicInterrupt(
    VOID)
{
    HAL_PRIVATE_DATA *PrivateData = HalGetPrivateData();
    ESTATUS Status = E_SUCCESS;

    Status = HalRegisterInterrupt(
        &PrivateData->InterruptObjects.ApicSpurious,
        &HalIsrSpurious,
        NULL, VECTOR_SPURIOUS);

    if (!E_IS_SUCCESS(Status))
    {
        FATAL("Failed to allocate/register IRQ for APIC");
    }

    Status = HalRegisterInterrupt(
        &PrivateData->InterruptObjects.ApicTimer,
        &HalApicIsrTimer,
        NULL, VECTOR_LVT_TIMER);

    if (!E_IS_SUCCESS(Status))
    {
        FATAL("Failed to allocate/register IRQ for APIC");
    }


//    Status = HalRegisterInterrupt(
//        &PrivateData->InterruptObjects.ApicError,
//        &HalIsrSpurious,
//        NULL, VECTOR_LVT_ERROR);
}

VOID
KERNELAPI
HalInitializeProcessor(
    VOID)
{
    HalInitializePrivateData();
    HalRegisterApicInterrupt();
    HalApicSetDefaultState(HalApicBase);
//    HalApicEnable();
    HalApicSetSpuriousVector(HalApicBase, TRUE, VECTOR_SPURIOUS);
    HalApicSetTimerVector(HalApicBase, 0, VECTOR_LVT_TIMER);
    //HalApicSetErrorVector(HalApicBase, TRUE, VECTOR_LVT_ERROR);
}
