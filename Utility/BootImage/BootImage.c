
#include <memory.h>
#include "BootImage.h"

BOOLEAN
CALLCONV
BimInitializeContext(
	OUT PBIM_CONTEXT Context,
	IN PBIM_ALLOCATE_MEMORY AllocateMemory,
	IN PBIM_FREE_MEMORY FreeMemory, 
	IN PBIM_DUMP_IMAGE DumpImage, 
	IN PTR DumpImageContext)
{
	if (!Context || !AllocateMemory || !FreeMemory)
		return FALSE;

	Context->ImageMode = 0;
	Context->ImageBuffer = NULL;
	Context->SizeOfImage = 0;
	Context->AllocateMemory = AllocateMemory;
	Context->FreeMemory = FreeMemory;

	Context->Debug.DumpImage = DumpImage;
	Context->Debug.DumpImageContext = DumpImageContext;

	return TRUE;
}

VOID
CALLCONV
BimDestroyContext(
	IN PBIM_CONTEXT Context)
{
	BimFreeImage(Context);

	memset(Context, 0, sizeof(*Context));
}

BOOLEAN
CALLCONV
BimCreateImage(
	IN PBIM_CONTEXT Context, 
	OPTIONAL IN PTR ImageBuffer, 
	IN U32 SizeOfImage)
{
	U32 SizeOfImageMinimum;
	PBIM_HEADER Buffer;

	SizeOfImageMinimum = sizeof(BIM_HEADER);

	if (SizeOfImageMinimum > SizeOfImage)
		return FALSE;

	Buffer = (PBIM_HEADER)Context->AllocateMemory(SizeOfImage);
	if (!Buffer)
		return FALSE;

	if (ImageBuffer)
	{
		memcpy(Buffer, ImageBuffer, SizeOfImage);

		if (!BimLoadHeader(Context, NULL))
		{
			Context->FreeMemory((PTR)Buffer);
			return FALSE;
		}

		Context->ImageMode = BIM_IMAGE_MODE_READONLY;
	}
	else
	{
		memset(Buffer, -1, SizeOfImage);

		Context->Cache.Header.Signature = BIM_HEADER_SIGNATURE;
		Context->Cache.Header.MajorVersion = 1;
		Context->Cache.Header.MinorVersion = 0;
		Context->Cache.Header.OffsetToFileEntry = sizeof(BIM_HEADER);
		Context->Cache.Header.Reserved = 0;
		Context->Cache.Header.SizeTotal = SizeOfImage;
		Context->Cache.Header.SizeUsed = sizeof(BIM_HEADER);

		Context->ImageMode = BIM_IMAGE_MODE_WRITEONLY;
	}

	Context->SizeOfImage = SizeOfImage;
	Context->ImageBuffer = Buffer;

	return TRUE;
}

BOOLEAN
CALLCONV
BimDumpImage(
	IN PBIM_CONTEXT Context)
{
	if (!BIM_IMAGE_ALLOCATED(Context))
		return FALSE;

	BimFlushImage(Context);

	return Context->Debug.DumpImage(
		Context->ImageBuffer, Context->SizeOfImage, Context->Debug.DumpImageContext);
}

BOOLEAN
CALLCONV
BimFreeImage(
	IN PBIM_CONTEXT Context)
{
	if (!BIM_IMAGE_ALLOCATED(Context))
		return FALSE;

	Context->FreeMemory((PTR)Context->ImageBuffer);
	Context->ImageBuffer = NULL;
	Context->SizeOfImage = 0;

	return TRUE;
}

BOOLEAN
CALLCONV
BimFlushImage(
	IN PBIM_CONTEXT Context)
{
	if (Context->ImageMode == BIM_IMAGE_MODE_WRITEONLY)
	{
		if (BimRawReadWrite(Context, 0, (PTR)&Context->Cache.Header, sizeof(Context->Cache.Header), FALSE, FALSE) != 0)
			return TRUE;
	}

	return FALSE;
}

U32
CALLCONV
BimRawGetAccessibleBytes(
	IN PBIM_CONTEXT Context,
	IN U32 Offset,
	IN U32 BytesToAccess)
{
	U32 StartOffset;
	U32 EndOffset;
	U32 BytesAccessible;

	if (!BIM_IMAGE_ALLOCATED(Context))
		return 0;

	StartOffset = Offset;
	EndOffset = Offset + BytesToAccess - 1;

	if (StartOffset > EndOffset)
		return 0;

	if (EndOffset >= Context->Cache.Header.SizeTotal)
		EndOffset = Context->Cache.Header.SizeTotal - 1;

	// Check the boundary twice
	if (StartOffset > EndOffset)
		return 0;

	BytesAccessible = EndOffset - StartOffset + 1;

	return BytesAccessible;
}

U32
CALLCONV
BimFileGetAccessibleBytes(
	IN PBIM_CONTEXT Context,
	IN PBIM_FILE_HANDLE Handle,
	IN U32 FileOffset, 
	IN U32 BytesToAccess)
{
	U64 StartOffset64;
	U64 EndOffset64;
	U64 StartRange64;
	U64 EndRange64;
	U32 BytesAccessible;

	if (!BIM_IMAGE_ALLOCATED(Context))
		return 0;

	StartRange64 = Handle->OffsetToFileEntry + Handle->OffsetToFileDataFromFileEntry;
	EndRange64 = Handle->OffsetToFileEntry + Handle->OffsetToFileDataFromFileEntry + Handle->SizeOfFileData - 1;

	StartOffset64 = Handle->OffsetToFileEntry + Handle->OffsetToFileDataFromFileEntry + FileOffset;
	EndOffset64 = Handle->OffsetToFileEntry + Handle->OffsetToFileDataFromFileEntry + FileOffset + BytesToAccess - 1;

	if (StartRange64 > EndRange64 || StartOffset64 > EndOffset64 || 
		StartRange64 > StartOffset64 || 
		StartRange64 != (U32)StartRange64 || EndRange64 != (U32)EndRange64 || 
		StartOffset64 != (U32)StartOffset64 || EndOffset64 != (U32)EndOffset64)
		return 0;

	if (EndOffset64 > EndRange64)
		EndOffset64 = EndRange64;

	BytesAccessible = (U32)(EndOffset64 - StartOffset64 + 1);

	return BimRawGetAccessibleBytes(Context, (U32)StartOffset64, BytesAccessible);
}

U32
CALLCONV
BimRawReadWrite(
	IN PBIM_CONTEXT Context,
	IN U32 Offset,
	IN OUT PTR Buffer,
	IN U32 BytesToReadWrite, 
	IN BOOLEAN ReadOperation, 
	IN BOOLEAN PartialOperation)
{
	U32 BytesReadWritten;
	PTR Source;
	PTR Destination;

	if (!BIM_IMAGE_ALLOCATED(Context))
		return 0;

	BytesReadWritten = BimRawGetAccessibleBytes(Context, Offset, BytesToReadWrite);
	if (!BytesReadWritten)
		return 0;

	if (BytesToReadWrite != BytesReadWritten && !PartialOperation)
		return 0;

	if (ReadOperation)
	{
		Source = (PTR)((U8 *)Context->ImageBuffer + Offset);
		Destination = (PTR)Buffer;
	}
	else
	{
		Destination = (PTR)((U8 *)Context->ImageBuffer + Offset);
		Source = (PTR)Buffer;
	}

	memcpy((PVOID)Destination, (PVOID)Source, BytesReadWritten);

	return BytesReadWritten;
}

U32
CALLCONV
BimFileReadWrite(
	IN PBIM_CONTEXT Context,
	IN PBIM_FILE_HANDLE Handle, 
	IN U32 FileOffset,
	IN OUT PTR Buffer,
	IN U32 BytesToReadWrite,
	IN BOOLEAN ReadOperation,
	IN BOOLEAN PartialOperation)
{
	U32 BytesReadWritten;

	if (!BIM_IMAGE_ALLOCATED(Context))
		return 0;

	BytesReadWritten = BimFileGetAccessibleBytes(Context, Handle, FileOffset, BytesToReadWrite);
	if (!BytesReadWritten)
		return 0;

	if (BytesToReadWrite != BytesReadWritten && !PartialOperation)
		return 0;

	return BimRawReadWrite(Context,
		Handle->OffsetToFileEntry + Handle->OffsetToFileDataFromFileEntry + FileOffset,
		Buffer, BytesReadWritten, ReadOperation, PartialOperation);
}

BOOLEAN
CALLCONV
BimLoadHeader(
	IN PBIM_CONTEXT Context, 
	OUT PBIM_HEADER Buffer)
{
	BIM_HEADER Header;

	if (!BimRawReadWrite(Context, 0, (PTR)&Header, sizeof(BIM_HEADER), TRUE, FALSE))
		return FALSE;

	if (Header.Signature != BIM_HEADER_SIGNATURE)
		return FALSE;

	if (Header.MajorVersion != 1 || Header.MinorVersion != 0)
		return FALSE;

	if (Header.OffsetToFileEntry != sizeof(BIM_HEADER))
		return FALSE;

	if (Header.SizeUsed > Header.SizeTotal || 
		Header.SizeTotal != Context->SizeOfImage)
		return FALSE;

	Context->Cache.Header = Header;

	if (Buffer)
		*Buffer = Header;

	return TRUE;
}

BOOLEAN
CALLCONV
BimIsValidFileEntry(
	IN PBIM_CONTEXT Context,
	IN U32 OffsetToFileEntry, 
	OUT PBIM_FILE_ENTRY Buffer)
{
	BIM_FILE_ENTRY FileEntry;

	if (!BimRawReadWrite(Context, OffsetToFileEntry, (PTR)&FileEntry, sizeof(BIM_FILE_ENTRY), TRUE, FALSE))
		return FALSE;

	if (FileEntry.Signature != BIM_FILE_SIGNATURE)
		return FALSE;

	if (FileEntry.SizeOfFileName > BIM_MAX_FILENAME)
		return FALSE;

	if (Buffer)
		*Buffer = FileEntry;

	return TRUE;
}

BOOLEAN
CALLCONV
BimNormalizeCompareCharacter(
	IN CHAR8 C1,
	IN CHAR8 C2)
{
	C1 = ToLowerAlphabet(C1);
	C2 = ToLowerAlphabet(C2);

	if (C1 == '\\') C1 = '/';
	if (C2 == '\\') C2 = '/';

	return (BOOLEAN)(C1 == C2);
}

BOOLEAN
CALLCONV
BimCompareFilePath(
	IN CHAR8 *FilePath1,
	IN CHAR8 *FilePath2,
	IN U32 FilePathLength1, 
	IN U32 FilePathLength2)
{
	CHAR8 *s1 = FilePath1;
	CHAR8 *s2 = FilePath2;
	U32 l = FilePathLength1;

	if (FilePathLength1 != FilePathLength2)
		return FALSE;

	while (l > 0)
	{
		CHAR8 c1 = *s1++;
		CHAR8 c2 = *s2++;

		if (!BimNormalizeCompareCharacter(c1, c2))
			return FALSE;

		l--;
	}

	return TRUE;
}

U32
CALLCONV
BimGetFilePathLength(
	IN CHAR8 *FilePath)
{
	CHAR8 *p = FilePath;
	while (*p++);

	return (U32)(p - FilePath);
}

PBIM_FILE_HANDLE
CALLCONV
BimCreateFile(
	IN PBIM_CONTEXT Context,
	IN CHAR8 *FilePath,
	IN U32 FileFlags, 
	IN U32 FileSize)
{
	BIM_FILE_ENTRY FileEntry;
	U32 SizeOfFileName;
	U32 SizeOfFileData;
	U32 SizeOfReserve;
	U32 OffsetToFileEntry;
	U32 Offset;
	BOOLEAN Directory;
	PBIM_FILE_HANDLE Handle;

	if (Context->ImageMode != BIM_IMAGE_MODE_WRITEONLY)
		return FALSE;

	Directory = !!(FileFlags & BIM_FILE_FLAG_DIRECTORY);

	if (Directory)
		SizeOfFileData = 0;
	else
		SizeOfFileData = FileSize;

	SizeOfFileName = BimGetFilePathLength(FilePath);
	if (SizeOfFileName > BIM_MAX_FILENAME)
		return FALSE;

	SizeOfReserve = sizeof(BIM_FILE_ENTRY) + SizeOfFileName + SizeOfFileData;

	OffsetToFileEntry = Context->Cache.Header.SizeUsed;
	if (BimRawGetAccessibleBytes(Context, OffsetToFileEntry, SizeOfReserve) != SizeOfReserve)
		return FALSE;

	FileEntry.Signature = BIM_FILE_SIGNATURE;
	FileEntry.Flags = FileFlags;
	FileEntry.LastCreatedFileTime = 0;
	FileEntry.OffsetToFileName = sizeof(BIM_FILE_ENTRY);
	FileEntry.OffsetToFileData = sizeof(BIM_FILE_ENTRY) + SizeOfFileName;
	FileEntry.SizeOfFileData = SizeOfFileData;
	FileEntry.SizeOfFileName = SizeOfFileName;

	if (!BimRawReadWrite(Context, OffsetToFileEntry, (PTR)&FileEntry, sizeof(BIM_FILE_ENTRY), FALSE, FALSE))
		return FALSE;

	Offset = OffsetToFileEntry + sizeof(BIM_FILE_ENTRY);
	if (!BimRawReadWrite(Context, Offset, (PTR)FilePath, SizeOfFileName, FALSE, FALSE))
		return FALSE;

	Offset += SizeOfFileName + SizeOfFileData;

	Context->Cache.Header.SizeUsed = Offset;

	BimFlushImage(Context);

	Handle = BimOpenFileByFileEntryOffset(Context, OffsetToFileEntry);

	return Handle;
}

BOOLEAN
CALLCONV
BimHandleFromFileEntry(
	IN PBIM_CONTEXT Context,
	IN PBIM_FILE_HANDLE Handle, 
	IN U32 OffsetToFileEntry)
{
	BIM_FILE_ENTRY FileEntry;

	if (!BimIsValidFileEntry(Context, OffsetToFileEntry, &FileEntry))
		return FALSE;

	Handle->Flags = FileEntry.Flags;
	Handle->LastCreatedFileTime = FileEntry.LastCreatedFileTime;
	Handle->OffsetToFileDataFromFileEntry = FileEntry.OffsetToFileData;
	Handle->OffsetToFileEntry = OffsetToFileEntry;
	Handle->OffsetToNextFileEntry = OffsetToFileEntry + sizeof(BIM_FILE_ENTRY) + FileEntry.SizeOfFileName + FileEntry.SizeOfFileData;
	Handle->SizeOfFileData = FileEntry.SizeOfFileData;

	if (!BimRawReadWrite(Context, OffsetToFileEntry + FileEntry.OffsetToFileName,
		(PTR)Handle->FilePath, FileEntry.SizeOfFileName, TRUE, FALSE))
		return FALSE;

	Handle->FilePath[FileEntry.SizeOfFileName] = '\0';

	return TRUE;
}

PBIM_FILE_HANDLE
CALLCONV
BimGetFirstFile(
	IN PBIM_CONTEXT Context)
{
	PBIM_FILE_HANDLE Handle;

	Handle = (PBIM_FILE_HANDLE)Context->AllocateMemory(sizeof(*Handle));
	if (!Handle)
		return NULL;

	if (!BimHandleFromFileEntry(Context, Handle, sizeof(BIM_HEADER)))
	{
		Context->FreeMemory((PTR)Handle);
		return NULL;
	}

	return Handle;
}

BOOLEAN
CALLCONV
BimGetNextFile(
	IN PBIM_CONTEXT Context, 
	IN PBIM_FILE_HANDLE Handle)
{
	if (!BimHandleFromFileEntry(Context, Handle, Handle->OffsetToNextFileEntry))
		return FALSE;

	return TRUE;
}

VOID
CALLCONV
BimCloseFile(
	IN PBIM_CONTEXT Context,
	IN PBIM_FILE_HANDLE Handle)
{
	if (Handle)
		Context->FreeMemory((PTR)Handle);
}


PBIM_FILE_HANDLE
CALLCONV
BimOpenFile(
	IN PBIM_CONTEXT Context, 
	IN CHAR8 *FilePath)
{
	PBIM_FILE_HANDLE Handle;
	BOOLEAN FileFound;
	U32 LookupFilePathLength;

	if (!BIM_IMAGE_ALLOCATED(Context))
		return NULL;

	LookupFilePathLength = BimGetFilePathLength(FilePath);

	Handle = BimGetFirstFile(Context);
	FileFound = !!Handle;

	while (FileFound)
	{
		U32 CurrentFilePathLength = BimGetFilePathLength(Handle->FilePath);
		if (BimCompareFilePath(Handle->FilePath, FilePath, CurrentFilePathLength, LookupFilePathLength))
			return Handle;

		FileFound = BimGetNextFile(Context, Handle);
	}

	BimCloseFile(Context, Handle);

	return NULL;
}

PBIM_FILE_HANDLE
CALLCONV
BimOpenFileByFileEntryOffset(
	IN PBIM_CONTEXT Context,
	IN U32 OffsetToFileEntry)
{
	PBIM_FILE_HANDLE Handle;

	if (!BIM_IMAGE_ALLOCATED(Context))
		return NULL;

	Handle = (PBIM_FILE_HANDLE)Context->AllocateMemory(sizeof(*Handle));
	if (!Handle)
		return NULL;

	if (!BimHandleFromFileEntry(Context, Handle, OffsetToFileEntry))
	{
		Context->FreeMemory((PTR)Handle);
		return NULL;
	}

	return Handle;
}
