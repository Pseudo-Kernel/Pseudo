#pragma once

#include <base/base.h>


//
// IRQLs.
//

#define IRQL_VALID(_irql)                   (IRQL_LOWEST <= (_irql) && (_irql) <= IRQL_HIGHEST)
#define VECTOR_VALID(_vector)               (0x00 <= (_vector) && (_vector) <= 0xff)
#define IRQ_VECTOR_VALID(_vector)           (0x20 <= (_vector) && (_vector) <= 0xff) // Vector 0x00 to 0x1f is reserved for other purposes

#define VECTOR_TO_IRQL(_vector)             (((KIRQL)(_vector) & 0xf0) >> 4)
#define IRQL_TO_VECTOR_START(_irql)         (((ULONG)(_irql) & 0x0f) << 4)
#define VECTOR_TO_GROUP_IRQ_INDEX(_vector)  ((ULONG)(_vector) & 0x0f)

// The interrupt-priority class is the value of bits 7:4 of the interrupt vector.
#define IRQL_LOWEST                         0       // Lowest
#define IRQL_NOT_USED_1                     1       // Not used (Vector range 0x00 - 0x1f is reserved by intel.)
#define IRQL_NORMAL                         2       // Normal
#define IRQL_CONTEXT_SWITCH                 3       // Context switch
#define IRQL_LEGACY                         4       // Legacy (Old PIC Interrupts)
#define IRQL_DEVICE_1                       5       // Used by devices
#define IRQL_DEVICE_2                       6       // Used by devices
#define IRQL_DEVICE_3                       7       // Used by devices
#define IRQL_DEVICE_4                       8       // Used by devices
#define IRQL_DEVICE_5                       9       // Used by devices
#define IRQL_DEVICE_6                       10      // Used by devices
#define IRQL_DEVICE_7                       11      // Used by devices
#define IRQL_DEVICE_8                       12      // Used by devices
#define IRQL_RESERVED_SPURIOUS              13      // Spurious vector
#define IRQL_HIGH                           14      // High priority
#define IRQL_IPI                            15      // Used by IPI delivery (Highest priority)
#define IRQL_HIGHEST                        IRQL_IPI

#define IRQL_COUNT                          (IRQL_HIGHEST - IRQL_LOWEST + 1)


//
// Legacy ISA IRQs.
//

#define ISA_IRQ_TIMER                       0
#define ISA_IRQ_PS2_KBD                     1
#define ISA_IRQ_SERIAL4                     3 // shared
#define ISA_IRQ_SERIAL2                     3 // shared
#define ISA_IRQ_SERIAL1                     4 // shared
#define ISA_IRQ_SERIAL3                     4 // shared
#define ISA_IRQ_PARALLEL3                   5 // shared
#define ISA_IRQ_SOUND                       5 // shared
#define ISA_IRQ_FLOPPY                      6 // shared
#define ISA_IRQ_PARALLEL2                   7 // shared
#define ISA_IRQ_PARALLEL1                   7 // shared
#define ISA_IRQ_RTC                         8
#define ISA_IRQ_PS2_MOUSE                   12
#define ISA_IRQ_ATA_PRIMARY                 14
#define ISA_IRQ_ATA_SECONDARY               15


//
// Reserved vector numbers.
//

#define VECTOR_SPURIOUS                     (IRQL_TO_VECTOR_START(IRQL_RESERVED_SPURIOUS) + 0)
#define VECTOR_LVT_TIMER                    (IRQL_TO_VECTOR_START(IRQL_CONTEXT_SWITCH) + 0)
#define VECTOR_LVT_ERROR                    (IRQL_TO_VECTOR_START(IRQL_CONTEXT_SWITCH) + 1)
#define VECTOR_PLATFORM_TIMER               (IRQL_TO_VECTOR_START(IRQL_DEVICE_1) + 0)


typedef enum _KINTERRUPT_RESULT
{
    InterruptResultMinimum = 0,
    InterruptError = InterruptResultMinimum, // Error
    InterruptAccepted, // Accepted
    InterruptCallNext, // Call next handler
    InterruptResultMaximum = InterruptCallNext,
} KINTERRUPT_RESULT;



typedef struct _KINTERRUPT          KINTERRUPT, *PKINTERRUPT;

typedef
KINTERRUPT_RESULT
(KERNELAPI *PKINTERRUPT_ROUTINE)(
    IN PKINTERRUPT Interrupt,
    IN PVOID InterruptContext,
    IN PVOID InterruptStackFrame);


typedef struct _KINTERRUPT
{
    PKINTERRUPT_ROUTINE InterruptRoutine;
    PVOID InterruptContext;                 // Interrupt context to be passed to InterruptRoutine()
    U8 InterruptVector;                     // Index of IDT[]. (InterruptVector[7:4] = IRQL = TPR[7:4])
    BOOLEAN Connected;                      // TRUE if connected to the interrupt chain
    BOOLEAN AutoEoi;                        // TRUE if InterruptRoutine() handles the EOI.
//    KAFFINITY InterruptAffinity;
//    ULONG32 Flags;

    DLIST_ENTRY InterruptList;
} KINTERRUPT;


#define IRQS_PER_IRQ_GROUP                  16  // 16 IRQs per 1 IRQL (IRQL = TPR[7:4] = CR8[3:0])
#define IRQ_GROUPS_MAX                      (0x100 / IRQS_PER_IRQ_GROUP) // (total 0x100 IDT entries) / (IRQs per group)

typedef struct _KIRQ
{
    BOOLEAN Allocated;
    BOOLEAN Shared;
    U32 SharedCount;
    DLIST_ENTRY InterruptListHead;          // Listhead of connected interrupts
} KIRQ;

typedef struct _KIRQ_GROUP
{
    KSPIN_LOCK GroupLock;                   // IRQL group lock
    KIRQL GroupIrql;                        // Group IRQL
    KIRQL PreviousIrql;                     // Previous IRQL when GroupLock locked
    KIRQ Irq[IRQS_PER_IRQ_GROUP];
} KIRQ_GROUP;





#define INTERRUPT_IRQ_SHARED            0x00000001  // IRQ is shared.
#define INTERRUPT_IRQ_LEGACY_ISA        0x00000002  // Legacy ISA IRQ.
#define INTERRUPT_IRQ_HINT_EXACT_MATCH  0x00000004  // 
#define INTERRUPT_AUTO_EOI              0x10000000  // Sends the EOI automatically if specified.



ESTATUS
KERNELAPI
KeAllocateIrqVector(
    IN KIRQL GroupIrql,
    IN ULONG Count,
    OPTIONAL IN ULONG VectorHint, // Ignored if 0
    IN ULONG Flags,
    OUT ULONG *StartingVector,
    IN BOOLEAN LockGroupIrq);

ESTATUS
KERNELAPI
KeIsIrqVectorAllocated(
    IN ULONG Vector,
    IN ULONG Flags,
    OUT BOOLEAN *Allocated,
    IN BOOLEAN LockGroupIrq);

ESTATUS
KERNELAPI
KeFreeIrqVector(
    IN ULONG StartingVector,
    IN ULONG Count,
    IN BOOLEAN LockGroupIrq);

ESTATUS
KERNELAPI
KeConnectInterrupt(
    IN PKINTERRUPT Interrupt,
    IN ULONG Vector,
    IN ULONG Flags);

ESTATUS
KERNELAPI
KeDisconnectInterrupt(
    IN PKINTERRUPT Interrupt);

ESTATUS
KERNELAPI
KeInitializeInterrupt(
    OUT PKINTERRUPT Interrupt,
    IN PKINTERRUPT_ROUTINE InterruptRoutine,
    IN PVOID InterruptContext,
    IN U64 Reserved /*InterruptAffinity*/);

VOID
KERNELAPI
KiCallInterruptChain(
    IN U8 Vector,
    OPTIONAL IN PVOID InterruptStackFrame);

VOID
KERNELAPI
KiInitializeIrqGroups(
    VOID);

