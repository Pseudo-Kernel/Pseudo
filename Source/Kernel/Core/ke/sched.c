
/**
 * @file sched.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements scheduler related routines.
 * @version 0.1
 * @date 2021-12-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <base/base.h>
#include <init/bootgfx.h>
#include <ke/lock.h>
#include <mm/mm.h>
#include <mm/pool.h>
#include <ke/inthandler.h>
#include <ke/interrupt.h>
#include <ke/kprocessor.h>
#include <ke/thread.h>
#include <ke/process.h>
#include <ke/sched.h>

U64
KiTestSystemThreadStart(
    IN PVOID Argument)
{
    volatile U64 Counter = 0;
    KPROCESSOR *Processor = KeGetCurrentProcessor();
    KTHREAD *Thread = KeGetCurrentThread();

    //
    // Test!
    //

    static U32 test1[4] = { 0x33333333, 0x33333333, 0x33333333, 0x33333333 };
    static U32 test2[4] = { 0x44444444, 0x44444444, 0x44444444, 0x44444444 };
    volatile U32 test_r1[4];
    volatile U32 test_r2[4];

    __asm__ __volatile__ (
        "movdqu xmm0, xmmword ptr [%0]\n\t"
        "movdqu xmm1, xmmword ptr [%1]\n\t"
        :
        : "r"(test1), "r"(test2)
        : "memory"
    );


    for (;;)
    {
        // Test!
        DASSERT(Processor->CurrentThread == Thread);

        __asm__ __volatile__ (
            "movdqa xmm2, xmm0\n\t"
            "movdqa xmm0, xmm1\n\t"
            "movdqa xmm1, xmm2\n\t"
            "movdqu xmmword ptr [%0], xmm0\n\t"
            "movdqu xmmword ptr [%1], xmm1\n\t"
            : "=m"(test_r1), "=m"(test_r2)
            : 
            : "memory"
        );

        if (Counter & 1)
        {
            DASSERT(
                !memcmp((void *)test_r1, (void *)test1, 16) && 
                !memcmp((void *)test_r2, (void *)test2, 16));
        }
        else
        {
            DASSERT(
                !memcmp((void *)test_r1, (void *)test2, 16) && 
                !memcmp((void *)test_r2, (void *)test1, 16));
        }

        Counter++;
    }
}

VOID
KiProcessorSchedInitialize(
    VOID)
{
    KPROCESSOR *Processor = KeGetCurrentProcessor();

    // 
    // Create scheduler class object.
    // 

    KSCHED_CLASS *NormalClass = (KSCHED_CLASS *)MmAllocatePool(PoolTypeNonPaged, sizeof(KSCHED_CLASS), 0x10, 0);
    if (!NormalClass)
    {
        FATAL("Failed to allocate scheduler object");
    }

    DASSERT(KiSchedInitialize(NormalClass, 
        &KiSchedNormalInsertThread, &KiSchedNormalRemoveThread, 
        &KiSchedNormalPeekThread, &KiSchedNormalNextThread, NULL, KSCHED_NORMAL_CLASS_LEVELS));

    //
    // Create idle thread.
    //

    KTHREAD *IdleThread = KiCreateThread(0, NULL, NULL, "Idle");
    if (!IdleThread)
    {
        FATAL("Failed to allocate thread object");
    }

    DASSERT(E_IS_SUCCESS(KiInsertThread(&KiIdleProcess, IdleThread)));

    Processor->CurrentThread = IdleThread;
    Processor->SchedNormalClass = NormalClass;



    //
    // Test!!!
    //

    KTHREAD *TestThread = KiCreateThread(1, (PKTHREAD_ROUTINE)&KiTestSystemThreadStart, NULL, "Test");
    DASSERT(TestThread);
    DASSERT(E_IS_SUCCESS(KiSetupInitialContextThread(TestThread, 0, (PVOID)__readcr3())));

    DASSERT(E_IS_SUCCESS(KiInsertThread(&KiSystemProcess, TestThread)));

    DASSERT(KiSchedInsertThread(KeGetCurrentProcessor()->SchedNormalClass, TestThread, KSCHED_READY_QUEUE));
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

ESTATUS
KiScheduleNextThread(
    IN KSTACK_FRAME_INTERRUPT *InterruptFrame,
    OUT KTHREAD **PreviousThread,
    OUT KTHREAD **SwitchedThread)
{
    DASSERT(KeGetCurrentIrql() == IRQL_CONTEXT_SWITCH);

    // @todo: consume timeslice of current thread

    KPROCESSOR *Processor = KeGetCurrentProcessor();
    KTHREAD *CurrentThread = Processor->CurrentThread;

    KTHREAD *NextThread = NULL;

    CurrentThread->ContextSwitchCount++;

    DASSERT(KiSchedInsertThread(Processor->SchedNormalClass, CurrentThread, KSCHED_IDLE_QUEUE));
    DASSERT(KiSchedNextThread(Processor->SchedNormalClass, &NextThread));
    DASSERT(NextThread);

    Processor->CurrentThread = NextThread;

    // We don't need to save/load context from same thread. (especially CR3!)
    if (CurrentThread == NextThread)
    {
        return E_NOT_PERFORMED;
    }

    // Save current thread context.
    KiLoadFrameToContext(InterruptFrame, &CurrentThread->ThreadContext);
    CurrentThread->ThreadContext.CR3 = __readcr3();

    // Check whether the SSE instruction was executed during thread quantum.
    if (!(__readcr0() & ARCH_X64_CR0_TS))
    {
        // Save SSE state as this thread uses SSE instructions
        _fxsave64(&CurrentThread->ThreadContext.FXSTATE);

        // Set CR0.TS to catch next #NM
        __writecr0(__readcr0() | ARCH_X64_CR0_TS);
    }

    // Load context from next thread.
    KiLoadContextToFrame(InterruptFrame, &NextThread->ThreadContext);

    // Reload CR3 only if needed.
    // Note that writing the CR3 flushes the TLB.
    if (__readcr3() != NextThread->ThreadContext.CR3)
    {
        __writecr3(NextThread->ThreadContext.CR3);
    }

    // We don't have to restore SSE state here
    // We'll restore SSE state in #NM handler

    if (PreviousThread)
    {
        *PreviousThread = CurrentThread;
    }

    if (SwitchedThread)
    {
        *SwitchedThread = NextThread;
    }

    return E_SUCCESS;
}

ESTATUS
KiScheduleSwitchContext(
    IN KSTACK_FRAME_INTERRUPT *InterruptFrame)
{
    DASSERT(KeGetCurrentIrql() == IRQL_CONTEXT_SWITCH);

    BOOLEAN Expired = FALSE;
    KPROCESSOR *Processor = KeGetCurrentProcessor();
    KTHREAD *CurrentThread = Processor->CurrentThread;

    DASSERT(CurrentThread);
    
    KiConsumeTimeslice(CurrentThread, 1, &Expired);

    if (!Expired)
    {
        return E_NOT_PERFORMED;
    }

    // Recalculate the timeslice and quantum by priority.
    U32 LastTimeslices = CurrentThread->CurrentTimeslices;

    CurrentThread->Priority = CurrentThread->BasePriority;
    CurrentThread->ThreadQuantum = // PRIORITY_TO_THREAD_QUANTUM(LastPriority)
        CurrentThread->Priority >= 2 ? CurrentThread->Priority / 2 : 1;

    U32 CurrentTimeslices = (CurrentThread->Priority + 1) * 2; // PRIORITY_TO_TIMESLICES(LastPriority)
    CurrentThread->RemainingTimeslices += CurrentTimeslices;
    CurrentThread->TimeslicesSpent += LastTimeslices;
    CurrentThread->CurrentTimeslices = CurrentTimeslices;

    return KiScheduleNextThread(InterruptFrame, NULL, NULL);
}

#if 0
ESTATUS
KiScheduleSwitchContext(
    IN KSTACK_FRAME_INTERRUPT *InterruptFrame)
{
    DASSERT(KeGetCurrentIrql() == IRQL_CONTEXT_SWITCH);

    BOOLEAN Expired = FALSE;
    KPROCESSOR *Processor = KeGetCurrentProcessor();
    KTHREAD *CurrentThread = Processor->CurrentThread;

    DASSERT(CurrentThread);
    
    KiConsumeTimeslice(CurrentThread, 1, &Expired);

    if (!Expired)
    {
        return E_NOT_PERFORMED;
    }

    // Recalculate the timeslice and quantum by priority.
    U32 LastTimeslices = CurrentThread->CurrentTimeslices;

    CurrentThread->Priority = CurrentThread->BasePriority;
    CurrentThread->ThreadQuantum = // PRIORITY_TO_THREAD_QUANTUM(LastPriority)
        CurrentThread->Priority >= 2 ? CurrentThread->Priority / 2 : 1;

    U32 CurrentTimeslices = (CurrentThread->Priority + 1) * 2; // PRIORITY_TO_TIMESLICES(LastPriority)
    CurrentThread->RemainingTimeslices += CurrentTimeslices;
    CurrentThread->TimeslicesSpent += LastTimeslices;
    CurrentThread->CurrentTimeslices = CurrentTimeslices;

    KTHREAD *NextThread = NULL;

    CurrentThread->ContextSwitchCount++;

    DASSERT(KiSchedInsertThread(Processor->SchedNormalClass, CurrentThread, KSCHED_IDLE_QUEUE));
    DASSERT(KiSchedNextThread(Processor->SchedNormalClass, &NextThread));
    DASSERT(NextThread);

    Processor->CurrentThread = NextThread;

    // We don't need to save/load context from same thread. (especially CR3!)
    if (CurrentThread != NextThread)
    {
        // Save current thread context.
        KiLoadFrameToContext(InterruptFrame, &CurrentThread->ThreadContext);
        CurrentThread->ThreadContext.CR3 = __readcr3();

        // Check whether the SSE instruction was executed during thread quantum.
        if (!(__readcr0() & ARCH_X64_CR0_TS))
        {
            // Save SSE state as this thread uses SSE instructions
            _fxsave64(&CurrentThread->ThreadContext.FXSTATE);

            // Set CR0.TS to catch next #NM
            __writecr0(__readcr0() | ARCH_X64_CR0_TS);
        }

        // Load context from next thread.
        KiLoadContextToFrame(InterruptFrame, &NextThread->ThreadContext);

        // Reload CR3 only if needed.
        // Note that writing the CR3 flushes the TLB.
        if (__readcr3() != NextThread->ThreadContext.CR3)
        {
            __writecr3(NextThread->ThreadContext.CR3);
        }

        // We don't have to restore SSE state here
        // We'll restore SSE state in #NM handler
    }

    return E_SUCCESS;
}
#endif

VOID
KiYieldThreadInternal(
    IN KSTACK_FRAME_INTERRUPT *InterruptFrame)
{
    ESTATUS Status = E_SUCCESS;

    KIRQL PrevIrql = KeRaiseIrql(IRQL_CONTEXT_SWITCH);

    KTHREAD *PreviousThread = NULL;
    KTHREAD *CurrentThread = NULL;

    Status = KiScheduleNextThread(InterruptFrame, &PreviousThread, &CurrentThread);

    KeLowerIrql(PrevIrql);

    InterruptFrame->Rax = Status;
    InterruptFrame->Rdx = (U64)PreviousThread;
}

ESTATUS
KiYieldThread(
    VOID)
{
    U64 Result = 0;
    KTHREAD *PreviousThread = NULL;

    __asm__ __volatile__ (
        "int %2\n\t"
        : "=a"(Result), "=d"(PreviousThread)
        : "K"(SOFTWARE_INTERRUPT_YIELD_THREAD)
        :
    );

    return (ESTATUS)Result;
}

