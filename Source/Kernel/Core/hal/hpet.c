
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

// 
// HPET driver implementation prerequisites:
// 1. HPET must support 64-bit wide counter (COUNT_SIZE_CAP = 1).
// 2. HPET must support 3 timers at least (NUM_TIM_CAP >= 3).
// 3. HPET must support 1 timer which is periodic capable (Tn_PER_INT_CAP = 1).
// 
// 4. HPET may report strange interrupt routing capability bits (Tn_INT_ROUTE_CAP).
//    e.g.) Reporting IOAPIC IRQ which doesn't exists in IOAPIC.
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

    KINTERRUPT Interrupt;
} HAL_HPET_CONTEXT;


U64
HalHpetReadRegister(
    IN VIRTUAL_ADDRESS Base,
    IN ULONG Register)
{
    DASSERT(0 <= Register && Register < HPET_SIZE_REGISTER_SPACE);
    DASSERT(!(Register & (HPET_ACCESS_ALIGNMENT - 1)));

    volatile U64 *p = (volatile U64 *)(Base + Register);

    return *p;
}

U64
HalHpetWriteRegister(
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

U64
HalHpetWriteRegisterByMask(
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


ESTATUS
HalHpetInitialize(
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
    Capabilities.Value = HalHpetReadRegister(VirtualAddress, HPET_REGISTER_CAPABILITIES_ID);

    // Must support 64-bit main counter and 3 timers.
    if (!Capabilities.COUNT_SIZE_CAP || 
        Capabilities.NUM_TIM_CAP < 3)
    {
        Status = E_NOT_SUPPORTED;
        goto Cleanup;
    }

    U8 PeriodicTimerNumber = ~0;

    for (U8 i = 0; i <= Capabilities.NUM_TIM_CAP; i++)
    {
        HPET_CONFIGURATION_AND_CAPABILITIES Configuration;
        Configuration.Value = HalHpetReadRegister(VirtualAddress, HPET_REGISTER_TIMER_N_CONF_CAPABILITY(i));

        if (!Configuration.TN_SIZE_CAP)
        {
            Status = E_NOT_SUPPORTED;
            goto Cleanup;
        }

        if (Configuration.TN_PER_INT_CAP)
        {
            if (PeriodicTimerNumber == (U8)~0)
            {
                PeriodicTimerNumber = i;
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
    HalApicSendEoi(HalApicBase);
    return InterruptAccepted;
}

ESTATUS
HalHpetEnable(
    IN HAL_HPET_CONTEXT *HpetContext)
{
    // 
    // @todo
    // 1. Allocate IRQ vector VV.
    // 2. Allocate IOAPIC redirection table (IOREDTBL) entry XX (by TN_INT_ROUTE_CAP bits).
    // 3. Set interrupt redirection (IOxAPIC -> VECTOR): IOREDTBL[XX].Vector = VV
    // 4. Route the HPET interrupt (HPET -> IOxAPIC): TN_INT_ROUTE_CNF = XX
    // 5. Do extra setups for HPET. (timer type, interrupt enable, comparator, ...)
    // 

    U32 Vector = VECTOR_PLATFORM_TIMER;
    ESTATUS Status = HalRegisterInterrupt(&HpetContext->Interrupt, &HalIsrHighPrecisionTimer, 
        HpetContext, VECTOR_TO_IRQL(Vector), Vector, NULL);

    if (!E_IS_SUCCESS(Status))
    {
        return Status;
    }

    HPET_CONFIGURATION_AND_CAPABILITIES Configuration;
    ULONG ConfigurationRegister = HPET_REGISTER_TIMER_N_CONF_CAPABILITY(HpetContext->PeriodicTimerNumber);
    Configuration.Value = HalHpetReadRegister(HpetContext->BaseAddress, ConfigurationRegister);

    for (U32 i = 0; i < 32; i++)
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
        HalHpetWriteRegister(HpetContext->BaseAddress, ConfigurationRegister, Configuration.Value);

        //
        // After write, We must prove TN_INT_ROUTE_CNF by reading back once.
        //
        // Tn_INT_ROUTE_CNF
        // If the value is not supported by this prarticular timer, then the value
        // read back will not match what is written. 
        // [From: IA-PC HPET Specification 1.0a]
        //

        Configuration.Value = HalHpetReadRegister(HpetContext->BaseAddress, ConfigurationRegister);

        if (Configuration.TN_INT_ROUTE_CNF == GSI)
        {
            // Write was successful.
            Status = HalSetInterruptRedirection(i, Vector, KeGetCurrentProcessorId(), 
                /* edge-triggered, high active */
                INTERRUPT_REDIRECTION_FLAG_SET_POLARITY | INTERRUPT_REDIRECTION_FLAG_SET_TRIGGER_MODE);
        }
        else
        {
            Status = E_NOT_ENOUGH_RESOURCE;
        }

        if (!E_IS_SUCCESS(Status))
        {
            DASSERT(E_IS_SUCCESS(HalFreeInterruptRedirection(GSI, 1)));
        }
    }

    if (!E_IS_SUCCESS(Status))
    {
        DASSERT(E_IS_SUCCESS(HalUnregisterInterrupt(&HpetContext->Interrupt)));
        return Status;
    }

    Configuration.TN_INT_TYPE_CNF = 0; // edge-triggered
    Configuration.TN_INT_ENB_CNF = 1; // interrupt enable
    Configuration.TN_TYPE_CNF = 1; // periodic mode
    Configuration.TN_VAL_SET_CNF = 1; // we'll modify comparator register
    Configuration.TN_32MODE_CNF = 0; // 64-bit mode
    Configuration.TN_FSB_EN_CNF = 0; // disable FSB

    HalHpetWriteRegister(HpetContext->BaseAddress, ConfigurationRegister, Configuration.Value);

    ULONG ComparatorRegister = HPET_REGISTER_TIMER_N_COMPARATOR(HpetContext->PeriodicTimerNumber);

    // @todo: fixme!
    HalHpetWriteRegister(HpetContext->BaseAddress, ComparatorRegister, 12345678);


    // 
    // The Device Driver code should do the following for an available timer:
    // 1. Set the timer type field (selects one-shot or periodic).
    // 2. Set the interrupt enable
    // 3. Set the comparator match
    // 4. Set the Overall Enable bit (Offset 04h, bit 0).
    //    This starts the main counter and enables comparators to deliver interrupts. 
    // 

    //HalHpetWriteRegisterByMask(HpetContext->BaseAddress, HPET_REGISTER_CONFIGURATION, 
    //    HPET_GENERAL_CONFIGURATION_ENABLE_CNF, HPET_GENERAL_CONFIGURATION_VALID_MASK);

    return E_NOT_IMPLEMENTED;
}

