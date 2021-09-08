#pragma once

#pragma pack(push, 8)

typedef struct _DLIST_ENTRY {
	struct _DLIST_ENTRY *Prev;
	struct _DLIST_ENTRY *Next;
} DLIST_ENTRY, *PDLIST_ENTRY;

#pragma pack(pop)

VOID
KERNELAPI
DListInitializeHead(
	IN PDLIST_ENTRY Head);

VOID
KERNELAPI
DListInsertAfter(
	IN PDLIST_ENTRY Head,
	IN PDLIST_ENTRY Entry);

VOID
KERNELAPI
DListInsertBefore(
	IN PDLIST_ENTRY Head,
	IN PDLIST_ENTRY Entry);

BOOLEAN
KERNELAPI
DListIsEmpty(
	IN PDLIST_ENTRY Head);

VOID
KERNELAPI
DListRemoveEntry(
	IN PDLIST_ENTRY Entry);

