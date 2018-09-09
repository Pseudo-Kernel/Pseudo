#pragma once

#pragma pack(push, 1)

typedef struct _GUID128 {
	UINT32 v1;
	UINT16 v2;
	UINT16 v3;
	UINT8 v4[8];
} GUID128, *PGUID128;

typedef struct _GPT_HEADER {
	UINT8 Signature[8];		// "EFI PART"
	UINT32 Revision;
	UINT32 SizeOfHeader;
	UINT32 Crc32;
	UINT32 ReservedZero;
	UINT64 LBA;
	UINT64 AlternateLBA;
	UINT64 FirstUsableLBA;
	UINT64 LastUsableLBA;
	GUID128 DiskGUID;
	UINT64 PartitionEntryLBA;
	UINT32 NumberOfPartitionEntries;
	UINT32 SizeOfPartitionEntry;
	UINT32 Crc32PartitionEntryArray;
//	UINT8 Reserved[BlockSize - 92];
} GPT_HEADER, *PGPT_HEADER;

typedef struct _GPT_PARTITION_ENTRY_HEADER {
	GUID128 PartitionTypeGUID;
	GUID128 UniquePartitionGUID;
	UINT64 StartingLBA;
	UINT64 EndingLBA;
	UINT64 Attributes;
	UINT16 PartitionName[36];
//	UINT8 Reserved[SizeOfPartitionEntry - 128];
} GPT_PARTITION_ENTRY_HEADER, *PGPT_PARTITION_ENTRY_HEADER;

#define	GUID_INITIALIZER(_v1, _v2, _v3, _vc1, _vc2, _vc3, _vc4, _vc5, _vc6, _vc7, _vc8)	\
	{ (_v1), (_v2), (_v3), (_vc1), (_vc2), (_vc3), (_vc4), (_vc5), (_vc6), (_vc7), (_vc8) }
#define	GUID_GPT_UNUSED		\
	GUID_INITIALIZER(0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
#define	GUID_GPT_EFI_SYSTEM_PARTITION		\
	GUID_INITIALIZER(0xC12A7328, 0xF81F, 0x11D2, 0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B)
#define	GUID_GPT_LEGACY_MBR		\
	GUID_INITIALIZER(0x024DEE41, 0x33E7, 0x11D3, 0x9D, 0x69, 0x00, 0x08, 0xC7, 0x81, 0xF3, 0x9F)


#pragma pack(pop)

BOOLEAN
APIENTRY
PartInitializeGptHeader(
	IN struct _RAW_CONTEXT *Context,
	IN ULONG64 StartOffset);

BOOLEAN
APIENTRY
PartSetGptEntry(
	IN struct _RAW_CONTEXT *Context,
	IN ULONG PartitionIndex,
	IN ULONG64 StartOffset,
	IN ULONG64 SizeOfPartition,
	IN ULONG64 Attributes,
	IN BOOLEAN SystemPartition);

BOOLEAN
APIENTRY
PartGetGptEntryHeader(
	IN struct _RAW_CONTEXT *Context,
	OUT PGPT_PARTITION_ENTRY_HEADER PartitionEntryHeader);

BOOLEAN
APIENTRY
PartWriteGpt(
	IN struct _RAW_CONTEXT *Context);

BOOLEAN
APIENTRY
PartSetGptEntrySystemPartition(
	IN struct _RAW_CONTEXT *Context,
	IN ULONG PartitionIndex,
	IN BOOLEAN SystemPartition);


BOOLEAN
APIENTRY
PartReadPartition(
	IN struct _RAW_CONTEXT *Context,
	IN ULONG64 Offset,
	IN PVOID Buffer,
	IN UINT32 BufferLength,
	OPTIONAL OUT UINT32 *BytesRead);

BOOLEAN
APIENTRY
PartWritePartition(
	IN struct _RAW_CONTEXT *Context,
	IN ULONG64 Offset,
	IN PVOID Buffer,
	IN UINT32 BufferLength,
	OPTIONAL OUT UINT32 *BytesWritten);

BOOLEAN
APIENTRY
PartFlushPartition(
	IN struct _RAW_CONTEXT *Context);

