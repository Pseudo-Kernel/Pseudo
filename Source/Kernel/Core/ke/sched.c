
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
#include <mm/pool.h>
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

    for (;;)
    {
        // Test!
        DASSERT(Processor->CurrentThread == Thread);
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
    CurrentThread->ContextSwitchCount++;

    KTHREAD *NextThread = NULL;

    DASSERT(KiSchedInsertThread(Processor->SchedNormalClass, CurrentThread, KSCHED_IDLE_QUEUE));
    DASSERT(KiSchedNextThread(Processor->SchedNormalClass, &NextThread));
    DASSERT(NextThread);

    Processor->CurrentThread = NextThread;

    // Save current context and load next thread context.
    KiLoadFrameToContext(InterruptFrame, &CurrentThread->ThreadContext);
    KiSaveControlRegisters(&CurrentThread->ThreadContext);

    // We don't need to load context from same thread. (especially CR3!)
    if (CurrentThread != NextThread)
    {
        KiLoadContextToFrame(InterruptFrame, &NextThread->ThreadContext);
        KiRestoreControlRegisters(&CurrentThread->ThreadContext);
    }

    return E_SUCCESS;
}
