
#include <stdio.h>
#include <Windows.h>
#include "types.h"
#include "list.h"

#include "runner_q.h"

#include "sched_base.h"
#include "sched_normal.h"

#include "wait.h"



KTIMER_LIST KiTimerList;



VOID
KiInitializeWaitHeader(
    OUT KWAIT_HEADER *Object,
    IN PVOID WaitContext)
{
    Object->Lock = 0;
    Object->State = 0;
    DListInitializeHead(&Object->WaitList);
    Object->WaitContext = NULL;
}

BOOLEAN
KiTryLockWaitHeader(
    IN KWAIT_HEADER *WaitHeader)
{
    return !_InterlockedExchange(&WaitHeader->Lock, 1);
}

VOID
KiLockWaitHeader(
    IN KWAIT_HEADER *WaitHeader)
{
    while (!KiTryLockWaitHeader(WaitHeader))
        _mm_pause();
}

VOID
KiUnlockWaitHeader(
    IN KWAIT_HEADER *WaitHeader)
{
    KASSERT(_InterlockedExchange(&WaitHeader->Lock, 0));
}

VOID
KiInitializeTimer(
    OUT KTIMER *Timer)
{
    memset(Timer, 0, sizeof(*Timer));

    KiInitializeWaitHeader(&Timer->WaitHeader, NULL);
    DListInitializeHead(&Timer->TimerList);
    Timer->Interval = 0;
    Timer->ExpirationTimeAbsolute = 0;
}






KTIMER_NODE *
KERNELAPI
KiTimerNodeAllocate(
    IN PVOID CallerContext)
{
    KTIMER_NODE *Node = (KTIMER_NODE *)malloc(sizeof(*Node));
    if (!Node)
        return NULL;

    memset(Node, 0, sizeof(*Node));
    DListInitializeHead(&Node->ListHead);

    return Node;
}

VOID
KERNELAPI
KiTimerNodeDelete(
    IN PVOID CallerContext,
    IN KTIMER_NODE *TimerNode)
{
    KASSERT(DListIsEmpty(&TimerNode->ListHead));
    //DListRemoveEntry(&TimerNode->ListHead);

    free(TimerNode);
}

U64 *
KERNELAPI
KiTimerNodeGetKey(
    IN PVOID CallerContext,
    IN KTIMER_NODE *TimerNode)
{
    return &TimerNode->Key;
}

VOID
KERNELAPI
KiTimerNodeSetKey(
    IN PVOID CallerContext,
    IN KTIMER_NODE *TimerNode,
    IN U64 *Key)
{
    TimerNode->Key = *Key;
}

INT
KERNELAPI
KiTimerNodeCompareKey(
    IN PVOID CallerContext,
    IN U64 *Key1,
    IN U64 *Key2)
{
    if (*Key1 < *Key2)
    {
        return -1;
    }
    else if (*Key1 > *Key2)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

SIZE_T
KiTimerNodeToString(
    IN PVOID CallerContext,
    IN U64 *Key,
    OUT CHAR *Buffer,
    IN SIZE_T BufferLength)
{
    return sprintf(Buffer, "%llu", *Key);
}

VOID
KiInitializeTimerList(
    IN KTIMER_LIST *TimerList)
{
    RS_BINARY_TREE_OPERATIONS Operations = 
    {
        .AllocateNode = (PRS_BINARY_TREE_ALLOCATE_NODE)&KiTimerNodeAllocate,
        .DeleteNode = (PRS_BINARY_TREE_DELETE_NODE)&KiTimerNodeDelete,
        .GetKey = (PRS_BINARY_TREE_GET_NODE_KEY)&KiTimerNodeGetKey,
        .SetKey = (PRS_BINARY_TREE_SET_NODE_KEY)&KiTimerNodeSetKey,
        .CompareKey = (PRS_BINARY_TREE_COMPARE_KEY)&KiTimerNodeCompareKey,
        .KeyToString = (PRS_BINARY_TREE_KEY_TO_STRING)&KiTimerNodeToString,
    };

    memset(TimerList, 0, sizeof(*TimerList));
    RsBtInitialize(&TimerList->Tree, &Operations, NULL);
}


BOOLEAN
KiTryLockTimerList(
    IN KTIMER_LIST *TimerList)
{
    return !_InterlockedExchange(&TimerList->Lock, 1);
}

VOID
KiLockTimerList(
    IN KTIMER_LIST *TimerList)
{
    while (!KiTryLockTimerList(TimerList))
        _mm_pause();
}

VOID
KiUnlockTimerList(
    IN KTIMER_LIST *TimerList)
{
    KASSERT(_InterlockedExchange(&TimerList->Lock, 0));
}

U64
KiGetFakeTickCount(
    VOID)
{
    return GetTickCount64();
}


ESTATUS
KiInsertTimer(
    IN KTIMER_LIST *TimerList,
    IN KTIMER *Timer,
    IN KTIMER_TYPE Type,
    IN U64 ExpirationTimeRelative)
{
    if (Type != TimerOneshot &&
        Type != TimerPeriodic)
    {
        return E_INVALID_PARAMETER;
    }

    //
    // Lock both timer and timer list before insert.
    // 

    do
    {
        if (!KiTryLockWaitHeader(&Timer->WaitHeader))
            continue;

        if (!KiTryLockTimerList(TimerList))
        {
            KiUnlockWaitHeader(&Timer->WaitHeader);
            continue;
        }

        break;
    } while (FALSE);

    ESTATUS Status = E_SUCCESS;

    U64 ExpirationTimeAbsolute = KiGetFakeTickCount() + ExpirationTimeRelative;
    RS_BINARY_TREE_LINK *BaseNode = NULL;

    if (!RsBtLookup(&TimerList->Tree, &ExpirationTimeAbsolute, &BaseNode))
    {
        if (!RsBtInsert(&TimerList->Tree, &ExpirationTimeAbsolute))
        {
            Status = E_FAILED;
            goto Cleanup;
        }

        KASSERT(RsBtLookup(&TimerList->Tree, &ExpirationTimeAbsolute, &BaseNode));
    }

    KASSERT(BaseNode);

    KTIMER_NODE *TimerNode = (KTIMER_NODE *)BaseNode;

    // Add to timer listhead.
    Timer->ExpirationTimeAbsolute = ExpirationTimeAbsolute;
    Timer->Type = Type;
    DListInsertAfter(&TimerNode->ListHead, &Timer->TimerList);


Cleanup:
    KiUnlockTimerList(TimerList);
    KiUnlockWaitHeader(&Timer->WaitHeader);

    return Status;
}

ESTATUS
KiLookupFirstExpiredTimerNode(
    IN KTIMER_LIST *TimerList,
    IN U64 ExpirationTimeAbsolute,
    OUT KTIMER_NODE **TimerNode)
{
    KTIMER_NODE *StartingNode = NULL;
    U64 AbsoluteTime = 0;

    if (!RsBtLookup2(&TimerList->Tree, &AbsoluteTime, 
        RS_BT_LOOKUP_NEAREST_ABOVE | RS_BT_LOOKUP_FLAG_EQUAL, 
        (RS_BINARY_TREE_LINK **)&StartingNode))
    {
        return E_NOT_FOUND;
    }

    if (StartingNode->Key > ExpirationTimeAbsolute)
    {
        return E_NOT_FOUND;
    }

    if (TimerNode)
    {
        *TimerNode = StartingNode;
    }

    return E_SUCCESS;
}

ESTATUS
KiStartTimer(
    IN KTIMER *Timer,
    IN KTIMER_TYPE Type,
    IN U64 ExpirationTimeRelative)
{
    return KiInsertTimer(&KiTimerList, Timer, Type, ExpirationTimeRelative);
}

VOID
KiW32FakeInitSystem(
    VOID)
{
    KiInitializeTimerList(&KiTimerList);
}

//ESTATUS
//KiWaitObject(
//    IN KWAIT_HEADER *WaitObjects[],
//    IN U32 Count,
//    IN U64 WaitTime)
//{
//    KTIMER Timer;
//    KiInitializeTimer(&Timer);
//
//
//    ESTATUS Status = KiStartTimer(&Timer, TimerOneshot, WaitTime);
//    if (!E_IS_SUCCESS(Status))
//    {
//        //
//    }
//}
