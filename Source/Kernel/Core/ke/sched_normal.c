
/**
 * @file sched_normal.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements normal scheduler class.
 * @version 0.1
 * @date 2021-11-29
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <base/base.h>
#include <ke/thread.h>
#include <ke/sched_normal.h>

U32 KiSchedNormalClassTimeslices[KSCHED_NORMAL_CLASS_LEVELS] =
{
    1, 2, 3, 4, 5, 6, 7, 8,
    9, 10, 11, 12, 13, 14, 15
};

U32 KiSchedNormalClassQuantum[KSCHED_NORMAL_CLASS_LEVELS] =
{
    1, 2, 2, 4, 4, 4, 4, 8,
    8, 8, 8, 12, 12, 12, 12, 15
};

//
// Scheduler class.
//

BOOLEAN
KiSchedNormalInsertThread(
    IN KSCHED_CLASS *This,
    IN KTHREAD *Thread,
    IN PVOID SchedulerContext,
    IN U32 Queue,
    IN U32 Flags)
{
    if (Queue != KSCHED_IDLE_QUEUE &&
        Queue != KSCHED_READY_QUEUE &&
        Queue != KSCHED_READY_SWAP_QUEUE)
        return FALSE;

    U32 QueueFlags = 0;

    if (Flags & KSCHED_INSERT_REMOVE_REVERSE)
        QueueFlags |= RQ_FLAG_INSERT_REMOVE_REVERSE_DIRECTION;

    KRUNNER_QUEUE *RunnerQueue = NULL;

    switch (Queue)
    {
    case KSCHED_IDLE_QUEUE:
        RunnerQueue = This->IdleQueue;
        break;

    case KSCHED_READY_QUEUE:
        RunnerQueue = This->ReadyQueue;
        break;

    case KSCHED_READY_SWAP_QUEUE:
        RunnerQueue = This->ReadySwapQueue;
        break;

    default:
        DASSERT(FALSE);
    }

    ESTATUS Status = KiRqEnqueue(RunnerQueue, Thread, Thread->Priority, QueueFlags);

    return E_IS_SUCCESS(Status);
}

BOOLEAN
KiSchedNormalRemoveThread(
    IN KSCHED_CLASS *This,
    IN KTHREAD *Thread,
    IN PVOID SchedulerContext)
{
    if (!E_IS_SUCCESS(KiRqRemove(Thread)))
    {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
KiSchedNormalPeekThread(
    IN KSCHED_CLASS *This,
    OUT KTHREAD **Thread,
    IN PVOID SchedulerContext,
    IN U32 Queue,
    IN U32 Flags)
{
    if (Queue != KSCHED_IDLE_QUEUE &&
        Queue != KSCHED_READY_QUEUE &&
        Queue != KSCHED_READY_SWAP_QUEUE)
        return FALSE;

    U32 QueueFlags = 0;

    if (Flags & KSCHED_INSERT_REMOVE_REVERSE)
        QueueFlags |= RQ_FLAG_INSERT_REMOVE_REVERSE_DIRECTION;

    KRUNNER_QUEUE *RunnerQueue = NULL;

    switch (Queue)
    {
    case KSCHED_IDLE_QUEUE:
        RunnerQueue = This->IdleQueue;
        break;

    case KSCHED_READY_QUEUE:
        RunnerQueue = This->ReadyQueue;
        break;

    case KSCHED_READY_SWAP_QUEUE:
        RunnerQueue = This->ReadySwapQueue;
        break;

    default:
        DASSERT(FALSE);
    }

    ESTATUS Status = KiRqDequeue(RunnerQueue, Thread, 0, QueueFlags | RQ_FLAG_NO_LEVEL | RQ_FLAG_NO_REMOVAL);

    return E_IS_SUCCESS(Status);
}

BOOLEAN
KiSchedNormalNextThread(
    IN KSCHED_CLASS *This,
    OUT KTHREAD **Thread,
    IN PVOID SchedulerContext)
{
    ESTATUS Status = KiRqDequeue(This->ReadyQueue, Thread, 0, RQ_FLAG_NO_LEVEL);

    if (!E_IS_SUCCESS(Status))
    {
        DASSERT(KiRqIsEmpty(This->ReadyQueue));

        if (!KiRqIsEmpty(This->ReadySwapQueue))
        {
            // thread exists in swap queue.
            KRUNNER_QUEUE *TempQueue = This->ReadySwapQueue;
            This->ReadySwapQueue = This->ReadyQueue;
            This->ReadyQueue = TempQueue;
        }
        else if (!KiRqIsEmpty(This->IdleQueue))
        {
            KRUNNER_QUEUE *TempQueue = This->IdleQueue;
            This->IdleQueue = This->ReadyQueue;
            This->ReadyQueue = TempQueue;
        }

        Status = KiRqDequeue(This->ReadyQueue, Thread, 0, RQ_FLAG_NO_LEVEL);
    }

    return E_IS_SUCCESS(Status);
}


