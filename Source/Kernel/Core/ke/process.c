
/**
 * @file process.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements process related routines.
 * @version 0.1
 * @date 2021-12-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <base/base.h>
#include <misc/common.h>
#include <ke/lock.h>
#include <mm/pool.h>
#include <mm/mm.h>
#include <ke/interrupt.h>
#include <ke/inthandler.h>
#include <ke/kprocessor.h>
#include <ke/thread.h>
#include <ke/process.h>
#include <hal/apic.h>

U64 KiProcessIdSeed;
DLIST_ENTRY KiProcessListHead;
KSPIN_LOCK KiProcessListLock;

KPROCESS KiIdleProcess;
KPROCESS KiSystemProcess;


VOID
KiCreateInitialProcessThreads(
    VOID)
{
    // BSP only
    DASSERT(HalIsBootstrapProcessor());

    DListInitializeHead(&KiProcessListHead);
    DListInitializeHead(&KiThreadListHead);
    KiProcessIdSeed = 0x40;

    KiInitializeProcess(&KiIdleProcess, 0, "Idle");
    DASSERT(E_IS_SUCCESS(KiInsertProcess(&KiIdleProcess)));

    KiInitializeProcess(&KiSystemProcess, 1, "System");
    DASSERT(E_IS_SUCCESS(KiInsertProcess(&KiSystemProcess)));
}

KPROCESS *
KeGetCurrentProcess(
    VOID)
{
    return KeGetCurrentProcessor()->CurrentThread->OwnerProcess;
}

VOID
KiInitializeProcess(
    IN KPROCESS *Process,
    IN U64 ProcessId,
    IN CHAR *ProcessName)
{
    memset(Process, 0, sizeof(*Process));

    KeInitializeSpinlock(&Process->Lock);

    DListInitializeHead(&Process->ThreadList);
    DListInitializeHead(&Process->ProcessList);

    Process->ReferenceCount = 1;

    Process->ProcessId = ProcessId;
    Process->KernelVads = &MiVadTree;
    Process->UserVads = NULL;
    Process->TopLevelPageTableVirtualBase = MiPML4TBase;
    Process->TopLevelPageTablePhysicalBase = MiPML4TPhysicalBase;
    
    if (ProcessName)
    {
        ClStrCopyU8(Process->Name, PROCESS_NAME_MAX_LENGTH, ProcessName);
    }
    else
    {
        Process->Name[0] = 0;
    }
}

KPROCESS *
KiAllocateProcess(
    IN U64 ProcessId,
    IN CHAR *ProcessName)
{
    KPROCESS *Process = (KPROCESS *)MmAllocatePool(PoolTypeNonPaged, sizeof(KPROCESS), 0x10, 0);

    if (!Process)
    {
        return NULL;
    }

    KiInitializeProcess(Process, ProcessId, ProcessName);

    return Process;
}

KPROCESS *
KiCreateProcess(
    IN CHAR *ProcessName)
{
    U64 NextProcessId = _InterlockedIncrement64((long long *)&KiProcessIdSeed);

    KPROCESS *Process = KiAllocateProcess(NextProcessId, ProcessName);

    if (!Process)
    {
        return NULL;
    }

    return Process;
}

ESTATUS
KiInsertProcess(
    IN KPROCESS *Process)
{
    KIRQL PrevIrql;
    KSPIN_LOCK *LockList[] = { &KiProcessListLock, &Process->Lock };

    KeAcquireSpinlockMultipleRaiseIrql(LockList, COUNTOF(LockList), IRQL_CONTEXT_SWITCH, &PrevIrql);
    DListInsertAfter(&KiProcessListHead, &Process->ProcessList);
    KeReleaseSpinlockMultipleLowerIrql(LockList, COUNTOF(LockList), PrevIrql);

    return E_SUCCESS;
}

VOID
KiDeleteProcess(
    IN KPROCESS *Process)
{
    // @todo: Free members before process deletion
    //        Not implemented
    MmFreePool(Process);
}


