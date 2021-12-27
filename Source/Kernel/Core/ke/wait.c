
#include <base/base.h>
#include <ke/lock.h>
#include <ke/interrupt.h>
#include <mm/pool.h>
#include <ke/kprocessor.h>
#include <ke/process.h>
#include <ke/thread.h>
#include <ke/sched.h>
#include <ke/timer.h>
#include <ke/wait.h>


VOID
KiInitializeWaitHeader(
    OUT KWAIT_HEADER *Object,
    IN U32 Flags,
    IN PVOID WaitContext)
{
    KeInitializeSpinlock(&Object->Lock);
    DListInitializeHead(&Object->WaiterBlockLinks);
    Object->State = 0;
    Object->Flags = Flags;
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

