
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

#pragma pack(push, 16)
typedef union _V128
{
    U64 u64_2[2];
    U32 u32_4[4];
    U16 u16_8[8];
    U8 u8_16[16];
} V128;
#pragma pack(pop)

#pragma pack(push, 16/*32*/)
typedef union _V256
{
    V128 v128_2[2];
    U64 u64_4[4];
    U32 u32_8[8];
    U16 u16_16[16];
    U8 u8_32[32];
} V256;
#pragma pack(pop)


typedef struct _KTHREAD_CONTEXT
{
    // GPRs (R0 - R15)
    U64 R15;
    U64 R14;
    U64 R13;
    U64 R12;
    U64 R11;
    U64 R10;
    U64 R9;
    U64 R8;
    U64 Rdi;
    U64 Rsi;
    U64 Rbp;
    U64 Rsp;
    U64 Rbx;
    U64 Rdx;
    U64 Rcx;
    U64 Rax;

    // SREGS
    U64 Ss;
    U64 Gs;
    U64 Fs;
    U64 Es;
    U64 Ds;
    U64 Cs;

    // IP
    U64 Rip;

    // FLAGS
    U64 Rflags;

    // CRx (CR0, 2, 3, 4, 8)
    U64 CR0;
    U64 CR2;
    U64 CR3;
    U64 CR4;
    U64 CR8;
} KTHREAD_CONTEXT;

#define THREAD_NAME_MAX_LENGTH      128

typedef
U64
(KERNELAPI *PKTHREAD_ROUTINE)(
    IN PVOID Argument);

typedef struct _KTHREAD
{
//    KWAIT_HEADER WaitHeader;
    DLIST_ENTRY WaiterList;         // Used when thread is waiting objects

    DLIST_ENTRY RunnerLinks;
    KTHREAD_CONTEXT ThreadContext;
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

    BOOLEAN InWaiting;              // Non-zero if thread is in wait state (waiting objects to be signaled)
    CHAR Name[THREAD_NAME_MAX_LENGTH];

    PKTHREAD_ROUTINE StartRoutine;
    PVOID ThreadArgument;
} KTHREAD;

typedef struct _KSTACK_FRAME_INTERRUPT          KSTACK_FRAME_INTERRUPT;


KTHREAD *
KeGetCurrentThread(
    VOID);

VOID
KiInitializeThread(
    IN KTHREAD *Thread,
    IN U32 BasePriority,
    IN U64 ThreadId,
    IN CHAR *ThreadName);

VOID
KiLoadContextToFrame(
    IN KSTACK_FRAME_INTERRUPT *InterruptFrame,
    IN KTHREAD_CONTEXT *Context);

VOID
KiLoadFrameToContext(
    IN KSTACK_FRAME_INTERRUPT *InterruptFrame,
    IN KTHREAD_CONTEXT *Context);

