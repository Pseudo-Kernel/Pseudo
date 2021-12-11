
#pragma once

#include <ke/lock.h>

typedef struct _KSCHED_CLASS        KSCHED_CLASS;
typedef struct _KRUNNER_QUEUE       KRUNNER_QUEUE;
typedef struct _KPROCESS            KPROCESS;

typedef struct _PHYSICAL_ADDRESSES  PHYSICAL_ADDRESSES;

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

#pragma pack(push, 16)
typedef struct _KTHREAD_FXSAVE64
{
    U16 FCW;
    U16 FSW;
    U8 FTW;
    U8 Reserved1;
    U16 FOP;
    U64 FIP;
    U64 FDP;
    U32 MXCSR;
    U32 MXCSR_MASK;
    U8 ST0[10];
    U8 Reserved_ST0[6];
    U8 ST1[10];
    U8 Reserved_ST1[6];
    U8 ST2[10];
    U8 Reserved_ST2[6];
    U8 ST3[10];
    U8 Reserved_ST3[6];
    U8 ST4[10];
    U8 Reserved_ST4[6];
    U8 ST5[10];
    U8 Reserved_ST5[6];
    U8 ST6[10];
    U8 Reserved_ST6[6];
    U8 ST7[10];
    U8 Reserved_ST7[6];
    V128 XMM0;
    V128 XMM1;
    V128 XMM2;
    V128 XMM3;
    V128 XMM4;
    V128 XMM5;
    V128 XMM6;
    V128 XMM7;
    V128 XMM8;
    V128 XMM9;
    V128 XMM10;
    V128 XMM11;
    V128 XMM12;
    V128 XMM13;
    V128 XMM14;
    V128 XMM15;
    U8 Reserved2[16 * 3];
    U8 NotUsed[16 * 3];
} KTHREAD_FXSAVE64;

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

    // CR3 (Top-level paging structure pointer)
    U64 CR3;        // @todo: Remove this field. Use KTHTEAD.OwnerProcess->TopLevelPageTablePhysicalBase instead.
    U64 Padding1;

    KTHREAD_FXSAVE64 FXSTATE;
} KTHREAD_CONTEXT;
#pragma pack(pop)

#define THREAD_NAME_MAX_LENGTH      128

typedef
U64
(KERNELAPI *PKTHREAD_ROUTINE)(
    IN PVOID Argument);

typedef struct _KTHREAD
{
    KSPIN_LOCK Lock;

    DLIST_ENTRY ThreadList;
    DLIST_ENTRY ProcessThreadList;

    U64 ReferenceCount;

    //
    // Scheduler.
    //

//    KWAIT_HEADER WaitHeader;
    DLIST_ENTRY WaiterList;         // Used when thread is waiting objects

    DLIST_ENTRY RunnerLinks;
    U32 BasePriority;               // Base priority
    U32 Priority;                   // Dynamic priority

	U32 CurrentTimeslices;
    S32 RemainingTimeslices;
	U64 TimeslicesSpent;
    U32 ThreadQuantum;

    KRUNNER_QUEUE *RunnerQueue;
    U32 RunnerLevel;
    THREAD_STATE State;

    BOOLEAN InWaiting;              // Non-zero if thread is in wait state (waiting objects to be signaled)

    //
    // Statistics.
    //

	U64 ContextSwitchCount;

    //
    // Thread context.
    //

    KTHREAD_CONTEXT ThreadContext;

    //
    // Thread stack.
    //

    PHYSICAL_ADDRESSES *StackPaList;
    PVOID StackBase;    // Stack base. StackTop = (StackBase) + (StackSize) - sizeof(U64)
    SIZE_T StackSize;

    //
    // Owner.
    //

    KPROCESS *OwnerProcess;

    //
    // Startup parameters.
    //

    U64 ThreadId;
    CHAR Name[THREAD_NAME_MAX_LENGTH];
    PKTHREAD_ROUTINE StartRoutine;
    PVOID ThreadArgument;
} KTHREAD;

typedef struct _KSTACK_FRAME_INTERRUPT          KSTACK_FRAME_INTERRUPT;




extern DLIST_ENTRY KiThreadListHead;
extern KSPIN_LOCK KiThreadListLock;



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

ESTATUS
KiSetupInitialContextThread(
    IN KTHREAD *Thread,
    IN SIZE_T StackSize, 
    IN PVOID PML4Base);

KIRQL
KiLockThread(
    IN KTHREAD *Thread);

VOID
KiUnlockThread(
    IN KTHREAD *Thread,
    IN KIRQL PrevIrql);

KTHREAD *
KiAllocateThread(
    IN U32 BasePriority,
    IN U64 ThreadId,
    IN CHAR *ThreadName);

ESTATUS
KiInsertThread(
    IN KPROCESS *Process,
    IN KTHREAD *Thread);

KTHREAD *
KiCreateThread(
    IN U32 BasePriority,
    IN PKTHREAD_ROUTINE StartRoutine,
    IN PVOID ThreadArgument,
    IN CHAR *ThreadName);

VOID
KiDeleteThread(
    IN KTHREAD *Thread);

