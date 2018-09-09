
#include <stdio.h>
#include <Windows.h>

#include "device.h"
#include "crc.h"

/*
	LBA 0: MBR (sector size)
	LBA 1: GPT Header Only (sector size)
	LBA 2~: GPT Entry (128 byte x 4 x 32(LBA count))
	...
	33 LBAs except MBR
*/

VOID
APIENTRY
PartGenerateGUID(
	OUT PGUID128 GUID)
{
	ULONG64 s1, s2, s3, s4, s5, s6;

	s1 = __rdtsc();
	s2 = ((ULONG64)GetCurrentThreadId() << 0x20) | GetTickCount();
	s3 = (s1 * 1234567) ^ s2 + 9876543;

	Sleep(0);

	s4 = __rdtsc();
	s5 = ((ULONG64)GetCurrentThreadId() << 0x20) | GetTickCount();
	s6 = (s5 * 0x2345678) ^ s4 + 0xfeadbeef;

	((PULONG64)GUID)[0] = s3;
	((PULONG64)GUID)[1] = s6;
}

BOOLEAN
APIENTRY
PartInitializeGptHeader(
	IN struct _RAW_CONTEXT *Context,
	IN ULONG64 StartOffset)
{
	// random guid
	GUID128 DiskGUID = GUID_INITIALIZER(0x4ae8d3a9, 0x7883, 0x4e1c, 0x86, 0x50, 0x3b, 0x65, 0x9e, 0xf3, 0x57, 0xc5);

	GPT_HEADER PartHeader;

	ULONG SizeOfSector = Context->SizeOfSector;
	ULONG64 GPTMirrLBA = Context->SizeOfMedia / SizeOfSector - (34 - 1);
	ULONG64 StartingLBA = StartOffset / SizeOfSector;
	ULONG64 EndingLBA = GPTMirrLBA - 1;

	PVOID GPTEntries;

	if (EndingLBA < StartingLBA)
		return FALSE;

	if (StartingLBA < 34)
		return FALSE;

	ZeroMemory(&PartHeader, sizeof(PartHeader));
	strcpy(PartHeader.Signature, "EFI PART");
	PartHeader.Revision = 0x10000;
	PartHeader.SizeOfHeader = sizeof(PartHeader);
	PartHeader.Crc32 = 0;
	PartHeader.ReservedZero = 0;
	PartHeader.LBA = 1;
	PartHeader.AlternateLBA = GPTMirrLBA;
	PartHeader.FirstUsableLBA = StartingLBA;
	PartHeader.LastUsableLBA = EndingLBA;
	PartHeader.DiskGUID = DiskGUID;
	PartHeader.PartitionEntryLBA = 2;
	PartHeader.NumberOfPartitionEntries = 128;
	PartHeader.SizeOfPartitionEntry = sizeof(GPT_PARTITION_ENTRY_HEADER);
	PartHeader.Crc32PartitionEntryArray = 0;

	GPTEntries = malloc(SizeOfSector * 32);
	if (!GPTEntries)
		return FALSE;

	ZeroMemory(GPTEntries, SizeOfSector * 32);

	memcpy(&Context->GPTHeader[0], &PartHeader, sizeof(PartHeader));

	PartHeader.LBA = GPTMirrLBA;
	PartHeader.AlternateLBA = GPTMirrLBA;

	memcpy(&Context->GPTHeader[1], &PartHeader, sizeof(PartHeader));

	Context->GPTEntries = GPTEntries;

	return TRUE;
}

BOOLEAN
APIENTRY
PartSetGptEntry(
	IN struct _RAW_CONTEXT *Context,
	IN ULONG PartitionIndex, 
	IN ULONG64 StartOffset, 
	IN ULONG64 SizeOfPartition, 
	IN ULONG64 Attributes, 
	IN BOOLEAN SystemPartition)
{
	GPT_PARTITION_ENTRY_HEADER EntryHeader;
	GUID128 PartTypeEspGUID = GUID_GPT_EFI_SYSTEM_PARTITION;

	ULONG SizeOfSector = Context->SizeOfSector;
	ULONG64 StartingLBA = StartOffset / SizeOfSector;
	ULONG64 EndingLBA = (StartOffset + SizeOfPartition) / SizeOfSector - 1;

	if (!Context->GPTEntries)
		return FALSE;

	if (EndingLBA < StartingLBA)
		return FALSE;

	ZeroMemory(&EntryHeader, sizeof(EntryHeader));
	EntryHeader.Attributes = Attributes;
	EntryHeader.StartingLBA = StartingLBA;
	EntryHeader.EndingLBA = EndingLBA;

	if (SystemPartition)
		EntryHeader.PartitionTypeGUID = PartTypeEspGUID;

	PartGenerateGUID(&EntryHeader.UniquePartitionGUID);

	// FIXME : Check whether partition entry is out of range or not
	*(PGPT_PARTITION_ENTRY_HEADER)(
		(PCHAR)Context->GPTEntries + Context->GPTHeader[0].SizeOfPartitionEntry * PartitionIndex) =
		EntryHeader;

	return TRUE;
}

BOOLEAN
APIENTRY
PartSetGptEntrySystemPartition(
	IN struct _RAW_CONTEXT *Context,
	IN ULONG PartitionIndex,
	IN BOOLEAN SystemPartition)
{
	PGPT_PARTITION_ENTRY_HEADER EntryHeader;
	GUID128 PartTypeEspGUID = GUID_GPT_EFI_SYSTEM_PARTITION;

	if (!Context->GPTEntries)
		return FALSE;

	EntryHeader = (PGPT_PARTITION_ENTRY_HEADER)(
		(PCHAR)Context->GPTEntries + Context->GPTHeader[0].SizeOfPartitionEntry * PartitionIndex);

	if (SystemPartition)
		EntryHeader->PartitionTypeGUID = PartTypeEspGUID;
	else
		ZeroMemory(&EntryHeader->PartitionTypeGUID, sizeof(EntryHeader->PartitionTypeGUID));

	return TRUE;
}

BOOLEAN
APIENTRY
PartWriteGpt(
	IN struct _RAW_CONTEXT *Context)
{
	ULONG Crc32PartitionEntryArray = 0;
	ULONG ChecksumLength = 0;
	ULONG SizeOfSector = Context->SizeOfSector;
	ULONG SizeOfEntries = Context->GPTHeader[0].SizeOfPartitionEntry * Context->GPTHeader[0].NumberOfPartitionEntries;
	ULONG64 StartingMirrLBA = Context->GPTHeader[0].AlternateLBA;

	if (!Context->GPTEntries)
		return FALSE;

	ChecksumLength = Context->GPTHeader[0].NumberOfPartitionEntries * Context->GPTHeader[0].SizeOfPartitionEntry;
	Crc32PartitionEntryArray = CrcMessage(0, (PUCHAR)Context->GPTHeader, ChecksumLength);

	Context->GPTHeader[0].Crc32PartitionEntryArray = Crc32PartitionEntryArray;
	Context->GPTHeader[1].Crc32PartitionEntryArray = Crc32PartitionEntryArray;

	Context->GPTHeader[0].Crc32 = CrcMessage(0, (PUCHAR)&Context->GPTHeader[0], sizeof(Context->GPTHeader[0]));
	Context->GPTHeader[1].Crc32 = CrcMessage(0, (PUCHAR)&Context->GPTHeader[1], sizeof(Context->GPTHeader[1]));

	//
	// Write to LBA 1 ~ 33 / -33 ~ -1.
	//

	Context->SetPosition(Context->hDevice, 1 * SizeOfSector);
	Context->WriteBlock(Context->hDevice, &Context->GPTHeader[0], sizeof(Context->GPTHeader[0]), NULL);

	Context->SetPosition(Context->hDevice, 2 * SizeOfSector);
	Context->WriteBlock(Context->hDevice, Context->GPTEntries, SizeOfEntries, NULL);

	Context->SetPosition(Context->hDevice, StartingMirrLBA * SizeOfSector);
	Context->WriteBlock(Context->hDevice, &Context->GPTHeader[1], sizeof(Context->GPTHeader[1]), NULL);

	Context->SetPosition(Context->hDevice, (StartingMirrLBA + 1) * SizeOfSector);
	Context->WriteBlock(Context->hDevice, Context->GPTEntries, SizeOfEntries, NULL);

	return TRUE;
}

BOOLEAN
APIENTRY
PartGetGptEntryHeader(
	IN struct _RAW_CONTEXT *Context,
	OUT PGPT_PARTITION_ENTRY_HEADER PartitionEntryHeader)
{
	PGPT_PARTITION_ENTRY_HEADER Entry;
	ULONG SizeOfSector;

	if (!Context->GPTEntries)
		return FALSE;

	SizeOfSector = Context->SizeOfSector;
	Entry = (PGPT_PARTITION_ENTRY_HEADER)(
		(PCHAR)Context->GPTEntries + Context->GPTHeader[0].SizeOfPartitionEntry * Context->SelectedPartition);

	*PartitionEntryHeader = *Entry;

	return TRUE;
}

BOOLEAN
APIENTRY
PartGetAbsoluteOffset(
	IN struct _RAW_CONTEXT *Context,
	IN ULONG64 OffsetRelative,
	IN UINT32 Length, 
	OUT PULONG64 OffsetAbsolute)
{
	PGPT_PARTITION_ENTRY_HEADER Entry;
	ULONG SizeOfSector;
	ULONG64 StartingOffset;
	ULONG64 EndingOffset;
	ULONG64 ReturnOffsetAbsolute;

	if (!Context->GPTEntries)
		return FALSE;

	SizeOfSector = Context->SizeOfSector;

	Entry = (PGPT_PARTITION_ENTRY_HEADER)(
		(PCHAR)Context->GPTEntries + Context->GPTHeader[0].SizeOfPartitionEntry * Context->SelectedPartition);

	StartingOffset = Entry->StartingLBA * SizeOfSector;
	EndingOffset = (Entry->EndingLBA + 1) * SizeOfSector - 1;
	ReturnOffsetAbsolute = StartingOffset + OffsetRelative;

	if (StartingOffset < EndingOffset &&
		StartingOffset <= ReturnOffsetAbsolute && ReturnOffsetAbsolute <= EndingOffset &&
		StartingOffset <= ReturnOffsetAbsolute + Length &&
		ReturnOffsetAbsolute + Length <= EndingOffset && 
		ReturnOffsetAbsolute + Length >= ReturnOffsetAbsolute)
	{
		*OffsetAbsolute = ReturnOffsetAbsolute;
		return TRUE;
	}

	return FALSE;
}

BOOLEAN
APIENTRY
PartReadPartition(
	IN struct _RAW_CONTEXT *Context,
	IN ULONG64 Offset, 
	IN PVOID Buffer,
	IN UINT32 BufferLength,
	OPTIONAL OUT UINT32 *BytesRead)
{
	ULONG64 OffsetAbsolute;

	if (!PartGetAbsoluteOffset(Context, Offset, BufferLength, &OffsetAbsolute))
		return FALSE;

	return Context->SetPosition(Context->hDevice, OffsetAbsolute) && 
		Context->ReadBlock(Context->hDevice, Buffer, BufferLength, BytesRead);
}

BOOLEAN
APIENTRY
PartWritePartition(
	IN struct _RAW_CONTEXT *Context,
	IN ULONG64 Offset,
	IN PVOID Buffer,
	IN UINT32 BufferLength,
	OPTIONAL OUT UINT32 *BytesWritten)
{
	ULONG64 OffsetAbsolute;

	if (!PartGetAbsoluteOffset(Context, Offset, BufferLength, &OffsetAbsolute))
		return FALSE;

	return Context->SetPosition(Context->hDevice, OffsetAbsolute) &&
		Context->WriteBlock(Context->hDevice, Buffer, BufferLength, BytesWritten);
}

BOOLEAN
APIENTRY
PartFlushPartition(
	IN struct _RAW_CONTEXT *Context)
{
	return Context->Flush(Context->hDevice);
}

