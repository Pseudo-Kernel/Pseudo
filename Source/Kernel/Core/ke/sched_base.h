
#pragma once
#include <ke/runner_q.h>


#define KSCHED_IDLE_QUEUE                   0
#define KSCHED_READY_QUEUE                  1
#define KSCHED_READY_SWAP_QUEUE             2

#define KSCHED_INSERT_REMOVE_REVERSE        0x00000001

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
    U32 Lock;
    U32 PriorityLevels;

    PKSCHEDULER_INSERT_THREAD Insert;
    PKSCHEDULER_REMOVE_THREAD Remove;
    PKSCHEDULER_PEEK_THREAD Peek;
    PKSCHEDULER_NEXT_THREAD Next;

    PVOID SchedulerContext;

    KRUNNER_QUEUE *IdleQueue;
    KRUNNER_QUEUE *ReadyQueue;
    KRUNNER_QUEUE *ReadySwapQueue;

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




BOOLEAN
KiSchedInsertThread(
    IN KSCHED_CLASS *Scheduler,
    IN KTHREAD *Thread,
    IN U32 Queue);

BOOLEAN
KiSchedRemoveThread(
    IN KSCHED_CLASS *Scheduler,
    IN KTHREAD *Thread);

BOOLEAN
KiSchedPeekThread(
    IN KSCHED_CLASS *Scheduler,
    OUT KTHREAD **Thread,
    IN U32 Queue,
    IN U32 Flags);

BOOLEAN
KiSchedNextThread(
    IN KSCHED_CLASS *Scheduler,
    OUT KTHREAD **Thread);


