
#include <base/base.h>
#include <misc/common.h>
#include <ke/lock.h>
#include <mm/pool.h>
#include <ke/interrupt.h>
#include <ke/inthandler.h>
#include <ke/kprocessor.h>
#include <ke/thread.h>

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

KTHREAD *
KiAllocateThread(
    IN U32 BasePriority,
    IN U32 ThreadId,
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

//KTHREAD *
//KiCreateThread(
//    IN U32 BasePriority,
//    IN PKTHREAD_ROUTINE StartRoutine,
//    IN PVOID ThreadArgument,
//    IN CHAR *ThreadName)
//{
//}

