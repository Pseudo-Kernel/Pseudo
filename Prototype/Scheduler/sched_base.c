
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

    memset(Scheduler, 0, sizeof(*Scheduler));
    Scheduler->Insert = Insert;
    Scheduler->Remove = Remove;
    Scheduler->Peek = Peek;
    Scheduler->Next = Next;
    Scheduler->SchedulerContext = SchedulerContext;
    Scheduler->PriorityLevels = PriorityLevels;
    Scheduler->Lock = 0;

    return TRUE;
}


