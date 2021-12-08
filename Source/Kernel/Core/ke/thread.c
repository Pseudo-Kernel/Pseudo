
/**
 * @file thread.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements thread related routines.
 * @version 0.1
 * @date 2021-12-06
 * 
 * @copyright Copyright (c) 2021
 * 
 * @todo Preserve extended contexts when doing context switch (x87/XMM/YMM/...)
 */

#include <base/base.h>
#include <misc/common.h>
#include <ke/lock.h>
#include <init/bootgfx.h>
#include <mm/pool.h>
#include <mm/mm.h>
#include <ke/interrupt.h>
#include <ke/inthandler.h>
#include <ke/kprocessor.h>
#include <ke/thread.h>
#include <ke/process.h>

U64 KiThreadIdSeed;
DLIST_ENTRY KiThreadListHead;
KSPIN_LOCK KiThreadListLock;

KTHREAD *
KeGetCurrentThread(
    VOID)
{
    return KeGetCurrentProcessor()->CurrentThread;
}

VOID
KiInitializeThread(
    IN KTHREAD *Thread,
    IN U32 BasePriority,
    IN U64 ThreadId,
    IN CHAR *ThreadName)
{
    memset(Thread, 0, sizeof(*Thread));

    KeInitializeSpinlock(&Thread->Lock);
    DListInitializeHead(&Thread->ThreadList);
    DListInitializeHead(&Thread->ProcessThreadList);

    Thread->ReferenceCount = 1;

    DListInitializeHead(&Thread->WaiterList);
    DListInitializeHead(&Thread->RunnerLinks);

    Thread->BasePriority = BasePriority;
    Thread->Priority = BasePriority;
    Thread->ThreadId = ThreadId;
    Thread->RunnerQueue = NULL;
    Thread->State = ThreadStateInitialize;
    Thread->InWaiting = FALSE;

    if (ThreadName)
    {
        ClStrCopyU8(Thread->Name, THREAD_NAME_MAX_LENGTH, ThreadName);
    }
    else
    {
        Thread->Name[0] = 0;
    }
}

VOID
KiSaveControlRegisters(
    IN KTHREAD_CONTEXT *Context)
{
    Context->CR0 = __readcr0();
    Context->CR2 = __readcr2();
    Context->CR3 = __readcr3();
    Context->CR4 = __readcr4();
    Context->CR8 = __readcr8();
}

VOID
KiRestoreControlRegisters(
    IN KTHREAD_CONTEXT *Context)
{
    __writecr0(Context->CR0);
    __writecr2(Context->CR2);
    __writecr3(Context->CR3);
    __writecr4(Context->CR4);
    __writecr8(Context->CR8);
}

VOID
KiLoadContextToFrame(
    IN KSTACK_FRAME_INTERRUPT *InterruptFrame,
    IN KTHREAD_CONTEXT *Context)
{
    InterruptFrame->Cs = Context->Cs;
    InterruptFrame->Ds = Context->Ds;
    InterruptFrame->Es = Context->Es;
    InterruptFrame->Fs = Context->Fs;
    InterruptFrame->Gs = Context->Gs;
    InterruptFrame->Ss = Context->Ss;

    InterruptFrame->Rax = Context->Rax;
    InterruptFrame->Rbx = Context->Rbx;
    InterruptFrame->Rcx = Context->Rcx;
    InterruptFrame->Rdx = Context->Rdx;
    InterruptFrame->Rsi = Context->Rsi;
    InterruptFrame->Rdi = Context->Rdi;
    InterruptFrame->Rsp = Context->Rsp;
    InterruptFrame->Rbp = Context->Rbp;
    InterruptFrame->R8 = Context->R8;
    InterruptFrame->R9 = Context->R9;
    InterruptFrame->R10 = Context->R10;
    InterruptFrame->R11 = Context->R11;
    InterruptFrame->R12 = Context->R12;
    InterruptFrame->R13 = Context->R13;
    InterruptFrame->R14 = Context->R14;
    InterruptFrame->R15 = Context->R15;

    InterruptFrame->Rip = Context->Rip;
    InterruptFrame->Rflags = Context->Rflags;
}

VOID
KiLoadFrameToContext(
    IN KSTACK_FRAME_INTERRUPT *InterruptFrame,
    IN KTHREAD_CONTEXT *Context)
{
    Context->Cs = InterruptFrame->Cs;
    Context->Ds = InterruptFrame->Ds;
    Context->Es = InterruptFrame->Es;
    Context->Fs = InterruptFrame->Fs;
    Context->Gs = InterruptFrame->Gs;
    Context->Ss = InterruptFrame->Ss;

    Context->Rax = InterruptFrame->Rax;
    Context->Rbx = InterruptFrame->Rbx;
    Context->Rcx = InterruptFrame->Rcx;
    Context->Rdx = InterruptFrame->Rdx;
    Context->Rsi = InterruptFrame->Rsi;
    Context->Rdi = InterruptFrame->Rdi;
    Context->Rsp = InterruptFrame->Rsp;
    Context->Rbp = InterruptFrame->Rbp;
    Context->R8 = InterruptFrame->R8;
    Context->R9 = InterruptFrame->R9;
    Context->R10 = InterruptFrame->R10;
    Context->R11 = InterruptFrame->R11;
    Context->R12 = InterruptFrame->R12;
    Context->R13 = InterruptFrame->R13;
    Context->R14 = InterruptFrame->R14;
    Context->R15 = InterruptFrame->R15;

    Context->Rip = InterruptFrame->Rip;
    Context->Rflags = InterruptFrame->Rflags;
}

VOID
KiSystemThreadStartup(
    IN KTHREAD *Thread)
{
    Thread->StartRoutine(Thread->ThreadArgument);

    // @todo: Exit self
    FATAL("Thread %d returned without exit!", Thread->ThreadId);
}

ESTATUS
KiSetupInitialContextThread(
    IN KTHREAD *Thread,
    IN SIZE_T StackSize, 
    IN PVOID PML4Base)
{
    PHYSICAL_ADDRESSES_R128 PhysicalAddresses;
    INITIALIZE_PHYSICAL_ADDRESSES_R128(&PhysicalAddresses, 0);

    if (!StackSize)
    {
        StackSize = KERNEL_STACK_SIZE_DEFAULT;
    }

    ESTATUS Status = MmAllocateAndMapPagesGather(
        &PhysicalAddresses.Addresses, StackSize, ARCH_X64_PXE_WRITABLE, PadInUse, VadInUse);

    if (!E_IS_SUCCESS(Status))
    {
        return Status;
    }

    PHYSICAL_ADDRESSES *PaList = MmAllocatePhysicalAddressesStructure(
        PoolTypeNonPaged, PhysicalAddresses.Addresses.AddressCount);

    if (!PaList)
    {
        MmFreeAndUnmapPagesGather(&PhysicalAddresses.Addresses);
        return E_NOT_ENOUGH_MEMORY;
    }

    INITIALIZE_PHYSICAL_ADDRESSES(PaList, 0, PhysicalAddresses.Addresses.AddressCount);
    Status = MmCopyPhysicalAddressesStructure(PaList, &PhysicalAddresses.Addresses);
    DASSERT(E_IS_SUCCESS(Status));

    //
    // Allocate thread stack and setup context.
    //

    KIRQL PrevIrql = KiLockThread(Thread);

    DASSERT(!Thread->StackBase && !Thread->StackSize && !Thread->StackPaList);

    PVOID AllocatedStackBase = (PVOID)PhysicalAddresses.Addresses.StartingVirtualAddress;
    SIZE_T AllocatedStackSize = PhysicalAddresses.Addresses.AllocatedSize;

    Thread->StackBase = AllocatedStackBase;
    Thread->StackSize = AllocatedStackSize;
    Thread->StackPaList = PaList;

    // Zero out all fields in context.
    memset(&Thread->ThreadContext, 0, sizeof(Thread->ThreadContext));

    Thread->ThreadContext.CR0 = ARCH_X64_CR0_PG | ARCH_X64_CR0_WP | ARCH_X64_CR0_NE | ARCH_X64_CR0_MP | ARCH_X64_CR0_PE;
    Thread->ThreadContext.CR2 = 0;
    Thread->ThreadContext.CR3 = (U64)PML4Base; // PCD and PWT cleared
    Thread->ThreadContext.CR4 = ARCH_X64_CR4_OSXMMEXCPT | ARCH_X64_CR4_OSFXSR | ARCH_X64_CR4_PGE | ARCH_X64_CR4_MCE | ARCH_X64_CR4_PAE;
    Thread->ThreadContext.CR8 = IRQL_LOWEST; // All interrupts enabled

    Thread->ThreadContext.Cs = KERNEL_CS;
    Thread->ThreadContext.Ds = KERNEL_DS;
    Thread->ThreadContext.Es = KERNEL_DS;
    Thread->ThreadContext.Fs = KERNEL_DS;
    Thread->ThreadContext.Gs = KERNEL_DS;
    Thread->ThreadContext.Ss = KERNEL_SS;

    Thread->ThreadContext.Rflags = RFLAG_IF | (1 << 1); /* RFLAGS[1] is always 1 */
    Thread->ThreadContext.Rip = (U64)&KiSystemThreadStartup;
    Thread->ThreadContext.Rcx = (U64)Thread; /* msabi uses rcx for 1st argument */
    Thread->ThreadContext.Rdi = (U64)Thread; /* sysvabi uses rdi for 1st argument */
    // RSP = (Base) + (Size) - (RIP placeholder size (8bytes)) - (Extra size (e.g. red zone))
    Thread->ThreadContext.Rsp = (U64)AllocatedStackBase + AllocatedStackSize - sizeof(U64) - sizeof(U64) * 16;

    KiUnlockThread(Thread, PrevIrql);

    return Status;
}

KTHREAD *
KiAllocateThread(
    IN U32 BasePriority,
    IN U64 ThreadId,
    IN CHAR *ThreadName)
{
    KTHREAD *Thread = (KTHREAD *)MmAllocatePool(PoolTypeNonPaged, sizeof(KTHREAD), 0x10, 0);

    if (!Thread)
    {
        return NULL;
    }

    KiInitializeThread(Thread, BasePriority, ThreadId, ThreadName);

    return Thread;
}

KIRQL
KiLockThread(
    IN KTHREAD *Thread)
{
    KIRQL PrevIrql;
    KeAcquireSpinlockRaiseIrqlToContextSwitch(&Thread->Lock, &PrevIrql);

    return PrevIrql;
}

VOID
KiUnlockThread(
    IN KTHREAD *Thread,
    IN KIRQL PrevIrql)
{
    KeReleaseSpinlockLowerIrql(&Thread->Lock, PrevIrql);
}

ESTATUS
KiInsertThread(
    IN KPROCESS *Process,
    IN KTHREAD *Thread)
{
    KIRQL PrevIrql;
    
    DbgTraceF(TraceLevelDebug, 
        "Inserting thread 0x%016llx (id %d, name '%s') to"
        " process 0x%016llx (id %d, name '%s')\n", 
        Thread, Thread->ThreadId, Thread->Name, 
        Process, Process->ProcessId, Process->Name);

    KSPIN_LOCK *Locks_ThreadList[] = { &KiThreadListLock, &Thread->Lock };

    KeAcquireSpinlockMultipleRaiseIrql(Locks_ThreadList, COUNTOF(Locks_ThreadList), IRQL_CONTEXT_SWITCH, &PrevIrql);
    DListInsertAfter(&KiThreadListHead, &Thread->ThreadList);
    KeReleaseSpinlockMultipleLowerIrql(Locks_ThreadList, COUNTOF(Locks_ThreadList), PrevIrql);

    KSPIN_LOCK *Locks_ProcessThread[] = { &Process->Lock, &Thread->Lock };

    KeAcquireSpinlockMultipleRaiseIrql(Locks_ProcessThread, COUNTOF(Locks_ProcessThread), IRQL_CONTEXT_SWITCH, &PrevIrql);
    DListInsertAfter(&Process->ThreadList, &Thread->ProcessThreadList);
    Thread->OwnerProcess = Process;
    KeReleaseSpinlockMultipleLowerIrql(Locks_ProcessThread, COUNTOF(Locks_ProcessThread), PrevIrql);

    return E_SUCCESS;
}

KTHREAD *
KiCreateThread(
    IN U32 BasePriority,
    IN PKTHREAD_ROUTINE StartRoutine,
    IN PVOID ThreadArgument,
    IN CHAR *ThreadName)
{
    U64 NextThreadId = _InterlockedIncrement64((long long *)&KiThreadIdSeed);

    KTHREAD *Thread = KiAllocateThread(BasePriority, NextThreadId, ThreadName);

    if (!Thread)
    {
        return NULL;
    }

    Thread->ThreadArgument = ThreadArgument;
    Thread->StartRoutine = StartRoutine;

    return Thread;
}

VOID
KiDeleteThread(
    IN KTHREAD *Thread)
{
    // @todo: Free members before process deletion
    //        Not implemented
    MmFreePool(Thread);
}
