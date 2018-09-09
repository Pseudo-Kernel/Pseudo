#pragma once

#include "gpt.h"

#pragma pack(push, 1)

typedef struct _VHD_FOOTER_DATA {
	UCHAR Cookie[8];			// connectix
	UINT32 Features;			// Currently we use 0x02
	UINT32 FileFormatVersion;	// Currently we use 0x10000
	UINT64 DataOffset;			// Currently we use 0xffffffff (fixed disk)
	UINT32 TimeStamp;
	UINT32 CreatorApplication;
	UINT32 CreatorVersion;
	UINT32 CreatorHostOS;
	UINT64 OriginalSize;
	UINT64 CurrentSize;
	UINT32 DiskGeometry;
	UINT32 DiskType;
	UINT32 Checksum;
	UCHAR UUID[16];				// GUID
	UINT8 SavedState;
	UCHAR Reserved[427];
} VHD_FOOTER_DATA, *PVHD_FOOTER_DATA;

__inline UINT16 ToNet16(UINT16 Value)
{
	return
		((Value & 0xff00) >> 0x08) |
		((Value & 0x00ff) << 0x08);
}

__inline UINT32 ToNet32(UINT32 Value)
{
	return
		((Value & 0xff000000) >> 0x18) |
		((Value & 0x00ff0000) >> 0x08) |
		((Value & 0x0000ff00) << 0x08) |
		((Value & 0x000000ff) << 0x18);
}

#if 1
__inline UINT64 ToNet64(UINT64 Value)
{
	return
		(UINT64)ToNet32((UINT32)(Value >> 0x20)) |
		((UINT64)ToNet32((UINT32)Value) << 0x20);
}
#endif


#pragma pack(pop)


typedef
BOOLEAN
(APIENTRY *PRAW_READ_BLOCK)(
	IN HANDLE DeviceHandle,
	IN PVOID Buffer,
	IN UINT32 BufferLength,
	OPTIONAL OUT UINT32 *BytesRead);

typedef
BOOLEAN
(APIENTRY *PRAW_WRITE_BLOCK)(
	IN HANDLE DeviceHandle,
	IN PVOID Buffer,
	IN UINT32 BufferLength,
	OPTIONAL OUT UINT32 *BytesWritten);

typedef
BOOLEAN
(APIENTRY *PRAW_SET_POSITION)(
	IN HANDLE DeviceHandle,
	IN ULONG64 Position);

typedef
BOOLEAN
(APIENTRY *PRAW_FLUSH)(
	IN HANDLE DeviceHandle);

typedef struct _RAW_CONTEXT {
	// Physical Device.
	HANDLE hDevice;
	ULONG64 SizeOfMedia;
	ULONG SizeOfSector;
	PRAW_READ_BLOCK ReadBlock;
	PRAW_WRITE_BLOCK WriteBlock;
	PRAW_SET_POSITION SetPosition;
	PRAW_FLUSH Flush;

	// Partition.
	GPT_HEADER GPTHeader[2];						// Header, HeaderMirr
	struct _GPT_PARTITION_ENTRY_HEADER *GPTEntries;	// LBA 2 to 33 (32 LBAs)
	ULONG SelectedPartition;

	// FATxx File system.
	ULONG SectorsPerCluster;
} RAW_CONTEXT, *PRAW_CONTEXT;


