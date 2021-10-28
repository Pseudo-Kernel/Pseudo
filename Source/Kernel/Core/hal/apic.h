
#pragma once


//
// Local APIC registers.
//

#define LAPIC_REG(_base, _reg)      ((U64)(_base) + (U64)(_reg))

#define LAPIC_ID                    0x20       // RW, [xx:24] -> LAPIC ID
#define LAPIC_VERSION               0x30       // R

#define LAPIC_TPR                   0x80       // RW
#define LAPIC_APR                   0x90       // R
#define LAPIC_PPR                   0xa0       // R
#define LAPIC_EOI                   0xb0       // W
#define LAPIC_RRD                   0xc0       // R
#define LAPIC_LDR                   0xd0       // RW
#define LAPIC_DFR                   0xe0       // RW 10.6.2.2
#define LAPIC_SPURIOUS_INTV         0xf0       // RW 10.9

#define LAPIC_ISR_NEXT              0x10
#define LAPIC_ISR0                  0x100      // R
#define LAPIC_ISR1                  0x110      // R
#define LAPIC_ISR2                  0x120      // R
#define LAPIC_ISR3                  0x130      // R
#define LAPIC_ISR4                  0x140      // R
#define LAPIC_ISR5                  0x150      // R
#define LAPIC_ISR6                  0x160      // R
#define LAPIC_ISR7                  0x170      // R

#define LAPIC_TMR_NEXT              0x10
#define LAPIC_TMR0                  0x180      // R
#define LAPIC_TMR1                  0x190      // R
#define LAPIC_TMR2                  0x1a0      // R
#define LAPIC_TMR3                  0x1b0      // R
#define LAPIC_TMR4                  0x1c0      // R
#define LAPIC_TMR5                  0x1d0      // R
#define LAPIC_TMR6                  0x1e0      // R
#define LAPIC_TMR7                  0x1f0      // R

#define LAPIC_IRR_NEXT              0x10
#define LAPIC_IRR0                  0x200      // R
#define LAPIC_IRR1                  0x210      // R
#define LAPIC_IRR2                  0x220      // R
#define LAPIC_IRR3                  0x230      // R
#define LAPIC_IRR4                  0x240      // R
#define LAPIC_IRR5                  0x250      // R
#define LAPIC_IRR6                  0x260      // R
#define LAPIC_IRR7                  0x270      // R

#define LAPIC_ESR                   0x280      // R


#define LAPIC_CMCI                  0x2f0      // RW
#define LAPIC_ICR_LOW               0x300      // RW
#define LAPIC_ICR_HIGH              0x310      // RW

#define LAPIC_TIMER                 0x320      // RW
#define LAPIC_THERMAL               0x330      // RW
#define LAPIC_PERFCNT               0x340      // RW
#define LAPIC_LINT0                 0x350      // RW
#define LAPIC_LINT1                 0x360      // RW
#define LAPIC_ERROR                 0x370      // RW

#define LAPIC_INITIAL_COUNT         0x380      // RW
#define LAPIC_CURRENT_COUNT         0x390      // R
#define LAPIC_DIV_CONF              0x3e0      // RW


//
// LAPIC ICR register.
//

//
// LAPIC ICR vector (ICR[7:0])
//

#define LAPIC_ICR_VECTOR(_v)                ((_v) & 0xff)

//
// LAPIC ICR delivery mode (ICR[10:8])
//

#define LAPIC_ICR_DELIVERY_MODE(_v)         (((_v) & 7) << 8)
#define LAPIC_ICR_DELIVER_FIXED             0
#define LAPIC_ICR_DELIVER_LOWEST_PRI        1
#define LAPIC_ICR_DELIVER_SMI               2
#define LAPIC_ICR_DELIVER_NMI               4
#define LAPIC_ICR_DELIVER_INIT              5
#define LAPIC_ICR_DELIVER_STARTUP           6

//
// LAPIC ICR destination mode (ICR[11])
//

#define LAPIC_ICR_DEST_MODE_LOGICAL         (1 << 11)
#define LAPIC_ICR_DEST_MODE_PHYSICAL        0

//
// LAPIC ICR delivery status (Read Only, ICR[12])
//

#define LAPIC_ICR_DELIVER_PENDING           (1 << 12)

//
// LAPIC ICR level (ICR[14])
// Only INIT level deassert messages are allowed to have the level bit set to 0.
//

#define LAPIC_ICR_LEVEL_ASSERT              (1 << 14)
#define LAPIC_ICR_LEVEL_DEASSERT            0

//
// LAPIC ICR trigger mode (ICR[15])
//

#define LAPIC_ICR_TRIGGERED_LEVEL           (1 << 15)
#define LAPIC_ICR_TRIGGERED_EDGE            0

//
// LAPIC ICR destination shorthand (ICR[19:18])
//

#define LAPIC_ICR_DEST_SHORTHAND(_v)        (((_v) & 3) << 18)
#define LAPIC_ICR_DEST_NO_SHORTHAND         0
#define LAPIC_ICR_DEST_SELF                 1
#define LAPIC_ICR_DEST_ALL_BROADCAST        2
#define LAPIC_ICR_DEST_BROADCAST_EXPT_SELF  3

//
// LAPIC ICR destination field (ICR[63:56] / ICR[59:56])
// - ICR[63:56] when Pentium4 / Xeon.
// - ICR[59:56] when P6 Family.
//

#define LAPIC_ICR_HIGH_DESTINATION_FIELD(_v)    ((_v) << (56-32))




VOID
KERNELAPI
HalApicEnable(
    VOID);

PHYSICAL_ADDRESS
KERNELAPI
HalApicGetBase(
    VOID);

VOID
KERNELAPI
HalApicSetBase(
    IN PHYSICAL_ADDRESS PhysicalApicBase);

U8
KERNELAPI
HalApicGetId(
    IN PTR ApicBase);

VOID
KERNELAPI
HalApicSetDefaultState(
    IN PTR ApicBase);

VOID
KERNELAPI
HalApicSetSpuriousVector(
    IN PTR ApicBase, 
    IN BOOLEAN Enable, 
    IN U8 Vector);

VOID
KERNELAPI
HalApicSetErrorVector(
    IN PTR ApicBase, 
    IN BOOLEAN Enable, 
    IN U8 Vector);

VOID
KERNELAPI
HalApicSetTimerVector(
    IN PTR ApicBase, 
	IN U32 PeriodicCount, 
	IN U8 Vector);

VOID
KERNELAPI
HalApicSetLINTxVector(
    IN PTR ApicBase, 
    IN U8 LINTx,
	IN BOOLEAN ActiveLow,
    IN BOOLEAN LevelSensitive,
    IN U32 DeliveryMode,
	IN U8 Vector);

VOID
KERNELAPI
HalApicReadTimerCounter(
    IN PTR ApicBase,
    OUT U32 *InitialCounter,
    OUT U32 *CurrentCounter);

VOID
KERNELAPI
HalApicSendEoi(
	IN PTR ApicBase);

BOOLEAN
KERNELAPI
HalIsBootstrapProcessor(
	VOID);

VOID
KERNELAPI
HalApicStartProcessor(
    IN PTR ApicBase,
	IN ULONG ApicId, 
	IN U8 ResetVector);

