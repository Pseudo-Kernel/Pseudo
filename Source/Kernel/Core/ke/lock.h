#pragma once

#include <base/base.h>
#include <ke/irql.h>

#pragma pack(push, 4)

typedef struct _KSPIN_LOCK
{
    volatile long Lock;
} KSPIN_LOCK, *PKSPIN_LOCK;

#pragma pack(pop)



VOID
KERNELAPI
KeInitializeSpinlock(
    OUT PKSPIN_LOCK Lock);

BOOLEAN
KERNELAPI
KeTryAcquireSpinlock(
    IN PKSPIN_LOCK Lock);

VOID
KERNELAPI
KeAcquireSpinlock(
    IN PKSPIN_LOCK Lock);

VOID
KERNELAPI
KeReleaseSpinlock(
    IN PKSPIN_LOCK Lock);

BOOLEAN
KERNELAPI
KeIsSpinlockAcquired(
    IN PKSPIN_LOCK Lock);

BOOLEAN
KERNELAPI
KeTryAcquireSpinlockMultiple(
    IN PKSPIN_LOCK *LockList,
    IN ULONG Count);

VOID
KERNELAPI
KeAcquireSpinlockMultiple(
    IN PKSPIN_LOCK *LockList,
    IN ULONG Count);

VOID
KERNELAPI
KeReleaseSpinlockMultiple(
    IN PKSPIN_LOCK *LockList,
    IN ULONG Count);


//
// Spinlock with interrupt flag.
//

BOOLEAN
KERNELAPI
KeTryAcquireSpinlockDisableInterrupt(
    IN PKSPIN_LOCK Lock,
    OUT BOOLEAN *PrevState);

VOID
KERNELAPI
KeAcquireSpinlockDisableInterrupt(
    IN PKSPIN_LOCK Lock,
    OUT BOOLEAN *PrevState);

VOID
KERNELAPI
KeReleaseSpinlockRestoreInterrupt(
    IN PKSPIN_LOCK Lock,
    IN BOOLEAN PrevState);

BOOLEAN
KERNELAPI
KeTryAcquireSpinlockMultipleDisableInterrupt(
    IN PKSPIN_LOCK *LockList,
    IN ULONG Count,
    OUT BOOLEAN *PrevState);

VOID
KERNELAPI
KeAcquireSpinlockMultipleDisableInterrupt(
    IN PKSPIN_LOCK *LockList,
    IN ULONG Count,
    OUT BOOLEAN *PrevState);

VOID
KERNELAPI
KeReleaseSpinlockMultipleDisableInterrupt(
    IN PKSPIN_LOCK *LockList,
    IN ULONG Count,
    IN BOOLEAN PrevState);


//
// Spinlock with IRQL.
//

BOOLEAN
KERNELAPI
KeTryAcquireSpinlockRaiseIrql(
    IN PKSPIN_LOCK Lock,
    IN KIRQL Irql,
    OUT KIRQL *PrevIrql);

VOID
KERNELAPI
KeAcquireSpinlockRaiseIrql(
    IN PKSPIN_LOCK Lock,
    IN KIRQL Irql,
    OUT KIRQL *PrevIrql);

VOID
KERNELAPI
KeReleaseSpinlockLowerIrql(
    IN PKSPIN_LOCK Lock,
    IN KIRQL PrevIrql);

BOOLEAN
KERNELAPI
KeTryAcquireSpinlockMultipleRaiseIrql(
    IN PKSPIN_LOCK *LockList,
    IN ULONG Count,
    IN KIRQL Irql,
    OUT KIRQL *PrevIrql);

VOID
KERNELAPI
KeAcquireSpinlockMultipleRaiseIrql(
    IN PKSPIN_LOCK *LockList,
    IN ULONG Count,
    IN KIRQL Irql,
    OUT KIRQL *PrevIrql);

VOID
KERNELAPI
KeReleaseSpinlockMultipleLowerIrql(
    IN PKSPIN_LOCK *LockList,
    IN ULONG Count,
    IN KIRQL PrevIrql);


#define KeTryAcquireSpinlockRaiseIrqlToContextSwitch(_lock, _prev_irql) \
    KeTryAcquireSpinlockRaiseIrql((_lock), IRQL_CONTEXT_SWITCH, (_prev_irql))

#define KeAcquireSpinlockRaiseIrqlToContextSwitch(_lock, _prev_irql)   \
    KeAcquireSpinlockRaiseIrql((_lock), IRQL_CONTEXT_SWITCH, (_prev_irql))


