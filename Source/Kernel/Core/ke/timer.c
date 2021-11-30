
/**
 * @file timer.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements routines for timer object.
 * @version 0.1
 * @date 2021-12-01
 * 
 * @copyright Copyright (c) 2021
 * 
 * @todo Not tested
 */

#include <base/base.h>
#include <ke/lock.h>
#include <ke/interrupt.h>
#include <mm/pool.h>
#include <ke/timer.h>
#include <hal/ptimer.h>

KTIMER_LIST KiTimerList;



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
    KTIMER_NODE *Node = (KTIMER_NODE *)MmAllocatePool(PoolTypeNonPaged, sizeof(*Node), 0x10, 0);
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
    DASSERT(DListIsEmpty(&TimerNode->ListHead));
    //DListRemoveEntry(&TimerNode->ListHead);

    MmFreePool(TimerNode);
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
    Buffer[0] = 0;
    return 1;//sprintf(Buffer, "%llu", *Key);
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
    IN KTIMER_LIST *TimerList,
    OUT KIRQL *Irql)
{
    return KeTryAcquireSpinlockRaiseIrqlToContextSwitch(&TimerList->Lock, Irql);
}

VOID
KiLockTimerList(
    IN KTIMER_LIST *TimerList,
    OUT KIRQL *Irql)
{
    while (!KiTryLockTimerList(TimerList, Irql))
        _mm_pause();
}

VOID
KiUnlockTimerList(
    IN KTIMER_LIST *TimerList,
    IN KIRQL PrevIrql)
{
    KeReleaseSpinlockLowerIrql(&TimerList->Lock, PrevIrql);
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

    KIRQL PrevIrql;
    KIRQL PrevIrql2;

    do
    {
        if (!KiTryLockWaitHeader(&Timer->WaitHeader, &PrevIrql))
            continue;

        if (!KiTryLockTimerList(TimerList, &PrevIrql2))
        {
            KiUnlockWaitHeader(&Timer->WaitHeader, PrevIrql);
            continue;
        }

        break;
    } while (FALSE);

    ESTATUS Status = E_SUCCESS;

    U64 ExpirationTimeAbsolute = HalGetTickCount() + ExpirationTimeRelative;
    RS_BINARY_TREE_LINK *BaseNode = NULL;

    if (!RsBtLookup(&TimerList->Tree, &ExpirationTimeAbsolute, &BaseNode))
    {
        if (!RsBtInsert(&TimerList->Tree, &ExpirationTimeAbsolute))
        {
            Status = E_FAILED;
            goto Cleanup;
        }

        DASSERT(RsBtLookup(&TimerList->Tree, &ExpirationTimeAbsolute, &BaseNode));
    }

    DASSERT(BaseNode);

    KTIMER_NODE *TimerNode = (KTIMER_NODE *)BaseNode;

    // Add to timer listhead.
    Timer->ExpirationTimeAbsolute = ExpirationTimeAbsolute;
    Timer->Type = Type;
    Timer->Inserted = TRUE;
    DListInsertAfter(&TimerNode->ListHead, &Timer->TimerList);


Cleanup:
    KiUnlockTimerList(TimerList, PrevIrql2);
    KiUnlockWaitHeader(&Timer->WaitHeader, PrevIrql);

    return Status;
}

ESTATUS
KiRemoveTimer(
    IN KTIMER_LIST *TimerList,
    IN KTIMER *Timer)
{
    KIRQL PrevIrql;
    KIRQL PrevIrql2;

    do
    {
        if (!KiTryLockWaitHeader(&Timer->WaitHeader, &PrevIrql))
            continue;

        if (!KiTryLockTimerList(TimerList, &PrevIrql2))
        {
            KiUnlockWaitHeader(&Timer->WaitHeader, PrevIrql);
            continue;
        }

        break;
    } while (FALSE);

    ESTATUS Status = E_SUCCESS;

    if (!Timer->Inserted)
    {
        Status = E_INVALID_PARAMETER;
        goto Cleanup;
    }

    U64 ExpirationTimeAbsolute = Timer->ExpirationTimeAbsolute;
    RS_BINARY_TREE_LINK *BaseNode = NULL;

    if (!RsBtLookup(&TimerList->Tree, &ExpirationTimeAbsolute, &BaseNode))
    {
        Status = E_LOOKUP_FAILED;
        goto Cleanup;
    }

    DASSERT(BaseNode);

    KTIMER_NODE *TimerNode = (KTIMER_NODE *)BaseNode;

    // Remove from timer listhead.
    DListRemoveEntry(&Timer->TimerList);
    Timer->Inserted = FALSE;

    if (DListIsEmpty(&TimerNode->ListHead))
    {
        // Timer node is empty. Remove it.
        DASSERT(RsAvlDeleteByKey(&TimerList->Tree, &ExpirationTimeAbsolute));
    }

Cleanup:
    KiUnlockTimerList(TimerList, PrevIrql2);
    KiUnlockWaitHeader(&Timer->WaitHeader, PrevIrql);

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
KeStartTimer(
    IN KTIMER *Timer,
    IN KTIMER_TYPE Type,
    IN U64 ExpirationTimeRelative)
{
    return KiInsertTimer(&KiTimerList, Timer, Type, ExpirationTimeRelative);
}

ESTATUS
KeRemoveTimer(
    IN KTIMER *Timer)
{
    return KiRemoveTimer(&KiTimerList, Timer);
}
