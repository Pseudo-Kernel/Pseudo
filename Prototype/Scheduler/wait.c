
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

VOID
KiInitializeTimerList(
    IN KTIMER_LIST *TimerList)
{
    memset(TimerList, 0, sizeof(*TimerList));
    KiInitializeTimerTable(&TimerList->Root.Header, 0); // 0 = root
}

VOID
KiInitializeTimerTable(
    IN KTIMER_TABLE_HEADER *Header,
    IN U32 Level)
{
    if (Level >= (KTIMER_TABLE_MAX_LEVELS - 1))
    {
        KTIMER_TABLE_LEAF *Leaf = (KTIMER_TABLE_LEAF *)Header;
        memset(Leaf, 0, sizeof(*Leaf));
        Leaf->Header.Flags |= KTIMER_HEADER_FLAG_LEAF;
        Leaf->Header.Level = Level;

        for (ULONG i = 0; i < KTIMER_TABLE_MAX_ENTRIES; i++)
        {
            DListInitializeHead(&Leaf->BucketListHead[i]);
            DListInitializeHead(&Leaf->TimerListHead[i]);
        }

        DListInitializeHead(&Leaf->LeafList);
        DListInitializeHead(&Leaf->BucketSkipListHead);
    }
    else
    {
        KTIMER_TABLE *Table = (KTIMER_TABLE *)Header;
        memset(Table, 0, sizeof(*Table));

        Table->Header.Level = Level;
    }
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

KTIMER_TABLE_HEADER *
KiGetTimerTableNextLevel(
    IN KTIMER_TABLE *Table,
    IN U8 Index)
{
    ULONG ByteIndex = Index >> 3;
    U8 Mask = 1 << (Index & 0x07);

    if (Table->Header.Bitmap[ByteIndex] & Mask)
    {
        KASSERT(Table->Entry[ByteIndex]);
        return Table->Entry[ByteIndex];
    }

    return NULL;
}

ESTATUS
KiAllocateTimerTableNextLevel(
    IN KTIMER_TABLE *Table,
    IN U8 Index,
    OUT KTIMER_TABLE_HEADER **Header)
{
    ULONG ByteIndex = Index >> 3;
    U8 Mask = 1 << (Index & 0x07);

    if (Table->Header.Bitmap[ByteIndex] & Mask)
    {
        return E_ALREADY_EXISTS;
    }

    KTIMER_TABLE_HEADER *NextLevelTableHeader = NULL;

    if (Table->Header.Flags & KTIMER_HEADER_FLAG_LEAF)
    {
        // There is no next level
        return E_INVALID_PARAMETER;
    }
    else if (Table->Header.Level + 1 < (KTIMER_TABLE_MAX_LEVELS - 1))
    {
        KTIMER_TABLE *NextLevelTable = (KTIMER_TABLE *)malloc(sizeof(*NextLevelTable));
        if (!NextLevelTable)
        {
            return E_NOT_ENOUGH_MEMORY;
        }

        KiInitializeTimerTable(&NextLevelTable->Header, Table->Header.Level + 1);
        NextLevelTableHeader = &NextLevelTable->Header;
    }
    else if (Table->Header.Level + 1 == (KTIMER_TABLE_MAX_LEVELS - 1))
    {
        KTIMER_TABLE_LEAF *Leaf = (KTIMER_TABLE_LEAF *)malloc(sizeof(*Leaf));
        if (!Leaf)
        {
            return E_NOT_ENOUGH_MEMORY;
        }
        
        KiInitializeTimerTable(&Leaf->Header, Table->Header.Level + 1);
        NextLevelTableHeader = &Leaf->Header;
    }
    else
    {
        KASSERT(FALSE);
    }

    Table->Header.Bitmap[ByteIndex] |= Mask;
    Table->Header.AllocationCount++;
    Table->Entry[ByteIndex] = NextLevelTableHeader;

    if (Header)
    {
        *Header = NextLevelTableHeader;
    }

    return E_SUCCESS;
}

ESTATUS
KiGetTimerLeafTable(
    IN KTIMER_LIST *TimerList,
    IN U64 Key,
    IN BOOLEAN Emplace,
    OUT U32 *LastLevel,
    OUT KTIMER_TABLE_LEAF **Leaf,
    OUT U8 *BucketIndex)
{
    KTIMER_TABLE_HEADER *TableHeader = &TimerList->Root.Header;

    for (ULONG i = 0; i < (KTIMER_TABLE_MAX_LEVELS - 1); i++)
    {
        U8 Index = (U8)(Key >> ((7 - i) << 3));
        KTIMER_TABLE *Table = (KTIMER_TABLE *)TableHeader;

        KASSERT(!(Table->Header.Flags & KTIMER_HEADER_FLAG_LEAF));

        KTIMER_TABLE_HEADER *NextLevelHeader = KiGetTimerTableNextLevel(Table, Index);
        if (!NextLevelHeader)
        {
            if (!Emplace)
            {
                if (LastLevel)
                {
                    *LastLevel = i;
                }

                return E_NOT_FOUND;
            }

            ESTATUS Status = KiAllocateTimerTableNextLevel((KTIMER_TABLE *)TableHeader, Index, &NextLevelHeader);
            if (!E_IS_SUCCESS(Status))
            {
                return Status;
            }

            KASSERT(NextLevelHeader);
        }

        TableHeader = NextLevelHeader;
    }

    KASSERT(
        (TableHeader->Flags & KTIMER_HEADER_FLAG_LEAF) &&
        (TableHeader->Level == (KTIMER_TABLE_MAX_LEVELS - 1)));

    if (LastLevel)
    {
        *LastLevel = TableHeader->Level;
    }

    if (Leaf)
    {
        *Leaf = (KTIMER_TABLE_LEAF *)TableHeader;
    }

    if (BucketIndex)
    {
        *BucketIndex = (U8)(Key & 0xff);
    }

    return E_SUCCESS;
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

    KTIMER_TABLE_LEAF *Leaf = NULL;
    U8 BucketIndex = 0;
    U64 ExpirationTimeAbsolute = KiGetFakeTickCount() + ExpirationTimeRelative;

    ESTATUS Status = KiGetTimerLeafTable(TimerList, ExpirationTimeAbsolute, TRUE, NULL, &Leaf, &BucketIndex);

    if (!E_IS_SUCCESS(Status))
    {
        goto Cleanup;
    }

    // Add to timer listhead.
    Timer->ExpirationTimeAbsolute = ExpirationTimeAbsolute;
    Timer->Type = Type;
    DListInsertAfter(&Leaf->TimerListHead[BucketIndex], &Timer->TimerList);

    if (!KiTestTimerTableAllocationBit(&Leaf->Header, BucketIndex))
    {
        // Add to skip list
        if (!Leaf->Header.AllocationCount)
        {
            // Add first link
            DListInsertAfter(&Leaf->BucketSkipListHead, &Leaf->BucketListHead[BucketIndex]);
        }
        else
        {
            INT PrevIndex = -1;
            for (ULONG i = 0; i < BucketIndex; i++)
            {
                if (KiTestTimerTableAllocationBit(&Leaf->Header, i))
                    PrevIndex = i;
            }

            KASSERT(PrevIndex >= 0);
            DListInsertAfter(&Leaf->BucketListHead[PrevIndex], &Leaf->BucketListHead[BucketIndex]);
        }

        KiSetTimerTableAllocationBit(&Leaf->Header, BucketIndex);
        Leaf->Header.AllocationCount++;
    }

Cleanup:
    KiUnlockTimerList(TimerList);
    KiUnlockWaitHeader(&Timer->WaitHeader);

    return Status;
}

#define KTIMER_KEY_TO_LEAF_INDEX(_k)            ((_k) & 0xff)
#define KTIMER_KEY_LEAF_INDEX_ZERO(_k)          ((_k) & ~0xff)

ESTATUS
KiLookupTimer(
    IN KTIMER_LIST *TimerList,
    IN PKTIMER_LOOKUP Lookup,
    IN PVOID LookupContext,
    IN U64 AbsoluteTimeStart,
    IN U64 AbsoluteTimeEnd)
{
    // [AbsoluteTimeStart, AbsoluteTimeEnd)

    U64 Key = AbsoluteTimeStart;
    while (Key < AbsoluteTimeEnd)
    {
        KTIMER_TABLE_LEAF *Leaf = NULL;
        U8 LeafIndexStart = 0;
        U32 LastLevel = 0;
        ESTATUS Status = KiGetTimerLeafTable(TimerList, Key, FALSE, &LastLevel, &Leaf, &LeafIndexStart);

        KASSERT(0 <= LastLevel && LastLevel < KTIMER_TABLE_MAX_LEVELS);

        if (!E_IS_SUCCESS(Status))
        {
            if (!LastLevel)
            {
                return E_NOT_FOUND;
            }
        }
        else
        {
            ULONG LeafIndexEnd = 0x100;
            U64 Base = KTIMER_KEY_LEAF_INDEX_ZERO(Key);
            if (Base == AbsoluteTimeEnd)
            {
                LeafIndexEnd = KTIMER_KEY_TO_LEAF_INDEX(AbsoluteTimeEnd);
            }

            if (LeafIndexStart == 0 && LeafIndexEnd == 0x100)
            {
                // Use skip list
                DLIST_ENTRY *ListHead = &Leaf->BucketSkipListHead;
                DLIST_ENTRY *Current = ListHead->Next;

                while (ListHead != Current)
                {
                    ULONG Index = Current - Leaf->BucketListHead;
                    Lookup(Leaf, Base + Index, Index, LookupContext);
                    Current = Current->Next;
                }
            }
            else
            {
                for (ULONG LeafIndex = LeafIndexStart; LeafIndex < LeafIndexEnd; LeafIndex++)
                {
                    if (KiTestTimerTableAllocationBit(&Leaf->Header, LeafIndex))
                    {
                        Lookup(Leaf, Base + LeafIndex, LeafIndex, LookupContext);
                    }
                }
            }
        }

        U64 Alignment = 1ULL << ((KTIMER_TABLE_MAX_LEVELS - LastLevel) << 3);
        Key = (Key & ~(Alignment - 1)) + Alignment;
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
