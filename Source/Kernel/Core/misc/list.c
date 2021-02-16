
#include <base/base.h>
#include <misc/list.h>

VOID
KERNELAPI
InitializeDListHead(
	IN PDLIST_ENTRY Head)
{
	// Before : ? - Head - ?
	// After  : ... - Head - Head - Head - ...
	Head->Prev = Head;
	Head->Next = Head;
}

VOID
KERNELAPI
InsertDListAfter(
	IN PDLIST_ENTRY Head,
	IN PDLIST_ENTRY Entry)
{
	// Entry  : ... - EntryPrev - Entry - EntryNext - ...
	// Before : Prev - Head - Next
	// After  : Prev - Head - [Entry - EntryNext - ... - EntryPrev] - Next
	//
	// Entry  : ... - 10 -  0 -  1 - ...
	// Before : ... - 19 - 20 - 11 - ...
	// After  : ... - 19 - 20 -[ 0 - ... - 10]- 11 - ...

	PDLIST_ENTRY HeadNext = Head->Next;
	PDLIST_ENTRY EntryPrev = Entry->Prev;

	HeadNext->Prev = EntryPrev;
	EntryPrev->Next = HeadNext;

	Head->Next = Entry;
	Entry->Prev = Head;
}

VOID
KERNELAPI
InsertDListBefore(
	IN PDLIST_ENTRY Head,
	IN PDLIST_ENTRY Entry)
{
	// Entry  : ... - EntryPrev - Entry - EntryNext - ...
	// Before : Prev - Head - Next
	// After  : Prev - [Entry - EntryNext - ... - EntryPrev] - Head - Next

	PDLIST_ENTRY HeadPrev = Head->Prev;
	PDLIST_ENTRY EntryPrev = Entry->Prev;

	HeadPrev->Next = Entry;
	EntryPrev->Next = Head;

	Head->Prev = EntryPrev;
	Entry->Prev = HeadPrev;
}

BOOLEAN
KERNELAPI
IsDListEmpty(
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
RemoveDListEntry(
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

