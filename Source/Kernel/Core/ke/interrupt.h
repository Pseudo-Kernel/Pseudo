#pragma once

#include <base/base.h>


//
// IRQLs.
//

#define	IRQL_VALID(_irql)					(IRQL_LOWEST <= (_irql) && (_irql) <= IRQL_HIGHEST)
#define VECTOR_VALID(_vector)               (0x00 <= (_vector) && (_vector) <= 0xff)
#define IRQ_VECTOR_VALID(_vector)           (0x20 <= (_vector) && (_vector) <= 0xff) // Vector 0x00 to 0x1f is reserved for other purposes

#define VECTOR_TO_IRQL(_vector)             (((KIRQL)(_vector) & 0xf0) >> 4)
#define IRQL_TO_VECTOR_START(_irql)         (((ULONG)(_irql) & 0x0f) << 4)
#define VECTOR_TO_GROUP_IRQ_INDEX(_vector)  ((ULONG)(_vector) & 0x0f)

// The interrupt-priority class is the value of bits 7:4 of the interrupt vector.
#define	IRQL_LOWEST							0       // Lowest
#define	IRQL_NOT_USED_1						1       // Not used (Vector range 0x00 - 0x1f is reserved by intel.)
#define	IRQL_NORMAL							2       // Normal
#define	IRQL_CONTEXT_SWITCH					3       // Context switch
#define	IRQL_LEGACY_ISA						4       // Legacy ISA
#define	IRQL_DEVICE_1						5       // Used by devices
#define	IRQL_DEVICE_2						6       // Used by devices
#define	IRQL_DEVICE_3						7       // Used by devices
#define	IRQL_DEVICE_4						8       // Used by devices
#define	IRQL_DEVICE_5						9       // Used by devices
#define	IRQL_DEVICE_6						10      // Used by devices
#define	IRQL_DEVICE_7						11      // Used by devices
#define	IRQL_DEVICE_8						12      // Used by devices
#define	IRQL_RESERVED_SPURIOUS				13      // Spurious vector
#define	IRQL_HIGH							14      // High priority
#define	IRQL_IPI							15      // Used by IPI delivery (Highest priority)
#define	IRQL_HIGHEST						IRQL_IPI

#define	IRQL_COUNT							(IRQL_HIGHEST - IRQL_LOWEST + 1)



typedef enum _KINTERRUPT_RESULT
{
    InterruptResultMinimum = 0,
    InterruptError = InterruptResultMinimum, // Error
    InterruptAccepted, // Accepted
    InterruptCallNext, // Call next handler
    InterruptResultMaximum = InterruptCallNext,
} KINTERRUPT_RESULT;


typedef
KINTERRUPT_RESULT
(*PKINTERRUPT_ROUTINE)(
    IN struct _KINTERRUPT *Interrupt,
    IN PVOID InterruptContext);


typedef struct _KINTERRUPT
{
    PKINTERRUPT_ROUTINE InterruptRoutine;
    PVOID InterruptContext;                 // Interrupt context to be passed to InterruptRoutine()
    U8 InterruptVector;                     // Index of IDT[]. (InterruptVector[7:4] = IRQL = TPR[7:4])
    BOOLEAN Connected;                      // TRUE if connected to the interrupt chain
//    KAFFINITY InterruptAffinity;
//    ULONG32 Flags;

    DLIST_ENTRY InterruptList;
} KINTERRUPT, *PKINTERRUPT;


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

extern KIRQ_GROUP KiIrqGroup[IRQ_GROUPS_MAX];




#define INTERRUPT_IRQ_SHARED            0x00000001  // IRQ is shared.
#define INTERRUPT_IRQ_LEGACY_ISA        0x00000002  // Legacy ISA IRQ.
#define INTERRUPT_IRQ_HINT_EXACT_MATCH  0x00000004  // 
#define INTERRUPT_AUTO_EOI              0x10000000  // Sends the EOI automatically if specified.
