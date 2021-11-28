
#pragma once

typedef struct _KSCHED_CLASS        KSCHED_CLASS;
typedef struct _KRUNNER_QUEUE       KRUNNER_QUEUE;
typedef struct _KTHREAD             KTHREAD;



#define RUNNER_QUEUE_MAX_LEVELS             64

typedef struct _KRUNNER_QUEUE
{
    ULONG Levels;
    DLIST_ENTRY ListHead[RUNNER_QUEUE_MAX_LEVELS];
    U64 QueuedState;    // Bitmap which represents queued state
    KSCHED_CLASS *SchedClass;
} KRUNNER_QUEUE;

//
// Runner queue.
//

ESTATUS
KiRqInitialize(
    IN KRUNNER_QUEUE *RunnerQueue,
    IN KSCHED_CLASS *SchedClass,
    IN ULONG Levels);


#define RQ_FLAG_INSERT_REMOVE_REVERSE_DIRECTION     0x00000001 // [I/R] insert/remove at reverse direction (insert/remove: listhead.next)
#define RQ_FLAG_NO_REMOVAL                          0x00000002 // [RO ] peek only, no removal

#define RQ_FLAG_NO_LEVEL                            0x00000100 // [RO ] ignores level and search all levels
#define RQ_FLAG_SEARCH_ASCENDING_ORDER              0x00000200 // [RO ] search 0..level


ESTATUS
KiRqEnqueue(
    IN KRUNNER_QUEUE *RunnerQueue,
    IN KTHREAD *Thread,
    IN ULONG Level,
    IN ULONG Flags);

ESTATUS
KiRqDequeue(
    IN KRUNNER_QUEUE *RunnerQueue,
    OUT KTHREAD **Thread,
    IN ULONG Level,
    IN ULONG Flags);

ESTATUS
KiRqRemove(
    IN KTHREAD *Thread);

BOOLEAN
KiRqIsEmpty(
    IN KRUNNER_QUEUE *RunnerQueue);

ESTATUS
KiRqSwap(
    IN KRUNNER_QUEUE *RunnerQueue1,
    IN KRUNNER_QUEUE *RunnerQueue2,
    IN ULONG Level,
    IN ULONG Flags);


