
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
#include <ke/sched.h>

VOID
KiProcessorSchedInitialize(
    VOID)
{
    KPROCESSOR *Processor = KeGetCurrentProcessor();

    KSCHED_CLASS *NormalClass = (KSCHED_CLASS *)MmAllocatePool(PoolTypeNonPaged, sizeof(KSCHED_CLASS), 0x10, 0);
    if (!NormalClass)
    {
        FATAL("Failed to allocate scheduler object");
    }

    DASSERT(KiSchedInitialize(NormalClass, 
        &KiSchedNormalInsertThread, &KiSchedNormalRemoveThread, 
        &KiSchedNormalPeekThread, &KiSchedNormalNextThread, NULL, KSCHED_NORMAL_CLASS_LEVELS));

    KTHREAD *IdleThread = (KTHREAD *)MmAllocatePool(PoolTypeNonPaged, sizeof(KTHREAD), 0x10, 0);
    if (!IdleThread)
    {
        FATAL("Failed to allocate thread object");
    }

    KiInitializeThread(IdleThread, 0, 0, "Idle");

    Processor->CurrentThread = IdleThread;
    Processor->SchedNormalClass = NormalClass;
}
