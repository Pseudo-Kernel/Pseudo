
#include <base/base.h>
#include <ke/thread.h>

VOID
KiInitializeThread(
    IN KTHREAD *Thread,
    IN U32 BasePriority,
    IN U64 ThreadId)
{
    memset(Thread, 0, sizeof(*Thread));

    DListInitializeHead(&Thread->WaiterList);

    DListInitializeHead(&Thread->RunnerLinks);
    Thread->BasePriority = BasePriority;
    Thread->Priority = BasePriority;
    Thread->ThreadId = ThreadId;
    Thread->RunnerQueue = NULL;
    Thread->State = ThreadStateInitialize;
    Thread->InWaiting = FALSE;
}

VOID
KiConsumeTimeslice(
    IN KTHREAD *Thread,
    IN U32 Timeslice,
    OUT BOOLEAN *Expired)
{
    Thread->RemainingTimeslices -= Timeslice;
    if (Thread->RemainingTimeslices <= 0)
    {
        //Thread->State = ThreadStateExpired;
        if (Expired)
            *Expired = TRUE;
    }
}

