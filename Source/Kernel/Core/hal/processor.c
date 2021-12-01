
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
#include <hal/ptimer.h>
#include <hal/processor.h>


U32 HalMeasuredApicInitialCounter;

/**
 * @brief 64-bit entry of AP start code.
 * 
 * @return None.
 */
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

/**
 * @brief ISR for spurious interrupt.
 * 
 * @param [in] Interrupt            Interruot object.
 * @param [in] InterruptContext     Interrupt context.
 * @param [in] InterruptStackFrame  Interrupt stack frame.
 * 
 * @return Always InterruptAccepted.
 */
KINTERRUPT_RESULT
KERNELAPI
HalIsrSpurious(
    IN PKINTERRUPT Interrupt,
    IN PVOID InterruptContext,
    IN PVOID InterruptStackFrame)
{
    // Spurious interrupt does not require EOI
    return InterruptAccepted;
}

/**
 * @brief ISR for local APIC timer interrupt.
 * 
 * @param [in] Interrupt            Interruot object.
 * @param [in] InterruptContext     Interrupt context.
 * @param [in] InterruptStackFrame  Interrupt stack frame.
 * 
 * @return Always InterruptAccepted.
 */
KINTERRUPT_RESULT
KERNELAPI
HalApicIsrTimer(
    IN PKINTERRUPT Interrupt,
    IN PVOID InterruptContext,
    IN PVOID InterruptStackFrame)
{
    // @todo: do context switch.
    HalGetPrivateData()->ApicTickCount++;
    HalApicSendEoi(HalApicBase);
    return InterruptAccepted;
}

/**
 * @brief Starts the processor.\n
 *        This function sets fields in AP packet, requests IPI and waits for initialization.
 * 
 * @param [in] ApicId               APIC ID to target processor.
 * @param [in] PML4TPhysicalBase    Physical address of PML4T.
 * @param [in] StartRoutine         Processor start routine.
 * @param [in] StackBase            Initial stack.
 * @param [in] StackSize            Initial stack size.
 * 
 * @return None.
 */
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
        BGXTRACE_DBG("\rWaiting AP (%hhd)", HalAPInitPacket->Status);
        _mm_pause();
    }
    while (HalAPInitPacket->Status < 4);
}

/**
 * @brief Starts all processors reported by ACPI.
 * 
 * @return None.
 */
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

/**
 * @brief Returns private data pointer.
 * 
 * @return HAL_PRIVATE_DATA* 
 */
HAL_PRIVATE_DATA *
KERNELAPI
HalGetPrivateData(
    VOID)
{
    return (HAL_PRIVATE_DATA *)KeGetCurrentProcessor()->HalPrivateData;
}

/**
 * @brief Initializes the private data for HAL.
 * 
 * @return None.
 */
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

    memset(PrivateData, 0, sizeof(*PrivateData));
    PrivateData->ApicTickCount = 0;

    Processor->HalPrivateData = PrivateData;
}

/**
 * @brief Registers interrupt with given vector.
 * 
 * @param [in,out] Interrupt        Interrupt object to be initialized.
 * @param [in] InterruptRoutine     ISR for given interrupt.
 * @param [in] InterruptContext     Interrupt context for given interrupt.
 * @param [in] Vector               Vector.
 * 
 * @return ESTATUS code.
 */
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

/**
 * @brief Registers interrupt for local APIC.
 * 
 * @return None.
 */
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

/**
 * @brief Measures local APIC counter for given MeasureUnit (in ms).\n
 * 
 * @param [in] MeasureUnit      Measure unit in miliseconds.
 * 
 * @return Returns APIC counter
 */
U32
KERNELAPI
HalMeasureApicCounter(
    IN U32 MeasureUnit)
{
    DASSERT(HalIsBootstrapProcessor());
    const ULONG MeasureCount = 10;

    volatile U64 Tick = 0;

    U32 InitialCounter = 0;
    U32 AverageCounterDelta = 0;

    // Stop timer.
    HalApicSetTimerVector(HalApicBase, 0, VECTOR_LVT_TIMER);

    DbgTraceF(TraceLevelDebug, "Measuring APIC counter (unit = %dms)\n", MeasureUnit);

    for (ULONG i = 0; i < MeasureCount; i++)
    {
        // Start timer
        HalApicSetTimerVector(HalApicBase, 0xffffffff, VECTOR_LVT_TIMER);

        Tick = HalGetTickCount() + MeasureUnit;

        U32 CurrentCounterStart = 0;
        HalApicReadTimerCounter(HalApicBase, &InitialCounter, &CurrentCounterStart);

        // Wait for tick count to be changed
        while (HalGetTickCount() < Tick)
            _mm_pause();

        U32 CurrentCounterEnd = 0;
        HalApicReadTimerCounter(HalApicBase, &InitialCounter, &CurrentCounterEnd);

        HalApicSetTimerVector(HalApicBase, 0, VECTOR_LVT_TIMER);

        U32 CounterDelta = CurrentCounterStart - CurrentCounterEnd;

        DbgTraceF(TraceLevelDebug, "Measured APIC counter #%d = %d\n", i, CounterDelta);

        AverageCounterDelta += CounterDelta;
    }

    AverageCounterDelta /= MeasureCount;

    DbgTraceF(TraceLevelDebug, "Measured APIC counter (average) = %d\n", AverageCounterDelta);

    return AverageCounterDelta;
}

/**
 * @brief Initializes the local APIC NMI vector.
 * 
 * @return None.
 */
VOID
KERNELAPI
HalSetApicNMIVector(
    VOID)
{
    U8 ApicId = HalApicGetId(HalApicBase);
    ACPI_LOCAL_APIC *Apic = HalAcpiLookupProcessor(HalAcpiMadt, ApicId);
    DASSERT(Apic);

    ACPI_MADT_RECORD_HEADER *Record = HalAcpiGetFirstMadtRecord(HalAcpiMadt, ACPI_MADT_RECORD_APIC_NMI);

    while (Record)
    {
        DASSERT(Record->EntryType == ACPI_MADT_RECORD_APIC_NMI);

        ACPI_LOCAL_APIC_NMI *NMI = (ACPI_LOCAL_APIC_NMI *)Record;
        if (NMI->AcpiProcessorId == Apic->AcpiProcessorId || 
            /* A value of 0xFF signifies that this applies to all processors in the machine. */
            NMI->AcpiProcessorId == 0xff)
        {
            BOOLEAN ActiveLow = (NMI->Flags.Polarity == ACPI_INT_OVERRIDE_POLARITY_ACTIVE_LOW);
            BOOLEAN LevelSensitive = (NMI->Flags.TriggerMode == ACPI_INT_OVERRIDE_TRIG_LEVEL);

            HalApicSetLINTxVector(HalApicBase, NMI->LocalApicLINTn, 
                ActiveLow, LevelSensitive, 4 /*delivery mode = NMI*/, 0);
        }

        Record = HalAcpiGetNextMadtRecord(HalAcpiMadt, Record);
    }
}

/**
 * @brief Initializes the processor.
 * 
 * @return None.
 */
VOID
KERNELAPI
HalInitializeProcessor(
    VOID)
{
    HalInitializePrivateData();

    HalRegisterApicInterrupt();
    HalApicSetDefaultState(HalApicBase);
    HalApicEnable();
    HalApicSetSpuriousVector(HalApicBase, TRUE, VECTOR_SPURIOUS);
    HalApicSetTimerVector(HalApicBase, 0, VECTOR_LVT_TIMER);
    //HalApicSetErrorVector(HalApicBase, TRUE, VECTOR_LVT_ERROR);
    HalSetApicNMIVector();

    if (HalIsBootstrapProcessor())
    {
        //
        // Do additional initializations for BSP.
        //

        ESTATUS Status = HalInitializePlatformTimer();
        if (!E_IS_SUCCESS(Status))
        {
            FATAL("Failed to initialize system timer");
        }

        _enable();

        //
        // Measure APIC counter before setup.
        //

        HalMeasuredApicInitialCounter = HalMeasureApicCounter(100) / 10 * 2; /* 1 context switch per 20ms */
        if (!HalMeasuredApicInitialCounter)
        {
            FATAL("Strange APIC counter");
        }
    }

    //
    // Finally, setup APIC timer.
    //

    _enable();

    HalApicSetTimerVector(HalApicBase, HalMeasuredApicInitialCounter, VECTOR_LVT_TIMER);
}

