
#include <base/base.h>
#include <init/zip.h>

#define	ZIPTRACE					
#define	ZipGetPointer(_zctx)		( (UPTR)((UPTR)(_zctx)->Buffer + (_zctx)->Offset) )

U32
KERNELAPI
ZipCrc32Message(
	IN U8 *Buffer,
	IN U32 Length)
{
	U32 c;
	U32 i;
	S32 j;

	for (c = 0xffffffff, i = 0; i < Length; i++)
	{
		c ^= Buffer[i];
		for (j = 7; j >= 0; j--)
		{
			U32 Mask = (S32)(-((S32)c & 1));
			c = (c >> 1) ^ (0xedb88320 & Mask);
		}
	}

	return ~c;
}

BOOLEAN
KERNELAPI
ZipCompareFilePath_U8(
	IN U8 *FilePath1,
	IN U8 *FilePath2)
{
	U8 *s1 = FilePath1;
	U8 *s2 = FilePath2;

	while (*s1 && *s2)
	{
		U8 c1 = *s1++;
		U8 c2 = *s2++;

		if ('a' <= c1 && c1 <= 'z') c1 += 'A' - 'a';
		if ('a' <= c2 && c2 <= 'z') c2 += 'A' - 'a';

		if (c1 == '\\') c1 = '/';
		if (c2 == '\\') c2 = '/';

		if (c1 != c2)
			return FALSE;
	}

	return TRUE;
}

BOOLEAN
KERNELAPI
ZipCompareFilePathL_U8(
	IN CHAR8 *FilePath1,
	IN CHAR8 *FilePath2, 
	IN U32 Length1, 
	IN U32 Length2)
{
	CHAR8 *s1 = FilePath1;
	CHAR8 *s2 = FilePath2;
	U32 Offset = 0;
	U32 MaxLength = Length1 > Length2 ? Length1 : Length2;

	while (Offset < MaxLength)
	{
		CHAR8 c1 = *s1++;
		CHAR8 c2 = *s2++;

		if ('a' <= c1 && c1 <= 'z') c1 += 'A' - 'a';
		if ('a' <= c2 && c2 <= 'z') c2 += 'A' - 'a';

		if (c1 == '\\') c1 = '/';
		if (c2 == '\\') c2 = '/';

		if (Offset >= Length1 || Offset >= Length2)
			return (BOOLEAN)(Length1 == Length2);

		if (c1 != c2)
			return FALSE;

		if (!c1 || !c2)
			break;

		Offset++;
	}

	return TRUE;
}

static
BOOLEAN
KERNELAPI
ZipInitializeParse(
	IN ZIP_CONTEXT *ZipContext)
{
	ZIP_RECORD_HEADER Header;
	U32 HeaderLength;
	U32 RecordLength;
	U32 RecordOffset;

	ZipSetOffset(ZipContext, 0, ZIP_OFFSET_BEGIN);

	// Get End of Central Directory.
	while (ZipGetRecord(ZipContext, &Header, &HeaderLength, &RecordLength, &RecordOffset))
	{
		if (Header.Signature == ZIP_SIGNATURE_CENTRAL_DIRECTORY_END)
		{
			if (Header.CentralDirectoryEnd.DiskNumber !=
				Header.CentralDirectoryEnd.CentralDirectoryStartDiskNumber ||
				Header.CentralDirectoryEnd.CentralDirectoryCount !=
				Header.CentralDirectoryEnd.TotalCentralDirectoryCount)
			{
				// Multiple disk image not supported
				break;
			}

			ZipContext->OffsetToCentralDirectoryEnd = RecordOffset;
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN
KERNELAPI
ZipInitializeReaderContext(
	OUT ZIP_CONTEXT *ZipContext,
	IN PVOID Buffer,
	IN U32 BufferLength)
{
	ZIP_CONTEXT TempContext;

	TempContext.Buffer = (UPTR)Buffer;
	TempContext.BufferLength = BufferLength;
	TempContext.Offset = 0;
	TempContext.OffsetToCentralDirectoryEnd = 0;

	if (!ZipInitializeParse(&TempContext))
		return FALSE;

	*ZipContext = TempContext;

	return TRUE;
}

U32
KERNELAPI
ZipInternalAccessibleRange(
	IN ZIP_CONTEXT *ZipContext,
	IN U32 Offset,
	IN U32 Length)
{
	U64 RangeEnd = Offset + Length;

	// [0, BufferLength-1], [Offset, Offset+Length-1]
	// => [max(0, Offset), min(BufferLength-1, Offset+Length-1)]
	// => min(BufferLength-1, Offset+Length-1) - max(0, Offset) + 1

	U64 AccessibleStart = Offset;
	U64 AccessibleEnd = ZipContext->BufferLength;
	U64 AccessibleBytes = 0;

	if (AccessibleEnd > RangeEnd)
		AccessibleEnd = RangeEnd;

	AccessibleBytes = AccessibleEnd - AccessibleStart;

	if (AccessibleStart < AccessibleEnd && AccessibleBytes <= 0xffffffff)
		return (U32)AccessibleBytes;

	return 0;
}

U32
KERNELAPI
ZipAccessibleRange(
	IN ZIP_CONTEXT *ZipContext,
	IN U32 Length)
{
	return ZipInternalAccessibleRange(ZipContext, ZipContext->Offset, Length);
}

U32
KERNELAPI
ZipAccessibleRangeByPointer(
	IN ZIP_CONTEXT *ZipContext,
	IN UPTR Pointer,
	IN U32 Length)
{
	if (Pointer < (UPTR)ZipContext->Buffer ||
		Pointer >= (UPTR)ZipContext->Buffer + ZipContext->BufferLength)
	{
		return 0;
	}

	return ZipInternalAccessibleRange(
		ZipContext, (U32)(Pointer - (UPTR)ZipContext->Buffer), Length);
}


U32
KERNELAPI
ZipSetOffset(
	IN ZIP_CONTEXT *ZipContext,
	IN S32 Offset, 
	IN U32 Type)
{
	U32 PrevOffset = ZipContext->Offset;

	switch (Type)
	{
	case ZIP_OFFSET_BEGIN:
		ZipContext->Offset = Offset;
		break;
	case ZIP_OFFSET_CURRENT:
		ZipContext->Offset += Offset;
		break;
	case ZIP_OFFSET_END:
		ZipContext->Offset = ZipContext->BufferLength;
		break;
	}

	return PrevOffset;
}

BOOLEAN
KERNELAPI
ZipReadBytes(
	IN ZIP_CONTEXT *ZipContext,
	OUT U8 *Buffer,
	IN U32 BufferLength, 
	IN BOOLEAN Advance)
{
	U8 *SourceBuffer;
	U32 i;

	if (!ZipAccessibleRange(ZipContext, BufferLength))
		return FALSE;

	SourceBuffer = (U8 *)ZipGetPointer(ZipContext);

	for (i = 0; i < BufferLength; i++)
		Buffer[i] = SourceBuffer[i];

	if (Advance)
		ZipSetOffset(ZipContext, BufferLength, ZIP_OFFSET_CURRENT);

	return TRUE;
}

BOOLEAN
KERNELAPI
ZipRead_U32(
	IN ZIP_CONTEXT *ZipContext,
	OUT U32 *Value,
	IN BOOLEAN Advance)
{
	return ZipReadBytes(ZipContext, (U8 *)Value, sizeof(U32), Advance);
}

BOOLEAN
KERNELAPI
ZipGetRecord(
	IN ZIP_CONTEXT *ZipContext,
	OUT PVOID RecordHeader, 
	OUT U32 *RecordHeaderLength, 
	OUT U32 *RecordLength, 
	OUT U32 *RecordOffset)
{
	ZIP_RECORD_HEADER Header;
	U32 HeaderLength = 0;
	U32 AdvanceLength = 0;

	if (!ZipRead_U32(ZipContext, &Header.Signature, FALSE))
		return FALSE;

	switch (Header.Signature)
	{
	case ZIP_SIGNATURE_LOCAL_FILE:
		if (!ZipReadBytes(ZipContext, (U8 *)&Header.LocalFileHeader, sizeof(Header.LocalFileHeader), FALSE))
			return FALSE;

		if (Header.LocalFileHeader.Flags & ~0x800) // except language encoding flag
			return FALSE;

		if (Header.LocalFileHeader.CompressionMethod ||
			Header.LocalFileHeader.CompressedSize != Header.LocalFileHeader.UncompressedSize)
			return FALSE;

		HeaderLength = sizeof(Header.LocalFileHeader);
		AdvanceLength = sizeof(Header.LocalFileHeader)
			+ Header.LocalFileHeader.FileNameLength
			+ Header.LocalFileHeader.ExtraFieldLength
			+ Header.LocalFileHeader.CompressedSize;
		break;

	case ZIP_SIGNATURE_CENTRAL_DIRECTORY:
		if (!ZipReadBytes(ZipContext, (U8 *)&Header.CentralDirectoryHeader, sizeof(Header.CentralDirectoryHeader), FALSE))
			return FALSE;

		if (Header.CentralDirectoryHeader.Flags & ~0x800) // except language encoding flag
			return FALSE;

		if (Header.CentralDirectoryHeader.CompressionMethod ||
			Header.CentralDirectoryHeader.CompressedSize != Header.CentralDirectoryHeader.UncompressedSize)
			return FALSE;

		HeaderLength = sizeof(Header.CentralDirectoryHeader);
		AdvanceLength = sizeof(Header.CentralDirectoryHeader)
			+ Header.CentralDirectoryHeader.FileNameLength
			+ Header.CentralDirectoryHeader.ExtraFieldLength
			+ Header.CentralDirectoryHeader.FileCommentLength;
		break;

	case ZIP_SIGNATURE_CENTRAL_DIRECTORY_END:
		if (!ZipReadBytes(ZipContext, (U8 *)&Header.CentralDirectoryEnd, sizeof(Header.CentralDirectoryEnd), FALSE))
			return FALSE;

		HeaderLength = sizeof(Header.CentralDirectoryEnd);
		AdvanceLength = sizeof(Header.CentralDirectoryEnd);
		break;

	default:
		return FALSE;
	}

	if (RecordOffset)
		*RecordOffset = ZipSetOffset(ZipContext, AdvanceLength, ZIP_OFFSET_CURRENT);

	if (RecordHeader)
		memcpy(RecordHeader, &Header, HeaderLength);

	if (RecordHeaderLength)
		*RecordHeaderLength = HeaderLength;

	if (RecordLength)
		*RecordLength = AdvanceLength;

	return TRUE;
}

BOOLEAN
KERNELAPI
ZipLookupFile_U8(
	IN ZIP_CONTEXT *ZipContext,
	IN CHAR8 *FileName, 
	OUT U32 *OffsetToCentralDirectory)
{
	ZIP_RECORD_HEADER Header;
	U32 HeaderLength;
	U32 RecordLength;
	U32 RecordOffset;

	U32 CentralDirectoryStartOffset = 0;
	U32 CentralDirectoryCount = 0;
	U32 Count = 0;

	U32 FileNameLength = (U32)strlen(FileName);

	//
	// Read the End of the central directory (EOCD).
	//

	ZipSetOffset(ZipContext, ZipContext->OffsetToCentralDirectoryEnd, ZIP_OFFSET_BEGIN);

	if (!ZipGetRecord(ZipContext, &Header, NULL, NULL, &RecordOffset) ||
		RecordOffset != ZipContext->OffsetToCentralDirectoryEnd ||
		Header.Signature != ZIP_SIGNATURE_CENTRAL_DIRECTORY_END)
		return FALSE;

	CentralDirectoryStartOffset = Header.CentralDirectoryEnd.AbsoluteOffsetToCentralDirectoryStart;
	CentralDirectoryCount = Header.CentralDirectoryEnd.CentralDirectoryCount;

	//
	// Parse from the central directory start
	//

	ZipSetOffset(ZipContext, CentralDirectoryStartOffset, ZIP_OFFSET_BEGIN);

	do
	{
		CHAR8 *CentralDirectoryFileName;

		if (!ZipGetRecord(ZipContext, &Header, &HeaderLength, &RecordLength, &RecordOffset))
			break;

		if (Header.Signature != ZIP_SIGNATURE_CENTRAL_DIRECTORY)
			break;

		ZipSetOffset(ZipContext, RecordOffset, ZIP_OFFSET_BEGIN);

		if (!ZipAccessibleRange(ZipContext, RecordLength))
			break;

		ZipSetOffset(ZipContext, sizeof(Header.CentralDirectoryHeader), ZIP_OFFSET_CURRENT);
		CentralDirectoryFileName = (CHAR8 *)ZipGetPointer(ZipContext);
		ZipSetOffset(ZipContext, RecordOffset + RecordLength, ZIP_OFFSET_BEGIN);

		ZIPTRACE("Record Offset 0x%08x, FileName `%.*s' ...\n",
			RecordOffset, Header.CentralDirectoryHeader.FileNameLength, CentralDirectoryFileName);

		if (ZipCompareFilePathL_U8(CentralDirectoryFileName, FileName, Header.CentralDirectoryHeader.FileNameLength, FileNameLength))
		{
			ZIPTRACE("Found it!\n");
			*OffsetToCentralDirectory = RecordOffset;
			return TRUE;
		}

		Count++;
	} while (Count < CentralDirectoryCount);

	return FALSE;
}

BOOLEAN
KERNELAPI
ZipGetFileAddress(
	IN ZIP_CONTEXT *ZipContext,
	IN U32 OffsetToCentralDirectory,
	OUT VOID **FileAddress,
	OUT U32 *FileSize)
{
	ZIP_RECORD_HEADER Header;
	U32 RecordHeaderLength;
	U32 RecordLength;
	U32 LocalHeaderOffset;
	U32 CompressedSize;
	U32 FileOffset;

	ZipSetOffset(ZipContext, OffsetToCentralDirectory, ZIP_OFFSET_BEGIN);

	if (!ZipGetRecord(ZipContext, &Header, &RecordHeaderLength, &RecordLength, NULL) ||
		Header.Signature != ZIP_SIGNATURE_CENTRAL_DIRECTORY)
		return FALSE;

	LocalHeaderOffset = Header.CentralDirectoryHeader.RelativeOffsetToLocalHeader;
	CompressedSize = Header.CentralDirectoryHeader.CompressedSize;

	ZipSetOffset(ZipContext, LocalHeaderOffset, ZIP_OFFSET_BEGIN);

	if (!ZipGetRecord(ZipContext, &Header, &RecordHeaderLength, &RecordLength, NULL) ||
		Header.Signature != ZIP_SIGNATURE_LOCAL_FILE)
		return FALSE;

	FileOffset = LocalHeaderOffset
		+ sizeof(Header.LocalFileHeader)
		+ Header.LocalFileHeader.FileNameLength
		+ Header.LocalFileHeader.ExtraFieldLength;
	ZipSetOffset(ZipContext, FileOffset, ZIP_OFFSET_BEGIN);

	if (FileAddress)
		*FileAddress = (VOID *)ZipGetPointer(ZipContext);

	if (FileSize)
		*FileSize = CompressedSize;

	return TRUE;
}



