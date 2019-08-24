#pragma once

#pragma pack(push, 8)

typedef struct _DLIST_ENTRY {
	struct _DLIST_ENTRY *Prev;
	struct _DLIST_ENTRY *Next;
} DLIST_ENTRY, *PDLIST_ENTRY;

#pragma pack(pop)

VOID
KERNELAPI
InitializeDListHead(
	IN PDLIST_ENTRY Head);

VOID
KERNELAPI
InsertDListAfter(
	IN PDLIST_ENTRY Head,
	IN PDLIST_ENTRY Entry);

VOID
KERNELAPI
InsertDListBefore(
	IN PDLIST_ENTRY Head,
	IN PDLIST_ENTRY Entry);

BOOLEAN
KERNELAPI
IsDListEmpty(
	IN PDLIST_ENTRY Head);

VOID
KERNELAPI
RemoveDListEntry(
	IN PDLIST_ENTRY Entry);

