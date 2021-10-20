

#include <base/base.h>
#include <ke/ke.h>
#include <mm/mm.h>
#include <mm/pool.h>
#include <init/bootgfx.h>
#include <init/preinit.h>
#include <hal/acpi.h>
#include <hal/ioapic.h>
#include <hal/apic.h>
#include <hal/halinit.h>
#include <hal/apstub16.h>


extern U8 HalLegacyIrqToGSIMappings[16];
extern U8 HalGSIToLegacyIrqMappings[256];

VIRTUAL_ADDRESS HalLowArea1MSpace;
AP_INIT_PACKET volatile *HalAPInitPacket;

VIRTUAL_ADDRESS HalApicBase;

typedef
VOID
(KERNELAPI *PKPROCESSOR_START_ROUTINE)(
    VOID);

VOID
KERNELAPI
HalApplicationProcessorStart(
    VOID)
{
    KiInitializeProcessor();

    // Acknowledge to BSP that processor is successfully started
    HalAPInitPacket->Status = 1;
    _mm_mfence();

    for(;;)
    {
        __halt();
    }
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

    while (HalAPInitPacket->Status == 0)
        _mm_pause();
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
        BGXTRACE("APIC_ID %hhu, AcpiProcessorId %hhu, Flags 0x%08x ", 
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

            BGXTRACE("Starting processor (APIC_ID %hhu) ... ", Apic->ApicId);

            HalStartProcessor(
                Apic->ApicId, 
                (U32)(__readcr3() & ~PAGE_MASK), 
                HalApplicationProcessorStart, 
                Addresses.Addresses.StartingVirtualAddress, 
                KERNEL_STACK_SIZE_DEFAULT);

            BGXTRACE("OK\n");
        }

        Apic = HalAcpiGetNextProcessor(Madt, Apic);
    }

}

VOID
KERNELAPI
HalPrepareAPStart(
    VOID)
{
    PTR ApicVirtualBase = 0;
    ESTATUS Status = MmAllocateVirtualMemory(NULL, &ApicVirtualBase, PAGE_SIZE, VadInUse);
    if (!E_IS_SUCCESS(Status))
    {
        FATAL("Failed to allocate memory for APIC\n");
    }

    PHYSICAL_ADDRESS LocalApicBase = (PHYSICAL_ADDRESS)(U32)HalAcpiMadt->LocalApicBase;

    Status = MmMapSinglePage(LocalApicBase, ApicVirtualBase, ARCH_X64_PXE_WRITABLE | ARCH_X64_PXE_CACHE_DISABLED);
    if (!E_IS_SUCCESS(Status))
    {
        FATAL("Failed to map memory for APIC\n");
    }

    //
    // Relocate 16-bit stub to low memory area.
    //

    PTR LowMemoryVirtual = 0;
    Status = MmAllocateVirtualMemory(NULL, &LowMemoryVirtual, 0x100000, VadInUse);
    if (!E_IS_SUCCESS(Status))
    {
        FATAL("Failed to allocate/map lower 1M area");
    }

    PHYSICAL_ADDRESSES PhysicalAddresses = 
    {
        .Mapped = FALSE,
        .PhysicalAddressCount = 1,
        .PhysicalAddresses[0].Range.Start = 0,
        .PhysicalAddresses[0].Range.End = 0x100000 - 1,
        .PhysicalAddressMaximumCount = 1,
    };

    Status = MmMapPages(&PhysicalAddresses, LowMemoryVirtual, ARCH_X64_PXE_WRITABLE, TRUE, 0);
    if (!E_IS_SUCCESS(Status))
    {
        FATAL("Failed to allocate/map lower 1M area");
    }

    AP_INIT_PACKET *Packet = (AP_INIT_PACKET *)(U64)&HalpApplicationProcessorInitStub;
    AP_INIT_PACKET *RelocatedPacket = (AP_INIT_PACKET *)(LowMemoryVirtual + HAL_PROCESSOR_RESET_ADDRESS);

    if (memcmp(Packet->Signature, "AP16INIT", 8))
    {
        FATAL("Corrupted AP16 stub");
    }

    memcpy(RelocatedPacket, Packet, Packet->PacketSize);

    BGXTRACE("Relocated AP init packet 0x%016llx\n", RelocatedPacket);

    HalLowArea1MSpace = LowMemoryVirtual;
    HalAPInitPacket = RelocatedPacket;
    HalApicBase = ApicVirtualBase;
}

VOID
KERNELAPI
HalPreInitialize(
    IN PVOID Rsdp)
{
    HalAcpiPreInitialize(Rsdp);
    HalPrepareAPStart();
}

VOID
KERNELAPI
HalInitialize(
    VOID)
{
    HalStartProcessors();
}

