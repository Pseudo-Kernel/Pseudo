
#pragma once
#include "runner_q.h"


#define KSCHED_WAIT_QUEUE                   0
#define KSCHED_READY_QUEUE                  1
#define KSCHED_READY_SWAP_QUEUE             2

//#define KSCHED_INSERT_REAR                  0x00000001

typedef
BOOLEAN
(*PKSCHEDULER_INSERT_THREAD)(
    IN KSCHED_CLASS *This,
    IN KTHREAD *Thread,
    IN PVOID SchedulerContext,
    IN U32 Queue,
    IN U32 Flags);

typedef
BOOLEAN
(*PKSCHEDULER_REMOVE_THREAD)(
    IN KSCHED_CLASS *This,
    IN KTHREAD *Thread,
    IN PVOID SchedulerContext);

typedef
BOOLEAN
(*PKSCHEDULER_PEEK_THREAD)(
    IN KSCHED_CLASS *This,
    OUT KTHREAD **Thread,
    IN PVOID SchedulerContext,
    IN U32 Queue,
    IN U32 Flags);

typedef
BOOLEAN
(*PKSCHEDULER_NEXT_THREAD)(
    IN KSCHED_CLASS *This,
    OUT KTHREAD **Thread,
    IN PVOID SchedulerContext);

typedef struct _KSCHED_CLASS
{
    PKSCHEDULER_INSERT_THREAD Insert;
    PKSCHEDULER_REMOVE_THREAD Remove;
    PKSCHEDULER_PEEK_THREAD Peek;
    PKSCHEDULER_NEXT_THREAD Next;

    U32 Lock;

    PVOID SchedulerContext;
    U32 PriorityLevels;
    U32 Body[1];
} KSCHED_CLASS;



BOOLEAN
KiSchedInitialize(
    IN KSCHED_CLASS *Scheduler,
    IN PKSCHEDULER_INSERT_THREAD Insert,
    IN PKSCHEDULER_REMOVE_THREAD Remove,
    IN PKSCHEDULER_PEEK_THREAD Peek,
    IN PKSCHEDULER_NEXT_THREAD Next,
    IN PVOID SchedulerContext,
    IN U32 PriorityLevels);

