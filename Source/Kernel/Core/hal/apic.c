
/**
 * @file apic.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements local APIC routines.
 * @version 0.1
 * @date 2021-10-18
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <base/base.h>
#include <ke/ke.h>
#include <mm/mm.h>
#include <init/bootgfx.h>
#include <hal/apic.h>
#include <hal/ioapic.h>

#define IA32_APIC_BASE              0x1b

#define SPIN_WAIT(_condition)   \
    while((_condition)) {       \
        _mm_pause();            \
    }


VOID
KERNELAPI
HalApicEnable(
    VOID)
{
    U64 Value = __readmsr(IA32_APIC_BASE);

    //
    // IA32_APIC_BASE_MSR[8] = BSP (1 = BSP, 0 = AP)
    // IA32_APIC_BASE_MSR[11] = APIC global enable/disable (enable = 1, disable = 0)
    //

    __writemsr(IA32_APIC_BASE, Value | 0x800);
}

PHYSICAL_ADDRESS
KERNELAPI
HalApicGetBase(
    VOID)
{
    U64 Value = __readmsr(IA32_APIC_BASE);

    // IA32_APIC_BASE_MSR[M:12] = APIB base physical address
    return (Value & ~0xfff);
}

VOID
KERNELAPI
HalApicSetBase(
    IN PHYSICAL_ADDRESS PhysicalApicBase)
{
    U64 Value = __readmsr(IA32_APIC_BASE);

    // IA32_APIC_BASE_MSR[M:12] = APIB base physical address
    Value = (Value & 0xfff) | ((U64)PhysicalApicBase & ~0xfff);

    __writemsr(IA32_APIC_BASE, Value);
}

U8
KERNELAPI
HalApicGetId(
    IN PTR ApicBase)
{
    U32 volatile *ApicId = (U32 volatile *)LAPIC_REG(ApicBase, LAPIC_ID);
    return (U8)(((*ApicId) >> 24) & 0xff);
}

VOID
KERNELAPI
HalApicSetDefaultState(
    IN PTR ApicBase)
{
    //
	// Initialize LAPIC registers to well-known state.
    //

    // Mask all LVT entries.
    // Timer, CMCI, LINT0, LINT1, ERROR, PERFCNT, THERMAL => Masked
    *(U32 volatile *)LAPIC_REG(ApicBase, LAPIC_TIMER) = 0x10000;
    *(U32 volatile *)LAPIC_REG(ApicBase, LAPIC_CMCI) = 0x10000;
    *(U32 volatile *)LAPIC_REG(ApicBase, LAPIC_LINT0) = 0x10000;
    *(U32 volatile *)LAPIC_REG(ApicBase, LAPIC_LINT1) = 0x10000;
    *(U32 volatile *)LAPIC_REG(ApicBase, LAPIC_ERROR) = 0x10000;
    *(U32 volatile *)LAPIC_REG(ApicBase, LAPIC_PERFCNT) = 0x10000;
    *(U32 volatile *)LAPIC_REG(ApicBase, LAPIC_THERMAL) = 0x10000;


    *(U32 volatile *)LAPIC_REG(ApicBase, LAPIC_DFR) = ~0;
    *(U32 volatile *)LAPIC_REG(ApicBase, LAPIC_INITIAL_COUNT) = 0;

    // We use cr8 (not the TPR register) to change TPR
    //*(U32 volatile *)LAPIC_REG(ApicBase, LAPIC_TPR) = 0;
}

VOID
KERNELAPI
HalApicSetSpuriousVector(
    IN PTR ApicBase, 
    IN BOOLEAN Enable, 
    IN U8 Vector)
{
	U32 volatile *Register = (U32 volatile *)LAPIC_REG(ApicBase, LAPIC_SPURIOUS_INTV);

	// LAPIC spurious register
	// LAPIC_SPURIOUS_INTV_REG[7:0] = Spurious vector
	// LAPIC_SPURIOUS_INTV_REG[8] = LAPIC enable

	if (Enable)
		*Register = ((*Register) & ~0xff) | 0x100 | Vector;
	else
		*Register &= ~0x100;
}

VOID
KERNELAPI
HalApicSetErrorVector(
    IN PTR ApicBase, 
    IN BOOLEAN Enable, 
    IN U8 Vector)
{
	U32 volatile *Register = (U32 volatile *)LAPIC_REG(ApicBase, LAPIC_ERROR);

	// Local APIC error register
	// LAPIC_ERROR[7:0] = Error vector
	// LAPIC_ERROR[16] = Masked

	if (Enable)
		*Register = ((*Register) & ~0xff) | Vector;
	else
		*Register |= 0x10000;
}

VOID
KERNELAPI
HalApicSetTimerVector(
    IN PTR ApicBase, 
	IN U32 PeriodicCount, 
	IN U8 Vector)
{
    U32 volatile *DivideConf = (U32 volatile *)LAPIC_REG(ApicBase, LAPIC_DIV_CONF);
    U32 volatile *Timer = (U32 volatile *)LAPIC_REG(ApicBase, LAPIC_TIMER);
    U32 volatile *InitialCount = (U32 volatile *)LAPIC_REG(ApicBase, LAPIC_INITIAL_COUNT);

    //
    // The timer is started by writing to the initial-count register.
    //

    *DivideConf = (*DivideConf & ~0x0b) | 0x03; // divide by 16
    *Timer = 0x20000 | Vector; // we'll use periodic mode (APIC.LVT.TMR[18:17] = 01)
    *InitialCount = PeriodicCount; // start the timer (if PeriodicCount > 0).
}

VOID
KERNELAPI
HalApicReadTimerCounter(
    IN PTR ApicBase,
    OUT U32 *InitialCounter,
    OUT U32 *CurrentCounter)
{
    U32 volatile *InitialCount = (U32 volatile *)LAPIC_REG(ApicBase, LAPIC_INITIAL_COUNT);
    U32 volatile *CurrentCount = (U32 volatile *)LAPIC_REG(ApicBase, LAPIC_CURRENT_COUNT);

    *CurrentCounter = *CurrentCount;
    *InitialCounter = *InitialCount;
}

VOID
KERNELAPI
HalApicSendEoi(
	IN PTR ApicBase)
{
	U32 volatile *Register = (U32 volatile *)LAPIC_REG(ApicBase, LAPIC_EOI);
	*Register = 0;
}


BOOLEAN
KERNELAPI
HalIsBootstrapProcessor(
	VOID)
{
	return !!(__readmsr(IA32_APIC_BASE) & 0x100);
}

VOID
KERNELAPI
HalApicStartProcessor(
    IN PTR ApicBase,
	IN ULONG ApicId, 
	IN U8 ResetVector)
{
    // 
    // Send IIPI-SIPI-SIPI sequence to reset the processor.
    // NOTE: Processor starting address = (ResetVector * 4096).
    // 

	U32 volatile *ICR0 = (U32 volatile *)LAPIC_REG(ApicBase, LAPIC_ICR_LOW);
	U32 volatile *ICR1 = (U32 volatile *)LAPIC_REG(ApicBase, LAPIC_ICR_HIGH);

	if (!ResetVector)
	{
		FATAL("ResetVector must not be 0!");
	}

    U32 High = LAPIC_ICR_HIGH_DESTINATION_FIELD(ApicId);
    U32 Low_IIPI = LAPIC_ICR_VECTOR(0) |
        LAPIC_ICR_DELIVERY_MODE(LAPIC_ICR_DELIVER_INIT) |
        LAPIC_ICR_DEST_MODE_PHYSICAL |
        LAPIC_ICR_LEVEL_ASSERT |
        LAPIC_ICR_TRIGGERED_EDGE |
        LAPIC_ICR_DEST_SHORTHAND(LAPIC_ICR_DEST_NO_SHORTHAND);

    U32 Low_SIPI = LAPIC_ICR_VECTOR(ResetVector) |
        LAPIC_ICR_DELIVERY_MODE(LAPIC_ICR_DELIVER_STARTUP) |
        LAPIC_ICR_DEST_MODE_PHYSICAL |
        LAPIC_ICR_LEVEL_ASSERT |
        LAPIC_ICR_TRIGGERED_EDGE |
        LAPIC_ICR_DEST_SHORTHAND(LAPIC_ICR_DEST_NO_SHORTHAND);

    // Use rdtsc to wait delay (not accurate).
    U64 ReferenceTscPerMilliseconds = 5000000; // Assume 5GHz/s.
    volatile U64 TscExpire = 0;

    U64 RFlags = __readeflags();
    _disable();

	// Wait for deliver pending
    BGXTRACE_DBG("Wait for delivery pending before send INIT IPI\n");
	SPIN_WAIT(*ICR0 & LAPIC_ICR_DELIVER_PENDING);

    // Send INIT IPI.
	/* No Shorthand, Edge Triggered, INIT, Physical, Assert */
    BGXTRACE_DBG("Send INIT IPI\n");
	*ICR1 = High;
	*ICR0 = Low_IIPI;
    BGXTRACE_DBG("Wait for delivery pending after send INIT IPI\n");
	SPIN_WAIT(*ICR0 & LAPIC_ICR_DELIVER_PENDING);

    // Wait 10ms.
    BGXTRACE_DBG("Wait for 10ms delay\n");
    TscExpire = __rdtsc() + ReferenceTscPerMilliseconds * 10; // 10ms
    SPIN_WAIT(__rdtsc() < TscExpire);

	// Send startup IPI.
	/* No Shorthand, Edge Triggered, All, Physical, Assert */
    BGXTRACE_DBG("Send STARTUP IPI\n");
	*ICR1 = High;
	*ICR0 = Low_SIPI;
    BGXTRACE_DBG("Wait for delivery pending after send STARTUP IPI\n");
	SPIN_WAIT(*ICR0 & LAPIC_ICR_DELIVER_PENDING);

    // Wait 200us.
    BGXTRACE_DBG("Wait for 200us delay\n");
    TscExpire = __rdtsc() + ReferenceTscPerMilliseconds / 5; // 200us
    SPIN_WAIT(__rdtsc() < TscExpire);

	// Send startup IPI.
	/* No Shorthand, Edge Triggered, All, Physical, Assert */
    BGXTRACE_DBG("Send STARTUP IPI\n");
	*ICR1 = High;
	*ICR0 = Low_SIPI;
    BGXTRACE_DBG("Wait for delivery pending after send STARTUP IPI\n");
	SPIN_WAIT(*ICR0 & LAPIC_ICR_DELIVER_PENDING);

    // Wait 200us.
    BGXTRACE_DBG("Wait for 200us delay\n");
    TscExpire = __rdtsc() + ReferenceTscPerMilliseconds / 5; // 200us
    SPIN_WAIT(__rdtsc() < TscExpire);

    BGXTRACE_DBG("IIPI-SIPI-SIPI done\n");

    if (RFlags & RFLAG_IF)
        _enable();
}
