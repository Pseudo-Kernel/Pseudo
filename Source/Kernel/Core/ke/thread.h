
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
    U8 Placeholders[512];
    // RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP
    // R8, R9, R10, R11, R12, R13, R14, R15
    // RIP, RFLAGS
//    U64 GPR[16]; // r0..r15
//    U64 PSW;
//    U64 IP;
//    U32 SEGR[6]; // CS, DS, ES, SS, FS, GS
//    V256 VR[16]; // ymm0..ymm16
} KTHREAD_CONTEXT;

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
} KTHREAD;


