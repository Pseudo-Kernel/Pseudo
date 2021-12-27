
#pragma once

typedef struct _KTHREAD             KTHREAD;


#define WAIT_STATE_SIGNALED         0x80000000

#define TEST_WAIT_SIGNALED(_w)      ((_w)->State & WAIT_STATE_SIGNALED)

#define WAIT_FLAG_WAKE_ONE          0x00000001 // default: wake all

typedef struct _KWAIT_HEADER
{
    KSPIN_LOCK Lock;
    U32 State;          // See WAIT_STATE_XXX.
    U32 Flags;          // See WAIT_FLAG_XXX.
    DLIST_ENTRY WaiterBlockLinks;
    PVOID WaitContext;
} KWAIT_HEADER;

typedef struct _KWAITER_BLOCK
{
    KWAIT_HEADER *Object;           // Object that thread is waiting
    KTHREAD *Thread;                // Waiter thread
    DLIST_ENTRY WaiterBlockLinks;   // Links to waiter blocks
} KWAITER_BLOCK;


VOID
KiInitializeWaitHeader(
    OUT KWAIT_HEADER *Object,
    IN U32 Flags,
    IN PVOID WaitContext);

BOOLEAN
KiTryLockWaitHeader(
    IN KWAIT_HEADER *WaitHeader,
    OUT KIRQL *Irql);

VOID
KiLockWaitHeader(
    IN KWAIT_HEADER *WaitHeader,
    OUT KIRQL *Irql);

VOID
KiUnlockWaitHeader(
    IN KWAIT_HEADER *WaitHeader,
    IN KIRQL PrevIrql);

