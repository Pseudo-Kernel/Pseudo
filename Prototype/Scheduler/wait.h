
#pragma once


typedef struct _KWAIT_HEADER
{
    U32 Lock;
    U32 State;
    DLIST_ENTRY WaitList;
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
    DLIST_ENTRY TimerList;      // Links to timer tree (self-balancing)
    KTIMER_TYPE Type;
    U64 ExpirationTimeAbsolute;
    U64 Interval;
} KTIMER;

#define KTIMER_TABLE_MAX_LEVELS             8
#define KTIMER_TABLE_MAX_ENTRIES            256
#define KTIMER_HEADER_FLAG_LEAF             0x00000001


#define KiTestTimerTableAllocationBit(_table_header, _index)  \
    ( (_table_header)->Bitmap[(_index) >> 3] & (1 << ((_index) & 7)) )

#define KiSetTimerTableAllocationBit(_table_header, _index)  \
    (_table_header)->Bitmap[(_index) >> 3] |= (1 << ((_index) & 7))

#define KiClearTimerTableAllocationBit(_table_header, _index)  \
    (_table_header)->Bitmap[(_index) >> 3] &= ~(1 << ((_index) & 7))

typedef struct _KTIMER_TABLE_HEADER
{
    U8 Bitmap[32];
    U32 Flags;
    U16 Level;
    U16 AllocationCount;
} KTIMER_TABLE_HEADER;

typedef struct _KTIMER_TABLE_LEAF
{
    KTIMER_TABLE_HEADER Header;
    DLIST_ENTRY LeafList;
    DLIST_ENTRY BucketSkipListHead;
    DLIST_ENTRY BucketListHead[KTIMER_TABLE_MAX_ENTRIES];
    DLIST_ENTRY TimerListHead[KTIMER_TABLE_MAX_ENTRIES];
} KTIMER_TABLE_LEAF;

typedef struct _KTIMER_TABLE
{
    KTIMER_TABLE_HEADER Header;
    struct _KTIMER_TABLE_HEADER *Entry[KTIMER_TABLE_MAX_ENTRIES];
} KTIMER_TABLE;

typedef struct _KTIMER_LIST
{
    // 8-level tables
    // ExpirationTimeAbsolute = [63..56][55..48] ... [7..0]
    U32 Lock;
    KTIMER_TABLE Root;
} KTIMER_LIST;

typedef
BOOLEAN
(*PKTIMER_LOOKUP)(
    IN KTIMER_TABLE_LEAF *Leaf,
    IN U64 AbsoluteTime,
    IN U8 Index,
    IN PVOID LookupContext);


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

VOID
KiInitializeTimerTable(
    IN KTIMER_TABLE_HEADER *Header,
    IN U32 Level);

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

KTIMER_TABLE_HEADER *
KiGetTimerTableNextLevel(
    IN KTIMER_TABLE *Table,
    IN U8 Index);

ESTATUS
KiAllocateTimerTableNextLevel(
    IN KTIMER_TABLE *Table,
    IN U8 Index,
    OUT KTIMER_TABLE_HEADER **Header);

ESTATUS
KiGetTimerLeafTable(
    IN KTIMER_LIST *TimerList,
    IN U64 Key,
    IN BOOLEAN Emplace,
    OUT U32 *LastLevel,
    OUT KTIMER_TABLE_LEAF **Leaf,
    OUT U8 *BucketIndex);

ESTATUS
KiInsertTimer(
    IN KTIMER_LIST *TimerList,
    IN KTIMER *Timer,
    IN KTIMER_TYPE Type,
    IN U64 ExpirationTimeRelative);

ESTATUS
KiLookupTimer(
    IN KTIMER_LIST *TimerList,
    IN PKTIMER_LOOKUP Lookup,
    IN PVOID LookupContext,
    IN U64 AbsoluteTimeStart,
    IN U64 AbsoluteTimeEnd);

ESTATUS
KiStartTimer(
    IN KTIMER *Timer,
    IN KTIMER_TYPE Type,
    IN U64 ExpirationTimeRelative);

VOID
KiW32FakeInitSystem(
    VOID); // W32



