
#pragma once

#include <ke/wait.h>

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
    KSPIN_LOCK Lock;
    RS_AVL_TREE Tree;
} KTIMER_LIST;


VOID
KiInitializeTimer(
    OUT KTIMER *Timer);

VOID
KiInitializeTimerList(
    IN KTIMER_LIST *TimerList);


BOOLEAN
KiTryLockTimerList(
    IN KTIMER_LIST *TimerList,
    OUT KIRQL *Irql);
    
VOID
KiLockTimerList(
    IN KTIMER_LIST *TimerList,
    OUT KIRQL *Irql);

VOID
KiUnlockTimerList(
    IN KTIMER_LIST *TimerList,
    IN KIRQL PrevIrql);


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



