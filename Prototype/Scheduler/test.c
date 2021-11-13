
#include <Windows.h>
#include "types.h"
#include "list.h"

#include "runner_q.h"

#include "sched_base.h"
#include "sched_normal.h"



typedef struct _KPROCESSOR
{
    KSCHED_CLASS *NormalClass;

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

    memset(Processor, 0, sizeof(*Processor));
    Processor->NormalClass = Class;

    KASSERT(KiSchedInitialize(Class, NULL, NULL, NULL, NULL, NULL, SCHED_NORMAL_CLASS_LEVELS));
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
}




//
// main
//

#define THREAD_COUNT                    64
#define PROCESSOR_COUNT                 1

KTHREAD g_Threads[THREAD_COUNT];
KPROCESSOR g_Processor[PROCESSOR_COUNT];

VOID VcpuContextRunnerThread(PVOID p)
{
    for (;;)
    {
    }
}

DWORD VcpuThread(KPROCESSOR *Processor)
{
    HANDLE Handle = Processor->Win32CtxThreadHandle;

    for (;;)
    {
        Sleep(10);

        //
        // 1. save current context to curr
        // 2. load context from next
        //

#if 0
        DASSERT(SuspendThread(Handle) != (DWORD)(-1));
        curr->context.ContextFlags = CONTEXT_ALL;
        DASSERT(GetThreadContext(Handle, &curr->context));
        next->context.ContextFlags = CONTEXT_ALL;
        DASSERT(SetThreadContext(Handle, &next->context));
        DASSERT(ResumeThread(Handle) != (DWORD)(-1));
#endif
    }

    return 0;
}

int main()
{
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        KiInitializeThread(&g_Threads[i], i / 10, i + 1);
    }

    for (int i = 0; i < PROCESSOR_COUNT; i++)
    {
        KiInitializeProcessor(&g_Processor[i]);
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

