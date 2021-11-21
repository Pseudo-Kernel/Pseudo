
#include <stdio.h>
#include <Windows.h>
#include "types.h"
#include "list.h"

#include "runner_q.h"

#include "sched_base.h"
#include "sched_normal.h"

#include "wait.h"

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
// console
//

#define CON_MAX_X				80
#define CON_MAX_Y				50

VOID ConSetPos(SHORT x, SHORT y)
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD Pos = { x, y };
	SetConsoleCursorPosition(hStdOut, Pos);
}

VOID ConClear()
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	COORD pos = { 0, 0 };
	DWORD dummy = 0;

	FillConsoleOutputAttribute(hStdOut, 0x07, CON_MAX_X * CON_MAX_Y, pos, &dummy);
	FillConsoleOutputCharacterA(hStdOut, ' ', CON_MAX_X * CON_MAX_Y, pos, &dummy);
}

VOID ConInit()
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize = sizeof(cfi);
	cfi.nFont = 0;
	cfi.FontFamily = FF_DONTCARE;
	cfi.FontWeight = FW_NORMAL;
	//cfi.dwFontSize.X = 0;
	//cfi.dwFontSize.Y = 16;
	//wcscpy_s(cfi.FaceName, L"Consolas");
	cfi.dwFontSize.X = 9;
	cfi.dwFontSize.Y = 16;
	wcscpy(cfi.FaceName, L"Terminal");
	SetCurrentConsoleFontEx(hStdOut, FALSE, &cfi);

	COORD size = { CON_MAX_X, CON_MAX_Y };
	SetConsoleScreenBufferSize(hStdOut, size);
	SetConsoleTextAttribute(hStdOut, 0x07);

	ConClear();

	//   SetWindowPos(GetConsoleWindow(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
}

//
// main
//

#define THREAD_COUNT                    (THREADS_PER_PROCESSOR * PROCESSOR_COUNT)
#define THREADS_PER_PROCESSOR           16
#define PROCESSOR_COUNT                 1
#define TIMESLICE_TO_MS                 1

KTHREAD g_Threads[THREAD_COUNT];
KPROCESSOR g_Processor[PROCESSOR_COUNT];
U64 g_GlobalCounter;

VOID ThreadStartEntry(KTHREAD *CurrentThread)
{
    for (;;)
    {
		CurrentThread->PrivateContext.Counter++;
		InterlockedIncrement64(&g_GlobalCounter);
    }
}


VOID VcpuContextRunnerThread(PVOID p)
{
    for (;;)
    {
    }
}

VOID VcpuDump()
{
	const int record_width = 50 + 3;
	int records_per_line = CON_MAX_X / record_width;
	const int base_x = 0;
	const int base_y = 4;

	int printed_count = 0;

	for (int i = 0; i < THREAD_COUNT; i++)
	{
		KTHREAD *Thread = &g_Threads[i];

		ConSetPos(
			base_x + (printed_count % records_per_line) * record_width,
			base_y + (printed_count / records_per_line));

		printf(
			"%2lld | p=%2d | %12lld | %5.02lf%% | ts=%7lld  ",
			Thread->ThreadId,
			Thread->Priority,
			Thread->PrivateContext.Counter,
			(double)Thread->PrivateContext.Counter * 100.0 / g_GlobalCounter,
			Thread->TimeslicesSpent);

		printed_count++;
	}

}

DWORD VcpuThread(KPROCESSOR *Processor)
{
    HANDLE Handle = Processor->Win32CtxThreadHandle;
    KTHREAD InitialThread;

    KiInitializeThread(&InitialThread, 0, -1);
    Processor->CurrentThread = &InitialThread;

    for (U64 i = 0;; i++)
    {
        Sleep(TIMESLICE_TO_MS);

		//if (!(i % 10))
		{
			SuspendThread(Handle);
			VcpuDump();
			ResumeThread(Handle);
		}

        BOOLEAN Expired = FALSE;
        KTHREAD *CurrentThread = Processor->CurrentThread;
        KiConsumeTimeslice(CurrentThread, 1, &Expired);

        if (Expired)
        {
            // Recalculate the timeslice and quantum by priority.
			U32 LastPriority = CurrentThread->Priority;
			U32 LastTimeslices = CurrentThread->CurrentTimeslices;

            CurrentThread->Priority = CurrentThread->BasePriority;
            CurrentThread->ThreadQuantum = CurrentThread->Priority >= 2 ? CurrentThread->Priority / 2 : 1;

			U32 CurrentTimeslices = (CurrentThread->Priority + 1) * 2; // PRIORITY_TO_TIMESLICES(LastPriority)
            CurrentThread->RemainingTimeslices += CurrentTimeslices;
			CurrentThread->TimeslicesSpent += LastTimeslices;
			CurrentThread->CurrentTimeslices = CurrentTimeslices;
			CurrentThread->ContextSwitchCount++;

            KASSERT(KiSchedInsertThread(Processor->NormalClass, CurrentThread, KSCHED_IDLE_QUEUE));

            KTHREAD *NextThread = NULL;

            KASSERT(KiSchedNextThread(Processor->NormalClass, &NextThread));

            if (1)
            {
				Processor->CurrentThread = NextThread;

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


#define TESTTHREADS             1024

HANDLE g_Event;
DWORD64 g_WaitEndTicks[TESTTHREADS];
DWORD64 g_TicksPerSeconds;

ULONG g_Counter;
ULONG g_EndCounter;

DWORD WINAPI DummyThread(ULONG i)
{
    InterlockedIncrement(&g_Counter);
    DWORD res = WaitForSingleObject(g_Event, INFINITE);
    QueryPerformanceCounter((LARGE_INTEGER *)&g_WaitEndTicks[i]);
    //g_WaitEndTicks[i] = GetTickCount64();
    KASSERT(res == WAIT_OBJECT_0);
    InterlockedIncrement(&g_EndCounter);

    while (g_EndCounter != TESTTHREADS)
    {
        _mm_pause();
    }

//    while (g_EndCounter != TESTTHREADS)
//        Sleep(1);

    return 0;
}

void test_windows_scheduler()
{
    //    SetProcessAffinityMask(GetCurrentProcess(), 4 | 16);

    QueryPerformanceFrequency((LARGE_INTEGER *)&g_TicksPerSeconds);

    for (int i = 0; i < 100; i++)
    {
        Sleep(1);
        printf("Tick = %llu\n", GetTickCount64());
    }

    Sleep(1000);

    g_Event = CreateEvent(NULL, TRUE, FALSE, NULL);

    HANDLE Threads[TESTTHREADS];

    for (int i = 0; i < TESTTHREADS; i++)
    {
        Threads[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DummyThread, (PVOID)i, CREATE_SUSPENDED, NULL);
        SetThreadPriority(Threads[i], IDLE_PRIORITY_CLASS);
        ResumeThread(Threads[i]);
    }

    while (g_Counter != TESTTHREADS)
        Sleep(1);

    SetEvent(g_Event);

    while (g_EndCounter != TESTTHREADS)
        Sleep(1);


    DWORD64 TickMin = g_WaitEndTicks[0];
    DWORD64 TickMax = g_WaitEndTicks[0];
    for (int i = 1; i < TESTTHREADS; i++)
    {
        if (TickMin > g_WaitEndTicks[i])
            TickMin = g_WaitEndTicks[i];
        if (TickMax < g_WaitEndTicks[i])
            TickMax = g_WaitEndTicks[i];
    }

    for (int i = 0; i < TESTTHREADS; i++)
    {
        printf("delta[%d] = %llu\n", i, g_WaitEndTicks[i] - TickMin);
    }

    printf("max-min = %llu\n", TickMax - TickMin);

    printf("ticks_per_sec = %llu\n", g_TicksPerSeconds);
}


BOOLEAN
TimerLookup(
    IN KTIMER_TABLE_LEAF *Leaf,
    IN U64 AbsoluteTime,
    IN U8 Index,
    IN PVOID LookupContext)
{
    printf("Leaf 0x%p, 0x%016llU\n", Leaf, AbsoluteTime);
    return TRUE;
}


int main()
{
    timeBeginPeriod(1);

	ConInit();

    KiW32FakeInitSystem();

    KTIMER Timer[32];
    for (int i = 0; i < 32; i++)
    {
        KiInitializeTimer(&Timer[i]);
    }

    for (int i = 0; i < 32; i++)
    {
        KiStartTimer(&Timer[i], TimerOneshot, i * 123 + 444);
    }

    extern KTIMER_LIST KiTimerList;

    KiLookupTimer(&KiTimerList, TimerLookup, NULL, 0, -1);


    Sleep(1);







    for (int i = 0; i < THREAD_COUNT; i++)
    {
		KTHREAD *Thread = &g_Threads[i];
		KiInitializeThread(Thread, (i * KSCHED_NORMAL_CLASS_LEVELS) / THREAD_COUNT, i + 1);
        KASSERT(VcpuSetInitialContext(Thread, ThreadStartEntry, Thread, 0));
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
		// Resume context runner first
		ResumeThread(ThreadCtxHandle[i]);
		ResumeThread(ThreadHandle[i]);
    }

    WaitForMultipleObjects(PROCESSOR_COUNT, ThreadHandle, TRUE, INFINITE);

    return 0;
}

