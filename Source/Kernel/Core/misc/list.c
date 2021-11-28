
#include <base/base.h>
#include <misc/list.h>

VOID
KERNELAPI
DListInitializeHead(
	IN PDLIST_ENTRY Head)
{
	// Before : ? - Head - ?
	// After  : ... - Head - Head - Head - ...
	Head->Prev = Head;
	Head->Next = Head;
}

VOID
KERNELAPI
DListInsertAfter(
	IN PDLIST_ENTRY Head,
	IN PDLIST_ENTRY Entry)
{
	PDLIST_ENTRY HeadNext = Head->Next;
	PDLIST_ENTRY EntryPrev = Entry->Prev;

	HeadNext->Prev = EntryPrev;
	EntryPrev->Next = HeadNext;

	Head->Next = Entry;
	Entry->Prev = Head;
}

VOID
KERNELAPI
DListInsertBefore(
	IN PDLIST_ENTRY Head,
	IN PDLIST_ENTRY Entry)
{
	PDLIST_ENTRY HeadPrev = Head->Prev;
	PDLIST_ENTRY EntryPrev = Entry->Prev;

	HeadPrev->Next = Entry;
	EntryPrev->Next = Head;

	Head->Prev = EntryPrev;
	Entry->Prev = HeadPrev;
}

BOOLEAN
KERNELAPI
DListIsEmpty(
	IN PDLIST_ENTRY Head)
{
	if (Head->Next == Head->Prev)
	{
		// ASSERT( Head == Head->Next );
		return TRUE;
	}

	return FALSE;
}

VOID
KERNELAPI
DListRemoveEntry(
	IN PDLIST_ENTRY Entry)
{
	// Before : ... - Prev - Entry - Next - ... 
	// After  : Entry* - Entry - Entry*

	PDLIST_ENTRY EntryPrev = Entry->Prev;
	PDLIST_ENTRY EntryNext = Entry->Next;

	EntryPrev->Next = EntryNext;
	EntryNext->Prev = EntryPrev;

	Entry->Next = Entry;
	Entry->Prev = Entry;
}

VOID
KERNELAPI
DListMoveAfter(
    IN OUT PDLIST_ENTRY ListHeadDest,
    IN OUT PDLIST_ENTRY ListHeadSource)
{
    PDLIST_ENTRY SourcePrev = ListHeadSource->Prev;
    PDLIST_ENTRY SourceNext = ListHeadSource->Next;
    PDLIST_ENTRY DestPrev = ListHeadDest->Prev;

    DestPrev->Next = SourceNext;
    SourceNext->Prev = DestPrev;

    SourcePrev->Next = ListHeadDest;
    ListHeadDest->Prev = SourcePrev;

    ListHeadSource->Next = ListHeadSource;
    ListHeadSource->Prev = ListHeadSource;
}
