#pragma once

#pragma pack(push, 8)

typedef struct _DLIST_ENTRY {
	struct _DLIST_ENTRY *Prev;
	struct _DLIST_ENTRY *Next;
} DLIST_ENTRY, *PDLIST_ENTRY;

#pragma pack(pop)

VOID
DListInitializeHead(
	IN PDLIST_ENTRY Head);

VOID
DListInsertAfter(
	IN PDLIST_ENTRY Head,
	IN PDLIST_ENTRY Entry);

VOID
DListInsertBefore(
	IN PDLIST_ENTRY Head,
	IN PDLIST_ENTRY Entry);

BOOLEAN
DListIsEmpty(
	IN PDLIST_ENTRY Head);

VOID
DListRemoveEntry(
	IN PDLIST_ENTRY Entry);

VOID
DListMoveAfter(
    IN OUT PDLIST_ENTRY ListHeadDest,
    IN OUT PDLIST_ENTRY ListHeadSource);

