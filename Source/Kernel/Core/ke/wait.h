
#pragma once

typedef struct _KWAIT_HEADER
{
    KSPIN_LOCK Lock;
    U32 State;
    DLIST_ENTRY WaiterThreadList;
    PVOID WaitContext;
} KWAIT_HEADER;


VOID
KiInitializeWaitHeader(
    OUT KWAIT_HEADER *Object,
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

