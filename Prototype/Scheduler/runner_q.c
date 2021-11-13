
#include <Windows.h>
#include "types.h"
#include "list.h"

#include "runner_q.h"


//
// Runner queue.
//

ESTATUS
KiRqInitialize(
    IN KRUNNER_QUEUE *RunnerQueue,
    IN KSCHED_CLASS *SchedClass,
    IN ULONG Levels)
{
    if (Levels > RUNNER_QUEUE_MAX_LEVELS)
        return E_INVALID_PARAMETER;

    RunnerQueue->Levels = Levels;

    for (ULONG i = 0; i < Levels; i++)
        DListInitializeHead(&RunnerQueue->ListHead[i]);

    RunnerQueue->SchedClass = SchedClass;
    RunnerQueue->QueuedState = 0;

    return E_SUCCESS;
}

ESTATUS
KiRqEnqueue(
    IN KRUNNER_QUEUE *RunnerQueue,
    IN KTHREAD *Thread,
    IN ULONG Level,
    IN ULONG Flags)
{
    if (Level >= RunnerQueue->Levels || 
        RunnerQueue->Levels > RUNNER_QUEUE_MAX_LEVELS)
        return E_INVALID_PARAMETER;

    if ((Flags & RQ_FLAG_NO_REMOVAL) || 
        (Flags & RQ_FLAG_NO_LEVEL))
        return E_INVALID_PARAMETER;

    if (Flags & RO_FLAG_INSERT_REMOVE_REVERSE_DIRECTION)
    {
        DListInsertAfter(&RunnerQueue->ListHead[Level], &Thread->RunnerLinks);
    }
    else
    {
        DListInsertBefore(&RunnerQueue->ListHead[Level], &Thread->RunnerLinks);
    }

    RunnerQueue->QueuedState |= (1ULL << Level);

    Thread->RunnerQueue = RunnerQueue;
    Thread->RunnerLevel = Level;

    return E_SUCCESS;
}

ESTATUS
KiRqDequeue(
    IN KRUNNER_QUEUE *RunnerQueue,
    OUT KTHREAD **Thread,
    IN ULONG Level,
    IN ULONG Flags)
{
    if (Level >= RunnerQueue->Levels ||
        RunnerQueue->Levels > RUNNER_QUEUE_MAX_LEVELS)
        return E_INVALID_PARAMETER;

    INT TargetLevel = -1;
    U64 QueuedState = RunnerQueue->QueuedState;

    if (Flags & RQ_FLAG_NO_LEVEL)
    {
        if (Flags & RQ_FLAG_SEARCH_ASCENDING_ORDER)
        {
            // 0..(maxlevel-1)
            for (INT i = 0; i < (INT)RunnerQueue->Levels; i++)
            {
                if (QueuedState & (1ULL << i))
                {
                    TargetLevel = i;
                    break;
                }
            }
        }
        else
        {
            // (maxlevel-1)..0
            for (INT i = RunnerQueue->Levels - 1; i >= 0; i--)
            {
                if (QueuedState & (1ULL << i))
                {
                    TargetLevel = i;
                    break;
                }
            }
        }
    }

    if (TargetLevel < 0)
    {
        return E_LOOKUP_FAILED;
    }

    DLIST_ENTRY *ListHead = &RunnerQueue->ListHead[TargetLevel];
    DLIST_ENTRY *Link = NULL;

    if (Flags & RO_FLAG_INSERT_REMOVE_REVERSE_DIRECTION)
    {
        Link = ListHead->Next;
    }
    else
    {
        Link = ListHead->Prev;
    }

    KTHREAD *TargetThread = CONTAINING_RECORD(Link, KTHREAD, RunnerLinks);

    if (!(Flags & RQ_FLAG_NO_REMOVAL))
    {
        DListRemoveEntry(Link);
        if (DListIsEmpty(ListHead))
        {
            RunnerQueue->QueuedState &= ~(1ULL << TargetLevel);
        }

        TargetThread->RunnerQueue = NULL;
        TargetThread->RunnerLevel = -1;
    }

    *Thread = TargetThread;

    return E_SUCCESS;
}

ESTATUS
KiRqRemove(
    IN KTHREAD *Thread)
{
    KRUNNER_QUEUE *RunnerQueue = Thread->RunnerQueue;
    U32 Level = Thread->RunnerLevel;

    if (Level >= RunnerQueue->Levels ||
        RunnerQueue->Levels > RUNNER_QUEUE_MAX_LEVELS)
        return E_INVALID_PARAMETER;

    DLIST_ENTRY *ListHead = &RunnerQueue->ListHead[Level];

    DListRemoveEntry(&Thread->RunnerLinks);
    if (DListIsEmpty(ListHead))
    {
        RunnerQueue->QueuedState &= ~(1ULL << Level);
    }

    Thread->RunnerQueue = NULL;
    Thread->RunnerLevel = -1;

    return E_SUCCESS;
}

ESTATUS
KiRqSwap(
    IN KRUNNER_QUEUE *RunnerQueue1,
    IN KRUNNER_QUEUE *RunnerQueue2,
    IN ULONG Level,
    IN ULONG Flags)
{
    if (Level >= RunnerQueue1->Levels ||
        RunnerQueue1->Levels > RUNNER_QUEUE_MAX_LEVELS)
        return E_INVALID_PARAMETER;

    if (Level >= RunnerQueue2->Levels ||
        RunnerQueue2->Levels > RUNNER_QUEUE_MAX_LEVELS)
        return E_INVALID_PARAMETER;

    if ((Flags & RQ_FLAG_SEARCH_ASCENDING_ORDER) ||
        (Flags & RQ_FLAG_NO_REMOVAL) ||
        (Flags & RO_FLAG_INSERT_REMOVE_REVERSE_DIRECTION))
        return E_INVALID_PARAMETER;

    U64 TempState = 0;
    DLIST_ENTRY TempListHead1, TempListHead2;
    DListInitializeHead(&TempListHead1);
    DListInitializeHead(&TempListHead2);

    if (Flags & RQ_FLAG_NO_LEVEL)
    {
        if (RunnerQueue1->Levels != RunnerQueue2->Levels)
        {
            return E_INVALID_PARAMETER;
        }

        // 0..(maxlevel-1)
        for (INT i = 0; i < (INT)RunnerQueue1->Levels; i++)
        {
            DListMoveAfter(&TempListHead1, &RunnerQueue1->ListHead[i]);
            DListMoveAfter(&TempListHead2, &RunnerQueue2->ListHead[i]);
            DListMoveAfter(&RunnerQueue1->ListHead[i], &TempListHead2);
            DListMoveAfter(&RunnerQueue2->ListHead[i], &TempListHead1);
        }

        TempState = RunnerQueue1->QueuedState;
        RunnerQueue1->QueuedState = RunnerQueue2->QueuedState;
        RunnerQueue2->QueuedState = TempState;
    }
    else
    {
        DListMoveAfter(&TempListHead1, &RunnerQueue1->ListHead[Level]);
        DListMoveAfter(&TempListHead2, &RunnerQueue2->ListHead[Level]);
        DListMoveAfter(&RunnerQueue1->ListHead[Level], &TempListHead2);
        DListMoveAfter(&RunnerQueue2->ListHead[Level], &TempListHead1);

        U64 Bit = 1ULL << Level;
        U64 Temp1 = (RunnerQueue1->QueuedState & Bit);
        U64 Temp2 = (RunnerQueue2->QueuedState & Bit);

        RunnerQueue1->QueuedState &= ~Bit;
        RunnerQueue2->QueuedState &= ~Bit;

        RunnerQueue1->QueuedState |= Temp2;
        RunnerQueue2->QueuedState |= Temp1;
    }

    return E_SUCCESS;
}


