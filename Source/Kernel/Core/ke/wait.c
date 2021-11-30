
#include <base/base.h>
#include <ke/lock.h>
#include <ke/interrupt.h>
#include <mm/pool.h>
#include <ke/wait.h>


VOID
KiInitializeWaitHeader(
    OUT KWAIT_HEADER *Object,
    IN PVOID WaitContext)
{
    KeInitializeSpinlock(&Object->Lock);
    DListInitializeHead(&Object->WaiterThreadList);
    Object->State = 0;
    Object->WaitContext = NULL;
}

BOOLEAN
KiTryLockWaitHeader(
    IN KWAIT_HEADER *WaitHeader,
    OUT KIRQL *Irql)
{
    return KeTryAcquireSpinlockRaiseIrqlToContextSwitch(&WaitHeader->Lock, Irql);
}

VOID
KiLockWaitHeader(
    IN KWAIT_HEADER *WaitHeader,
    OUT KIRQL *Irql)
{
    while (!KiTryLockWaitHeader(WaitHeader, Irql))
        _mm_pause();
}

VOID
KiUnlockWaitHeader(
    IN KWAIT_HEADER *WaitHeader,
    IN KIRQL PrevIrql)
{
    KeReleaseSpinlockLowerIrql(&WaitHeader->Lock, PrevIrql);
}

