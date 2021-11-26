
#pragma once

#include "bintree.h"

typedef struct _KPROCESSOR
{
    KSCHED_CLASS *NormalClass;
    KTHREAD *CurrentThread;
    U32 ProcessorId;

    // Win32 test only
    HANDLE Win32CtxThreadHandle;
} KPROCESSOR;





typedef struct _KWAIT_HEADER
{
    U32 Lock;
    U32 State;
    DLIST_ENTRY WaiterThreadList;
    PVOID WaitContext;
} KWAIT_HEADER;

typedef enum _KTIMER_TYPE
{
    TimerOneshot,
    TimerPeriodic,
} KTIMER_TYPE;

typedef struct _KTIMER
{
    // wait_all only
    KWAIT_HEADER WaitHeader;
    DLIST_ENTRY TimerList;      // Links to timer list
    KTIMER_TYPE Type;
    U64 ExpirationTimeAbsolute;
    U64 Interval;
    BOOLEAN Inserted;
} KTIMER;

typedef struct _KTIMER_NODE
{
    RS_AVL_NODE BaseNode;
    U64 Key;
    DLIST_ENTRY ListHead;
} KTIMER_NODE;

typedef struct _KTIMER_LIST
{
    U32 Lock;
    RS_AVL_TREE Tree;
} KTIMER_LIST;


#if 0
typedef struct _KMUTEX
{
    // wait_one only
    KWAIT_HEADER WaitHeader;
    struct _KTHREAD *Owner;
} KMUTEX;

typedef struct _KEVENT
{
    KWAIT_HEADER WaitHeader;
} KEVENT;
#endif



VOID
KiInitializeWaitHeader(
    OUT KWAIT_HEADER *Object,
    IN PVOID WaitContext);

BOOLEAN
KiTryLockWaitHeader(
    IN KWAIT_HEADER *WaitHeader);

VOID
KiLockWaitHeader(
    IN KWAIT_HEADER *WaitHeader);

VOID
KiUnlockWaitHeader(
    IN KWAIT_HEADER *WaitHeader);

VOID
KiInitializeTimer(
    OUT KTIMER *Timer);

VOID
KiInitializeTimerList(
    IN KTIMER_LIST *TimerList);


BOOLEAN
KiTryLockTimerList(
    IN KTIMER_LIST *TimerList);

VOID
KiLockTimerList(
    IN KTIMER_LIST *TimerList);

VOID
KiUnlockTimerList(
    IN KTIMER_LIST *TimerList);

U64
KiGetFakeTickCount(
    VOID); // W32


ESTATUS
KiInsertTimer(
    IN KTIMER_LIST *TimerList,
    IN KTIMER *Timer,
    IN KTIMER_TYPE Type,
    IN U64 ExpirationTimeRelative);

ESTATUS
KiLookupFirstExpiredTimerNode(
    IN KTIMER_LIST *TimerList,
    IN U64 ExpirationTimeAbsolute,
    OUT KTIMER_NODE **TimerNode);

ESTATUS
KeStartTimer(
    IN KTIMER *Timer,
    IN KTIMER_TYPE Type,
    IN U64 ExpirationTimeRelative);

ESTATUS
KeRemoveTimer(
    IN KTIMER *Timer);

VOID
KiW32FakeInitSystem(
    VOID); // W32



