
#pragma once

typedef struct _KSCHED_CLASS        KSCHED_CLASS;
typedef struct _KRUNNER_QUEUE       KRUNNER_QUEUE;

typedef enum _THREAD_STATE
{
    ThreadStateInitialize,  // Thread is initialized
    ThreadStateWait,        // In sleep state (queued in WaitListHead)
    ThreadStateReady,       // Ready to run (queued in ReadyListHead)
    ThreadStateRunning,     // Running
    ThreadStateExpired,     // Expired (remaining timeslices is zero, queued in IdleListHead)
} THREAD_STATE;

typedef struct _PRIVATE_CONTEXT
{
	U64 Counter;
    U32 ProcessorId;
} PRIVATE_CONTEXT;

typedef struct _KTHREAD
{
//    KWAIT_HEADER WaitHeader;
    DLIST_ENTRY WaiterList;         // Used when waiting objects to be signaled

    DLIST_ENTRY RunnerLinks;
    CONTEXT ThreadContext;
    U32 BasePriority;               // Base priority
    U32 Priority;                   // Dynamic priority

	U32 CurrentTimeslices;
    S32 RemainingTimeslices;
    U32 ThreadQuantum;
    U64 ThreadId;

    KRUNNER_QUEUE *RunnerQueue;
    U32 RunnerLevel;
    THREAD_STATE State;
	U64 ContextSwitchCount;
	U64 TimeslicesSpent;

	PRIVATE_CONTEXT PrivateContext;
} KTHREAD;



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


