
#include <stdio.h>
#include <Windows.h>
#include "types.h"
#include "list.h"

#include "runner_q.h"

#include "sched_base.h"
#include "sched_normal.h"



typedef struct _KPROCESSOR
{
    KSCHED_CLASS *NormalClass;
    KTHREAD *CurrentThread;

    // Win32 test only
    HANDLE Win32CtxThreadHandle;
} KPROCESSOR;




//
// Processor.
//

VOID
KiInitializeProcessor(
    IN KPROCESSOR *Processor)
{
    KSCHED_CLASS *Class = (KSCHED_CLASS*)malloc(sizeof(KSCHED_CLASS));

    KASSERT(KiSchedInitialize(Class, &KiSchedNormalInsertThread, &KiSchedNormalRemoveThread, 
        &KiSchedNormalPeekThread, &KiSchedNormalNextThread, NULL, KSCHED_NORMAL_CLASS_LEVELS));

    memset(Processor, 0, sizeof(*Processor));
    Processor->NormalClass = Class;
}


//
// Thread.
//

VOID
KiInitializeThread(
    IN KTHREAD *Thread,
    IN U32 BasePriority,
    IN U32 ThreadId)
{
    memset(Thread, 0, sizeof(*Thread));

    DListInitializeHead(&Thread->RunnerLinks);
    Thread->BasePriority = BasePriority;
    Thread->ThreadId = ThreadId;
    Thread->RunnerQueue = NULL;
    Thread->State = ThreadStateInitialize;
}

VOID
KiConsumeTimeslice(
    IN KTHREAD *Thread,
    IN U32 Timeslice,
    OUT BOOLEAN *Expired)
{
    Thread->RemainingTimeslices -= Timeslice;
    if (Thread->RemainingTimeslices <= 0)
    {
        //Thread->State = ThreadStateExpired;
        if (Expired)
            *Expired = TRUE;
    }
}


//
// main
//

#define THREAD_COUNT                    (THREADS_PER_PROCESSOR * PROCESSOR_COUNT)
#define THREADS_PER_PROCESSOR           32
#define PROCESSOR_COUNT                 1

KTHREAD g_Threads[THREAD_COUNT];
KPROCESSOR g_Processor[PROCESSOR_COUNT];

VOID ThreadStartEntry(PVOID p)
{
    for (;;)
    {
    }
}


VOID VcpuContextRunnerThread(PVOID p)
{
    for (;;)
    {
    }
}

DWORD VcpuThread(KPROCESSOR *Processor)
{
    HANDLE Handle = Processor->Win32CtxThreadHandle;
    KTHREAD InitialThread;

    KiInitializeThread(&InitialThread, 0, -1);
    Processor->CurrentThread = &InitialThread;

    for (;;)
    {
        Sleep(10);

        BOOLEAN Expired = FALSE;
        KTHREAD *CurrentThread = Processor->CurrentThread;
        KiConsumeTimeslice(CurrentThread, 1, &Expired);

        if (Expired)
        {
            // Recalculate the timeslice and quantum by priority.
            CurrentThread->ThreadQuantum = CurrentThread->Priority >= 2 ? CurrentThread->Priority / 2 : 1;
            CurrentThread->RemainingTimeslices += CurrentThread->Priority;

            KTHREAD *NextThread = NULL;
            if (KiSchedNextThread(Processor->NormalClass, &NextThread))
            {
                //
                // 1. save current context to curr
                // 2. load context from next
                //

                KASSERT(SuspendThread(Handle) != (DWORD)(-1));
                CurrentThread->ThreadContext.ContextFlags = CONTEXT_ALL;
                KASSERT(GetThreadContext(Handle, &CurrentThread->ThreadContext));
                NextThread->ThreadContext.ContextFlags = CONTEXT_ALL;
                KASSERT(SetThreadContext(Handle, &NextThread->ThreadContext));
                KASSERT(ResumeThread(Handle) != (DWORD)(-1));
            }
        }
    }

    return 0;
}

BOOLEAN VcpuSetInitialContext(KTHREAD *Thread, VOID (*StartAddress)(VOID *), VOID *Param, U32 StackSize)
{
    if (!StackSize)
        StackSize = 0x80000;

    PVOID StackBase = VirtualAlloc(NULL, StackSize, MEM_COMMIT, PAGE_READWRITE);
    if (!StackBase)
        return FALSE;

//    task->stack_base = stack_base;
//    task->stack_size = stack_size;

#ifdef _X86_
    UPTR *Xsp = (UPTR *)((char *)StackBase + StackSize - 0x20);

    Xsp--;
    Xsp[0] = 0xdeadbeef; // set invalid return address
    Xsp[1] = (UPTR)Param; // push 1st param

    memset(&Thread->ThreadContext, 0, sizeof(Thread->ThreadContext));
    Thread->ThreadContext.ContextFlags = CONTEXT_ALL;// &~CONTEXT_SEGMENTS;
    Thread->ThreadContext.Esp = (UPTR)Xsp;
    Thread->ThreadContext.Eip = (UPTR)StartAddress;
#elif (defined _AMD64_)
    UPTR *Xsp = (UPTR *)((char *)StackBase + StackSize - 0x40);

    Xsp--;
    Xsp[0] = 0xfeedbaaddeadbeef; // set invalid return address

    memset(&Thread->ThreadContext, 0, sizeof(Thread->ThreadContext));
    Thread->ThreadContext.ContextFlags = CONTEXT_ALL;// &~CONTEXT_SEGMENTS;
    Thread->ThreadContext.Rsp = (UPTR)Xsp;
    Thread->ThreadContext.Rip = (UPTR)StartAddress;
    Thread->ThreadContext.Rcx = (UPTR)Param; // set 1st param

#else
#error Requires x86 or amd64!
#endif

    return TRUE;
}


void rq_test()
{
    // RQ test
    KRUNNER_QUEUE rq;
    KiRqInitialize(&rq, NULL, 1);
    for (int i = 0; i < 10; i++)
    {
        ESTATUS ret = KiRqEnqueue(&rq, &g_Threads[i], 0, 0);
        KASSERT(E_IS_SUCCESS(ret));
        printf("Enqueue %lld\n", g_Threads[i].ThreadId);
    }


    for (int i = 0; i < 10; i++)
    {
        KTHREAD *t = NULL;
        ESTATUS ret = KiRqDequeue(&rq, &t, 0, RQ_FLAG_NO_REMOVAL);
        KASSERT(E_IS_SUCCESS(ret));
        printf("Dequeue %lld\n", t->ThreadId);
    }

    Sleep(-1);
}

int main()
{
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        KiInitializeThread(&g_Threads[i], i / 10, i + 1);
        KASSERT(VcpuSetInitialContext(&g_Threads[i], ThreadStartEntry, NULL, 0));
    }

    for (int i = 0; i < PROCESSOR_COUNT; i++)
    {
        KPROCESSOR *Processor = &g_Processor[i];
        KiInitializeProcessor(Processor);
        for (int j = 0; j < THREADS_PER_PROCESSOR; j++)
        {
            KTHREAD *Thread = &g_Threads[j];
            KASSERT(KiSchedInsertThread(Processor->NormalClass, Thread, KSCHED_IDLE_QUEUE));
        }
    }

    HANDLE ThreadHandle[PROCESSOR_COUNT];
    HANDLE ThreadCtxHandle[PROCESSOR_COUNT];

    for (int i = 0; i < PROCESSOR_COUNT; i++)
    {
        ThreadHandle[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)VcpuThread,
            &g_Processor[i], CREATE_SUSPENDED, NULL);

        ThreadCtxHandle[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)VcpuContextRunnerThread,
            &g_Processor[i], CREATE_SUSPENDED, NULL);

        g_Processor[i].Win32CtxThreadHandle = ThreadCtxHandle[i];
    }

    Sleep(500);

    for (int i = 0; i < PROCESSOR_COUNT; i++)
    {
        ResumeThread(ThreadHandle[i]);
    }

    WaitForMultipleObjects(PROCESSOR_COUNT, ThreadHandle, TRUE, INFINITE);

    return 0;
}

