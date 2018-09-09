#pragma once

#include "Arch.h"
#include "Types.h"
#include "Macros.h"

#define	BIM_HEADER_SIGNATURE				TAG4(0xf0, 0x55, 'f', 's')

typedef struct _BIM_HEADER {
	U32 Signature;
	U8 MajorVersion;
	U8 MinorVersion;
	U8 Reserved;
	U8 OffsetToFileEntry;
	U32 SizeTotal;
	U32 SizeUsed;
} BIM_HEADER, *PBIM_HEADER;

#define	BIM_FIRST_FILE_HEADER(_ctx)			((PBIM_FILE_ENTRY)( ((PTR)(_ctx)) + (_ctx)->OffsetToFileEntry ))

#define	BIM_FILE_SIGNATURE					TAG4(0xbf, 0xf7, 'f', 'i')

#define	BIM_FILE_FLAG_READONLY				0x0001
#define	BIM_FILE_FLAG_HIDDEN				0x0002
#define	BIM_FILE_FLAG_SYSTEM				0x0004
#define	BIM_FILE_FLAG_DIRECTORY				0x4000
#define	BIM_FILE_FLAG_DELETED				0x8000

#define	BIM_MAX_FILENAME					520

typedef struct _BIM_FILE_ENTRY {
	U32 Signature;
	U16 Flags;					// BIM_FILE_FLAG_XXX
	U16 OffsetToFileName;
	U16 OffsetToFileData;
	U16 SizeOfFileName;
	U32 SizeOfFileData;

	U64 LastCreatedFileTime;
} BIM_FILE_ENTRY, *PBIM_FILE_ENTRY;

//
// BIM Context.
//

typedef
PTR
(CALLCONV *PBIM_ALLOCATE_MEMORY)(
	IN U32 AllocationSize);

typedef
VOID
(CALLCONV *PBIM_FREE_MEMORY)(
	IN PTR AllocatedMemory);

typedef
BOOLEAN
(CALLCONV *PBIM_DUMP_IMAGE)(
	IN PBIM_HEADER ImageBuffer, 
	IN U32 SizeOfImage, 
	IN PTR DumpImageContext);

#define	BIM_IMAGE_MODE_READONLY					0x00000001
#define	BIM_IMAGE_MODE_WRITEONLY				0x00000002

typedef struct _BIM_CONTEXT {
	U32 ImageMode;
	U32 SizeOfImage;
	PBIM_HEADER ImageBuffer;

	PBIM_ALLOCATE_MEMORY AllocateMemory;
	PBIM_FREE_MEMORY FreeMemory;

	struct
	{
		BIM_HEADER Header;
	} Cache;

	struct
	{
		PBIM_DUMP_IMAGE DumpImage;
		PTR DumpImageContext;
	} Debug;
} BIM_CONTEXT, *PBIM_CONTEXT;

#define	BIM_IMAGE_ALLOCATED(_ctx)			( (_ctx)->ImageBuffer != NULL )

//
// Find Files.
//

typedef struct _BIM_FILE_HANDLE {
	U32 OffsetToFileEntry;	// Offset to BIM_FILE_ENTRY
	U32 OffsetToNextFileEntry;
	U16 Flags;				// BIM_FILE_FLAG_XXX
	U16 OffsetToFileDataFromFileEntry;
	U32 SizeOfFileData;
	U64 LastCreatedFileTime;
	CHAR8 FilePath[BIM_MAX_FILENAME];
} BIM_FILE_HANDLE, *PBIM_FILE_HANDLE;


//
// Functions.
//

BOOLEAN
CALLCONV
BimInitializeContext(
	OUT PBIM_CONTEXT Context,
	IN PBIM_ALLOCATE_MEMORY AllocateMemory,
	IN PBIM_FREE_MEMORY FreeMemory,
	IN PBIM_DUMP_IMAGE DumpImage,
	IN PTR DumpImageContext);

VOID
CALLCONV
BimDestroyContext(
	IN PBIM_CONTEXT Context);

BOOLEAN
CALLCONV
BimCreateImage(
	IN PBIM_CONTEXT Context,
	OPTIONAL IN PTR ImageBuffer,
	IN U32 SizeOfImage);

BOOLEAN
CALLCONV
BimDumpImage(
	IN PBIM_CONTEXT Context);

BOOLEAN
CALLCONV
BimFreeImage(
	IN PBIM_CONTEXT Context);

BOOLEAN
CALLCONV
BimFlushImage(
	IN PBIM_CONTEXT Context);

U32
CALLCONV
BimRawGetAccessibleBytes(
	IN PBIM_CONTEXT Context,
	IN U32 Offset,
	IN U32 BytesToAccess);

U32
CALLCONV
BimFileGetAccessibleBytes(
	IN PBIM_CONTEXT Context,
	IN PBIM_FILE_HANDLE Handle,
	IN U32 FileOffset,
	IN U32 BytesToAccess);

U32
CALLCONV
BimRawReadWrite(
	IN PBIM_CONTEXT Context,
	IN U32 Offset,
	IN OUT PTR Buffer,
	IN U32 BytesToReadWrite,
	IN BOOLEAN ReadOperation,
	IN BOOLEAN PartialOperation);

U32
CALLCONV
BimFileReadWrite(
	IN PBIM_CONTEXT Context,
	IN PBIM_FILE_HANDLE Handle,
	IN U32 FileOffset,
	IN OUT PTR Buffer,
	IN U32 BytesToReadWrite,
	IN BOOLEAN ReadOperation,
	IN BOOLEAN PartialOperation);

BOOLEAN
CALLCONV
BimLoadHeader(
	IN PBIM_CONTEXT Context,
	OUT PBIM_HEADER Buffer);

BOOLEAN
CALLCONV
BimIsValidFileEntry(
	IN PBIM_CONTEXT Context,
	IN U32 OffsetToFileEntry,
	OUT PBIM_FILE_ENTRY Buffer);

BOOLEAN
CALLCONV
BimCompareFilePath(
	IN CHAR8 *FilePath1,
	IN CHAR8 *FilePath2,
	IN U32 FilePathLength1,
	IN U32 FilePathLength2);

U32
CALLCONV
BimGetFilePathLength(
	IN CHAR8 *FilePath);

PBIM_FILE_HANDLE
CALLCONV
BimCreateFile(
	IN PBIM_CONTEXT Context,
	IN CHAR8 *FilePath,
	IN U32 FileFlags,
	IN U32 FileSize);

BOOLEAN
CALLCONV
BimHandleFromFileEntry(
	IN PBIM_CONTEXT Context,
	IN PBIM_FILE_HANDLE Handle,
	IN U32 OffsetToFileEntry);

PBIM_FILE_HANDLE
CALLCONV
BimGetFirstFile(
	IN PBIM_CONTEXT Context);

BOOLEAN
CALLCONV
BimGetNextFile(
	IN PBIM_CONTEXT Context,
	IN PBIM_FILE_HANDLE Handle);

VOID
CALLCONV
BimCloseFile(
	IN PBIM_CONTEXT Context,
	IN PBIM_FILE_HANDLE Handle);

PBIM_FILE_HANDLE
CALLCONV
BimOpenFile(
	IN PBIM_CONTEXT Context,
	IN CHAR8 *FilePath);

PBIM_FILE_HANDLE
CALLCONV
BimOpenFileByFileEntryOffset(
	IN PBIM_CONTEXT Context,
	IN U32 OffsetToFileEntry);

