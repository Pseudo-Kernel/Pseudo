
#include <Windows.h>
#include "types.h"
#include "list.h"

#include "runner_q.h"

#include "sched_base.h"
#include "sched_normal.h"


U32 KiSchedNormalThreadTimeslices[] =
{
    1, 2, 3, 4, 5, 6, 7, 8,
    9, 10, 11, 12, 13, 14, 15
};

U32 KiSchedNormalThreadQuantum[] =
{
    1, 2, 2, 4, 4, 4, 4, 8,
    8, 8, 8, 12, 12, 12, 12, 15
};

//
// Scheduler class.
//

BOOLEAN
KiSchedInsertThread(
    IN KSCHED_CLASS *This,
    IN KTHREAD *Thread,
    IN PVOID SchedulerContext,
    IN U32 Queue,
    IN U32 Flags)
{
    KSCHED_CONTEXT_NORMAL *Context = (KSCHED_CONTEXT_NORMAL *)SchedulerContext;

//    Thread->Priority = 0;
//    Thread->SchedClass
//
//    KiRqEnqueue(&Context->ReadyQueue, 

    return FALSE;
}

BOOLEAN
KiSchedRemoveThread(
    IN KSCHED_CLASS *This,
    IN KTHREAD *Thread,
    IN PVOID SchedulerContext)
{
    KSCHED_CONTEXT_NORMAL *Context = (KSCHED_CONTEXT_NORMAL *)SchedulerContext;

    if (!E_IS_SUCCESS(KiRqRemove(Thread)))
    {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
KiSchedPeekThread(
    IN KSCHED_CLASS *This,
    OUT KTHREAD **Thread,
    IN PVOID SchedulerContext,
    IN U32 Queue,
    IN U32 Flags)
{
    KSCHED_CONTEXT_NORMAL *Context = (KSCHED_CONTEXT_NORMAL *)SchedulerContext;

    return FALSE;
}

BOOLEAN
KiSchedNextThread(
    IN KSCHED_CLASS *This,
    OUT KTHREAD **Thread,
    IN PVOID SchedulerContext)
{
    KSCHED_CONTEXT_NORMAL *Context = (KSCHED_CONTEXT_NORMAL *)SchedulerContext;

    return FALSE;
}

