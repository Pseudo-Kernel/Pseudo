
/**
 * @file hpet.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements HPET.
 * @version 0.1
 * @date 2021-12-15
 * 
 * @copyright Copyright (c) 2021
 */

#include <base/base.h>
#include <mm/mm.h>
#include <ke/inthandler.h>
#include <ke/interrupt.h>
#include <ke/kprocessor.h>
#include <hal/halinit.h>
#include <hal/apic.h>
#include <hal/processor.h>
#include <hal/ioapic.h>
#include <hal/acpi.h>
#include <hal/hpet.h>

extern volatile U64 HalTickCount;

// 
// HPET driver implementation prerequisites:
// 1. HPET must support 64-bit wide counter (COUNT_SIZE_CAP = 1).
// 2. HPET must support 3 timers at least (NUM_TIM_CAP >= 3).
// 3. HPET must support 1 timer which is periodic capable (Tn_PER_INT_CAP = 1).
// 
// 4. HPET may report strange interrupt routing capability bits (Tn_INT_ROUTE_CAP).
//    e.g.) Reporting IOAPIC IRQ which doesn't exist in IOAPIC.
// 
// 5. HPET may share same IOAPIC IRQ with PIT.
//    e.g.) QEMU reports Tn_INT_ROUTE_CAP = 0x00000004 with interrupt override (ISA IRQ 0 -> IOAPIC IRQ 2),
//          which means HPET interrupt can only be routed to IOAPIC IRQ 2.
//          Since we can't completely disable the PIT, i think using legacy routing
//          mode is the only way to handle interrupt for HPET in this case.
//          (We can pause the PIT counter by writing only 1-byte for 2-byte write count mode, but it is ugly)
// 
// 

typedef struct _HAL_HPET_CONTEXT
{
    VIRTUAL_ADDRESS BaseAddress;
    PHYSICAL_ADDRESS PhysicalAddress;
    U8 HPETNumber;
    U16 MinimumClockTick;
    U32 EventTimerBlockId;
    HPET_CAPABILITIES_AND_ID Capabilities;

    U8 PeriodicTimerNumber;
    U64 TimerPeriod0;

    KINTERRUPT Interrupt;
    BOOLEAN Initialized;
} HAL_HPET_CONTEXT;

HAL_HPET_CONTEXT HalpHpetContext;

/*
U32
HalHpetReadRegister32(
    IN VIRTUAL_ADDRESS Base,
    IN ULONG Register)
{
    DASSERT(0 <= Register && Register < HPET_SIZE_REGISTER_SPACE);
    DASSERT(!(Register & (HPET_ACCESS_ALIGNMENT - 1)));

    volatile U32 *p = (volatile U32 *)(Base + Register);

    return *p;
}
*/

/**
 * @brief Reads the 64-bit HPET register.
 * 
 * @param [in] Base         Base address of HPET.
 * @param [in] Register     Register offset (8-byte aligned).
 * 
 * @return Register value.
 */
U64
HalHpetReadRegister64(
    IN VIRTUAL_ADDRESS Base,
    IN ULONG Register)
{
    DASSERT(0 <= Register && Register < HPET_SIZE_REGISTER_SPACE);
    DASSERT(!(Register & (HPET_ACCESS_ALIGNMENT - 1)));

    volatile U64 *p = (volatile U64 *)(Base + Register);

    return *p;
}

/**
 * @brief Writes to the 64-bit HPET register.
 * 
 * @param [in] Base         Base address of HPET.
 * @param [in] Register     Register offset (8-byte aligned).
 * @param [in] Value        New value.
 * 
 * @return Previous value.
 */
U64
HalHpetWriteRegister64(
    IN VIRTUAL_ADDRESS Base,
    IN ULONG Register,
    IN U64 Value)
{
    DASSERT(0 <= Register && Register < HPET_SIZE_REGISTER_SPACE);
    DASSERT(!(Register & (HPET_ACCESS_ALIGNMENT - 1)));

    volatile U64 *p = (volatile U64 *)(Base + Register);
    U64 PrevValue = *p;
    *p = Value;

    return PrevValue;
}

/**
 * @brief Do read-modify-write operation to the 64-bit HPET register.
 * 
 * @param [in] Base         Base address of HPET.
 * @param [in] Register     Register offset (8-byte aligned).
 * @param [in] Value        New value.
 * @param [in] SetMask      64-bit mask which specifies the bits to be modified.\n
 *                          The resulting value will be written to the register as follows:\n
 *                          Result = (Value AND SetMask) OR (PrevValue AND NOT(SetMask)).\n
 *                          If zero is specified, then operation has no effect because same value will be written.\n
 * 
 * @return Previous value.
 */
U64
HalHpetWriteRegisterByMask64(
    IN VIRTUAL_ADDRESS Base,
    IN ULONG Register,
    IN U64 Value,
    IN U64 SetMask)
{
    DASSERT(0 <= Register && Register < HPET_SIZE_REGISTER_SPACE);
    DASSERT(!(Register & (HPET_ACCESS_ALIGNMENT - 1)));

    volatile U64 *p = (volatile U64 *)(Base + Register);
    U64 PrevValue = *p;
    *p = (Value & SetMask) | (PrevValue & ~SetMask);

    return PrevValue;
}

/**
 * @brief Do the pre-initialization of HPET.
 * 
 * @param [in] HpetContext      HPET context structure.
 * @param [in] Hpet             ACPI HPET table.
 * 
 * @return ESTATUS code.
 */
ESTATUS
HalpHpetPreInitialize(
    IN HAL_HPET_CONTEXT *HpetContext,
    IN ACPI_HPET *Hpet)
{
    PTR VirtualAddress = 0;
    ESTATUS Status = MmAllocateVirtualMemory(NULL, &VirtualAddress, HPET_SIZE_REGISTER_SPACE, VadInUse);

    if (!E_IS_SUCCESS(Status))
    {
        return Status;
    }

    //
    // Map HPET registers to single page as HPET_SIZE_REGISTER_SPACE < PAGE_SIZE.
    //

    DASSERT(HPET_SIZE_REGISTER_SPACE < PAGE_SIZE);
    PHYSICAL_ADDRESS PhysicalAddress = Hpet->BaseAddress.Address;
    Status = MmMapSinglePage(PhysicalAddress, VirtualAddress, ARCH_X64_PXE_WRITABLE | ARCH_X64_PXE_CACHE_DISABLED);

    if (!E_IS_SUCCESS(Status))
    {
        MmFreeVirtualMemory(VirtualAddress, HPET_SIZE_REGISTER_SPACE);
        return Status;
    }

    //
    // Query from HPET registers.
    //

    HPET_CAPABILITIES_AND_ID Capabilities;
    Capabilities.Value = HalHpetReadRegister64(VirtualAddress, HPET_REGISTER_CAPABILITIES_ID);

    DbgTraceF(TraceLevelDebug,
        "COUNT_SIZE_CAP %d, NUM_TIM_CAP %d, COUNTER_CLK_PERIOD 0x%08x, LEG_ROUTE_CAP %d\n", 
        Capabilities.COUNT_SIZE_CAP, Capabilities.NUM_TIM_CAP, 
        Capabilities.COUNTER_CLK_PERIOD, Capabilities.LEG_ROUTE_CAP);

    // Must support 64-bit main counter and 3 timers.
    if (!Capabilities.COUNT_SIZE_CAP || 
        Capabilities.NUM_TIM_CAP + 1 < 3)
    {
        Status = E_NOT_SUPPORTED;
        goto Cleanup;
    }

    U8 PeriodicTimerNumber = ~0;

    for (U8 i = 0; i <= Capabilities.NUM_TIM_CAP; i++)
    {
        HPET_CONFIGURATION_AND_CAPABILITIES Configuration;
        Configuration.Value = HalHpetReadRegister64(VirtualAddress, HPET_REGISTER_TIMER_N_CONF_CAPABILITY(i));

        DbgTraceF(TraceLevelDebug,
            "T[%d]: TN_INT_ROUTE_CAP 0x%08x, TN_PER_INT_CAP %d, TN_SIZE_CAP %d\n", 
            i, Configuration.TN_INT_ROUTE_CAP, Configuration.TN_PER_INT_CAP, Configuration.TN_SIZE_CAP);
        
        if (Configuration.TN_SIZE_CAP && Configuration.TN_PER_INT_CAP)
        {
            if (PeriodicTimerNumber == (U8)~0)
            {
                DbgTraceF(TraceLevelDebug, "T[%d] selected\n", i);
                PeriodicTimerNumber = i;
            }
        }

        if (!(Configuration.TN_INT_ROUTE_CAP & ~0xffff))
        {
            // Only IOAPIC IRQ < 16 is supported for timer[i].
            // Check whether the legacy interrupt routing is supported.
            if (!Capabilities.LEG_ROUTE_CAP)
            {
                DbgTraceF(TraceLevelDebug, 
                    "T[%d] unusable (Legacy routing not supported, Routable IOAPIC IRQ < 16)\n", i);

                Status = E_NOT_SUPPORTED;
                goto Cleanup;
            }
        }
    }

    // Must support one periodic timer at least.
    if (PeriodicTimerNumber == (U8)~0)
    {
        Status = E_NOT_SUPPORTED;
        goto Cleanup;
    }

    HpetContext->PhysicalAddress = PhysicalAddress;
    HpetContext->BaseAddress = VirtualAddress;
    HpetContext->HPETNumber = Hpet->HPETNumber;
    HpetContext->EventTimerBlockId = Hpet->EventTimerBlockId;
    HpetContext->MinimumClockTick = Hpet->MainCounterMinimumTick;
    HpetContext->Capabilities = Capabilities;

    HpetContext->PeriodicTimerNumber = PeriodicTimerNumber;
    HpetContext->Initialized = TRUE;

    return E_SUCCESS;

Cleanup:
    // FIXME: Unmap pages before freeing page
    MmFreeVirtualMemory(VirtualAddress, HPET_SIZE_REGISTER_SPACE);

    return Status;
}

/**
 * @brief ISR for HPET timer.
 * 
 * @param [in] Interrupt            Interrupt object.
 * @param [in] InterruptContext     Interrupt context.
 * @param [in] InterruptStackFrame  Interrupt stack frame.
 * 
 * @return Always InterruptAccepted.
 */
KINTERRUPT_RESULT
KERNELAPI
HalIsrHighPrecisionTimer(
    IN PKINTERRUPT Interrupt,
    IN PVOID InterruptContext,
    IN PVOID InterruptStackFrame)
{
    HAL_HPET_CONTEXT *HpetContext = (HAL_HPET_CONTEXT *)InterruptContext;

    HalApicSendEoi(HalApicBase);

#if 1 // Periodic mode
    U64 MainCounter = HalHpetReadRegister64(HpetContext->BaseAddress, HPET_REGISTER_MAIN_COUNTER);

#else
    // Clear ENABLE_CNF.
    HalHpetWriteRegisterByMask64(HpetContext->BaseAddress, HPET_REGISTER_CONFIGURATION,
        0, HPET_GENERAL_CONFIGURATION_ENABLE_CNF);

    U64 MainCounter = HalHpetReadRegister64(HpetContext->BaseAddress, HPET_REGISTER_MAIN_COUNTER);
 
    U64 Comparator = HalHpetReadRegister64(HpetContext->BaseAddress, HPET_REGISTER_TIMER_N_COMPARATOR(0));
    HalHpetWriteRegister64(HpetContext->BaseAddress, HPET_REGISTER_TIMER_N_COMPARATOR(0), Comparator + HpetContext->TimerPeriod0);

    // Set ENABLE_CNF.
    HalHpetWriteRegisterByMask64(HpetContext->BaseAddress, HPET_REGISTER_CONFIGURATION,
        HPET_GENERAL_CONFIGURATION_ENABLE_CNF, HPET_GENERAL_CONFIGURATION_ENABLE_CNF);
#endif

//    DbgTraceF(TraceLevelDebug, "HPET_ISR: MainCounter 0x%016llx\n", MainCounter);

//    HalTickCount = MainCounter / (0xe8d4a51000ULL / HpetContext->Capabilities.COUNTER_CLK_PERIOD);
    _InterlockedIncrement64(&HalTickCount);

    return InterruptAccepted;
}

/**
 * @brief Registers the interrupt and enables the HPET timer.
 * 
 * @param [in] HpetContext      HPET context structure.
 * 
 * @return ESTATUS code.
 */
ESTATUS
HalpHpetEnable(
    IN HAL_HPET_CONTEXT *HpetContext)
{
    const ULONG TimerNumber = HpetContext->PeriodicTimerNumber;

    //
    // Allocate IRQ vector and register interrupt.
    //

    U32 Vector = VECTOR_PLATFORM_TIMER;
    ESTATUS Status = HalRegisterInterrupt(&HpetContext->Interrupt, &HalIsrHighPrecisionTimer, 
        HpetContext, VECTOR_TO_IRQL(Vector), Vector, NULL);

    if (!E_IS_SUCCESS(Status))
    {
        return Status;
    }

    BOOLEAN LegacyRouting = FALSE;

    //
    // Check legacy routing capability if needed.
    //

    HPET_CONFIGURATION_AND_CAPABILITIES Configuration;
    ULONG ConfigurationRegister = HPET_REGISTER_TIMER_N_CONF_CAPABILITY(TimerNumber);
    Configuration.Value = HalHpetReadRegister64(HpetContext->BaseAddress, ConfigurationRegister);

    DbgTraceF(TraceLevelDebug, "TN_INT_ROUTE_CAP 0x%08x\n", Configuration.TN_INT_ROUTE_CAP);

    if (TimerNumber < 2)
    {
        // Legacy routing affects for timer 0, 1
        if (!(Configuration.TN_INT_ROUTE_CAP & ~0xffff))
        {
            DASSERT(HpetContext->Capabilities.LEG_ROUTE_CAP);

            //
            // Do legacy routing.
            //

            DbgTraceF(TraceLevelDebug, "Use legacy replacement routing for HPET timer %d\n", TimerNumber);
            LegacyRouting = TRUE;
        }
    }

    // Clear ENABLE_CNF.
    HalHpetWriteRegisterByMask64(HpetContext->BaseAddress, HPET_REGISTER_CONFIGURATION,
        LegacyRouting ? HPET_GENERAL_CONFIGURATION_LEG_RT_CNF : 0,
        HPET_GENERAL_CONFIGURATION_VALID_MASK);

    if (LegacyRouting)
    {
        U32 GSI = 0;

        if (TimerNumber == 0)
            GSI = 2; // Timer0 -> IOAPIC IRQ 2
        else if (TimerNumber == 1)
            GSI = 8; // Timer1 -> IOAPIC IRQ 8
        else
            DASSERT(FALSE);

        Status = HalSetInterruptRedirection(GSI, Vector, KeGetCurrentProcessorId(), 
            /* edge-triggered, high active */
            INTERRUPT_REDIRECTION_FLAG_SET_TRIGGER_MODE | INTERRUPT_REDIRECTION_FLAG_SET_POLARITY);
    }
    else
    {
        //
        // Find and setup the interrupt redirection.
        // Route the HPET interrupt.
        //

        for (U32 i = 16; i < 32; i++)
        {
            if (!(Configuration.TN_INT_ROUTE_CAP & (1 << i)))
            {
                continue;
            }

            U32 GSI = 0;
            Status = HalAllocateInterruptRedirection(&GSI, 1, i, i + 1);
            if (!E_IS_SUCCESS(Status))
            {
                continue;
            }

            DASSERT(0 <= GSI && GSI < 32);
            Configuration.TN_INT_ROUTE_CNF = GSI;   // Can be 0..31
            HalHpetWriteRegister64(HpetContext->BaseAddress, ConfigurationRegister, Configuration.Value);

            //
            // After write, We must prove TN_INT_ROUTE_CNF by reading back once.
            //
            // Tn_INT_ROUTE_CNF
            // If the value is not supported by this prarticular timer, then the value
            // read back will not match what is written. 
            // [From: IA-PC HPET Specification 1.0a]
            //

            Configuration.Value = HalHpetReadRegister64(HpetContext->BaseAddress, ConfigurationRegister);

            if (Configuration.TN_INT_ROUTE_CNF == GSI)
            {
                // Write was successful.
                Status = HalSetInterruptRedirection(i, Vector, KeGetCurrentProcessorId(), 
                    /* edge-triggered, high active */
                    INTERRUPT_REDIRECTION_FLAG_SET_TRIGGER_MODE | INTERRUPT_REDIRECTION_FLAG_SET_POLARITY);
            }
            else
            {
                Status = E_NOT_ENOUGH_RESOURCE;
            }

            if (E_IS_SUCCESS(Status))
            {
                break;
            }

            DASSERT(E_IS_SUCCESS(HalFreeInterruptRedirection(GSI, 1)));
        }

        if (!E_IS_SUCCESS(Status))
        {
            DASSERT(E_IS_SUCCESS(HalUnregisterInterrupt(&HpetContext->Interrupt)));
            return Status;
        }
    }


    //
    // Do extra setups for HPET.
    // (Timer type, interrupt enable, ...)
    //

    // Set main counter to zero.
    HalHpetWriteRegister64(HpetContext->BaseAddress, HPET_REGISTER_MAIN_COUNTER, 0);

    Configuration.TN_INT_TYPE_CNF = 0; // edge-triggered
    Configuration.TN_INT_ENB_CNF = 1; // interrupt enable
    Configuration.TN_TYPE_CNF = 1; // periodic mode
    Configuration.TN_VAL_SET_CNF = 1; // we'll change comparator after write this
    Configuration.TN_32MODE_CNF = 0; // 64-bit mode
    Configuration.TN_FSB_EN_CNF = 0; // disable FSB

    HalHpetWriteRegister64(HpetContext->BaseAddress, ConfigurationRegister, Configuration.Value);

    ULONG ComparatorRegister = HPET_REGISTER_TIMER_N_COMPARATOR(TimerNumber);

    // Set comparator register (Must write TN_TYPE_CNF to 1 previously)
    // (10^15 / COUNTER_CLK_PERIOD) Hz
    // => (10^12 / COUNTER_CLK_PERIOD) times per 1ms
    U64 CounterPerMs = 0xe8d4a51000ULL / HpetContext->Capabilities.COUNTER_CLK_PERIOD;
    DbgTraceF(TraceLevelDebug, "HPET count per ms 0x%08llx\n", CounterPerMs);
    HalHpetWriteRegister64(HpetContext->BaseAddress, ComparatorRegister, CounterPerMs);
    HalHpetWriteRegister64(HpetContext->BaseAddress, ComparatorRegister, CounterPerMs); // once more!
    HpetContext->TimerPeriod0 = CounterPerMs;

    // Finally, set the overall enable bit.
    HalHpetWriteRegisterByMask64(HpetContext->BaseAddress, HPET_REGISTER_CONFIGURATION, 
        HPET_GENERAL_CONFIGURATION_ENABLE_CNF, HPET_GENERAL_CONFIGURATION_ENABLE_CNF);

    return E_SUCCESS;
}

/**
 * @brief Returns HPET timer frequency.
 * 
 * @param [out] Frequency   HPET timer Frequency.
 * 
 * @return ESTATUS code.
 */
ESTATUS
HalHpetGetFrequency(
    OUT U64 *Frequency)
{
    if (!HalpHpetContext.Initialized)
    {
        return E_FAILED;
    }

    DASSERT(HalpHpetContext.Capabilities.COUNTER_CLK_PERIOD);

    // 0x38d7ea4c68000ULL = 10^15
    *Frequency = 0x38d7ea4c68000ULL / HalpHpetContext.Capabilities.COUNTER_CLK_PERIOD;

    return E_SUCCESS;
}

/**
 * @brief Returns HPET main counter.
 * 
 * @param [out] Counter     HPET main counter.
 * 
 * @return ESTATUS code.
 */
ESTATUS
HalHpetReadCounter(
    OUT U64 *Counter)
{
    if (!HalpHpetContext.Initialized)
    {
        return E_FAILED;
    }

    U64 Configuration = HalHpetReadRegister64(HalpHpetContext.BaseAddress, HPET_REGISTER_CONFIGURATION);
    if (!(Configuration & HPET_GENERAL_CONFIGURATION_ENABLE_CNF))
    {
        return E_FAILED;
    }

    *Counter = HalHpetReadRegister64(HalpHpetContext.BaseAddress, HPET_REGISTER_MAIN_COUNTER);

    return E_SUCCESS;
}

/**
 * @brief Initializes the HPET.
 * 
 * @return ESTATUS code.
 */
ESTATUS
HalHpetInitialize(
    VOID)
{
    ESTATUS Status = HalpHpetPreInitialize(&HalpHpetContext, HalAcpiHpet);

    if (!E_IS_SUCCESS(Status))
    {
        return Status;
    }

    Status = HalpHpetEnable(&HalpHpetContext);

    if (!E_IS_SUCCESS(Status))
    {
        // @todo: Do the cleanup.
        //        Unregister interrupt, unmap base address, free virtual page, ...
    }

    return Status;
}

