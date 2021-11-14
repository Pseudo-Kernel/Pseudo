
#include <Windows.h>
#include "types.h"
#include "list.h"

#include "runner_q.h"

#include "sched_base.h"
#include "sched_normal.h"


//
// Scheduler class.
//


BOOLEAN
KiSchedInitialize(
    IN KSCHED_CLASS *Scheduler,
    IN PKSCHEDULER_INSERT_THREAD Insert,
    IN PKSCHEDULER_REMOVE_THREAD Remove,
    IN PKSCHEDULER_PEEK_THREAD Peek,
    IN PKSCHEDULER_NEXT_THREAD Next,
    IN PVOID SchedulerContext,
    IN U32 PriorityLevels)
{
    if (!PriorityLevels || PriorityLevels > RUNNER_QUEUE_MAX_LEVELS)
        return FALSE;

    KRUNNER_QUEUE *ReadyQueue = malloc(sizeof(KRUNNER_QUEUE));
    KRUNNER_QUEUE *IdleQueue = malloc(sizeof(KRUNNER_QUEUE));
    KRUNNER_QUEUE *ReadySwapQueue = malloc(sizeof(KRUNNER_QUEUE));

    BOOLEAN Result = FALSE;

    do
    {
        if (!ReadyQueue || !IdleQueue || !ReadySwapQueue)
            break;

        if (!E_IS_SUCCESS(KiRqInitialize(ReadyQueue, NULL, PriorityLevels)) ||
            !E_IS_SUCCESS(KiRqInitialize(IdleQueue, NULL, PriorityLevels)) ||
            !E_IS_SUCCESS(KiRqInitialize(ReadySwapQueue, NULL, PriorityLevels)))
            break;

        Result = TRUE;
    } while (0);

    if (!Result)
    {
        if (ReadyQueue)
            free(ReadyQueue);

        if (IdleQueue)
            free(IdleQueue);

        if (ReadySwapQueue)
            free(ReadySwapQueue);

        return FALSE;
    }


    memset(Scheduler, 0, sizeof(*Scheduler));

    Scheduler->Insert = Insert;
    Scheduler->Remove = Remove;
    Scheduler->Peek = Peek;
    Scheduler->Next = Next;
    Scheduler->SchedulerContext = SchedulerContext;
    Scheduler->PriorityLevels = PriorityLevels;
    Scheduler->Lock = 0;

    Scheduler->ReadyQueue = ReadyQueue;
    Scheduler->IdleQueue = IdleQueue;
    Scheduler->ReadySwapQueue = ReadySwapQueue;

    return TRUE;
}


BOOLEAN
KiSchedInsertThread(
    IN KSCHED_CLASS *Scheduler,
    IN KTHREAD *Thread,
    IN U32 Queue)
{
    return Scheduler->Insert(Scheduler, Thread, Scheduler->SchedulerContext, Queue, 0);
}

BOOLEAN
KiSchedRemoveThread(
    IN KSCHED_CLASS *Scheduler,
    IN KTHREAD *Thread)
{
    return Scheduler->Remove(Scheduler, Thread, Scheduler->SchedulerContext);
}

BOOLEAN
KiSchedPeekThread(
    IN KSCHED_CLASS *Scheduler,
    OUT KTHREAD **Thread,
    IN U32 Queue,
    IN U32 Flags)
{
    return Scheduler->Peek(Scheduler, Thread, Scheduler->SchedulerContext, Queue, Flags);
}

BOOLEAN
KiSchedNextThread(
    IN KSCHED_CLASS *Scheduler,
    OUT KTHREAD **Thread)
{
    return Scheduler->Next(Scheduler, Thread, Scheduler->SchedulerContext);
}


