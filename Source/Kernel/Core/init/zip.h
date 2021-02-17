
#pragma once

/*
	4.3.6 Overall .ZIP file format:

	[local file header 1]
	[encryption header 1]
	[file data 1]
	[data descriptor 1]
	.
	.
	.
	[local file header n]
	[encryption header n]
	[file data n]
	[data descriptor n]
	[archive decryption header]
	[archive extra data record]
	[central directory header 1]
	.
	.
	.
	[central directory header n]
	[zip64 end of central directory record]
	[zip64 end of central directory locator]
	[end of central directory record]



	local file header signature     4 bytes  (0x04034b50)
	version needed to extract       2 bytes
	general purpose bit flag        2 bytes
	compression method              2 bytes
	last mod file time              2 bytes
	last mod file date              2 bytes
	crc-32                          4 bytes
	compressed size                 4 bytes
	uncompressed size               4 bytes
	file name length                2 bytes
	extra field length              2 bytes



	[central directory header]
	central file header signature   4 bytes  (0x02014b50)
	version made by                 2 bytes
	version needed to extract       2 bytes
	general purpose bit flag        2 bytes
	compression method              2 bytes
	last mod file time              2 bytes
	last mod file date              2 bytes
	crc-32                          4 bytes
	compressed size                 4 bytes
	uncompressed size               4 bytes
	file name length                2 bytes
	extra field length              2 bytes
	file comment length             2 bytes
	disk number start               2 bytes
	internal file attributes        2 bytes
	external file attributes        4 bytes
	relative offset of local header 4 bytes

	file name (variable size)
	extra field (variable size)
	file comment (variable size)

*/

#pragma pack(push, 1)

#define	ZIP_SIGNATURE_LOCAL_FILE				0x04034b50
#define	ZIP_SIGNATURE_CENTRAL_DIRECTORY			0x02014b50
#define	ZIP_SIGNATURE_CENTRAL_DIRECTORY_END		0x06054b50


typedef struct _ZIP_LOCAL_FILE_HEADER {
	U32 Signature;	// 0x04034b50
	U16 VersionRequired;
	U16 Flags;
	U16 CompressionMethod;
	U16 LastModifiedFileTime;
	U16 LastModifiedFileDate;
	U32 Crc32;
	U32 CompressedSize;
	U32 UncompressedSize;
	U16 FileNameLength;
	U16 ExtraFieldLength;

	// U8 FileName[FileNameLength];
	// U8 ExtraField[ExtraFieldLength];

	// ...
	// U8 FileStream[CompressedSize];
	// ...
} ZIP_LOCAL_FILE_HEADER, *PZIP_LOCAL_FILE_HEADER;

// 50 4B 03 04 | 14 00 | 00 00 | 00 00 | 1C 10 | 5F 4D | 0C 52 EF D2 | C5 0C 00 00 | C5 0C 00 00 | 05 00 | 00 00 | 7A 69 70 2E 63
// 04034B50 0014 0000 0000 101C 5F4D D2EF520C 00000CC5 00000CC5 0005 0000 "zip.c"

typedef struct _ZIP_CENTRAL_DIRECTORY_HEADER {
	U32 Signature;	// 0x02014b50
	U16 VersionCreated;
	U16 VersionRequired;
	U16 Flags;
	U16 CompressionMethod;
	U16 LastModifiedFileTime;
	U16 LastModifiedFileDate;
	U32 Crc32;
	U32 CompressedSize;
	U32 UncompressedSize;
	U16 FileNameLength;
	U16 ExtraFieldLength;
	U16 FileCommentLength;
	U16 DiskNumberStart;
	U16 InternalFileAttributes;
	U32 ExternalFileAttributes;
	U32 RelativeOffsetToLocalHeader;

	// U8 FileName[FileNameLength];
	// U8 ExtraField[ExtraFieldLength];
	// U8 FileComment[FileCommentLength];
} ZIP_CENTRAL_DIRECTORY_HEADER, *PZIP_CENTRAL_DIRECTORY_HEADER;

// 50 4B 01 02 | 14 00 | 14 00 | 00 00 | 00 00 | 1C 10 | 5F 4D | 0C 52 EF D2 | 
// C5 0C 00 00 | C5 0C 00 00 | 05 00 | 24 00 | 00 00 | 00 00 | 00 00 | 20 00 00 00 | 00 00 00 00 | 7A 69 70 2E 63

typedef struct _ZIP_END_OF_CENTRAL_DIRECTORY {
	U32 Signature;	// 0x06054b50
	U16 DiskNumber;
	U16 CentralDirectoryStartDiskNumber;
	U16 CentralDirectoryCount;
	U16 TotalCentralDirectoryCount;
	U32 SizeOfCentralDirectory;
	U32 AbsoluteOffsetToCentralDirectoryStart;
	U16 CommentLength;
	// U8 Comment[CommentLength];
} ZIP_END_OF_CENTRAL_DIRECTORY, *PZIP_END_OF_CENTRAL_DIRECTORY;

// 50 4B 05 06 00 00 00 00 02 00 02 00 B5 00 00 00 23 12 00 00 00 00
// 50 4B 05 06 | 00 00 | 00 00 | 02 00 | 02 00 | B5 00 00 00 | 23 12 00 00 | 00 00
// => 06054B50 0000 0000 0002 0002 000000B5 | 00001223 | 0000

typedef union _ZIP_RECORD_HEADER {
	U32 Signature;
	ZIP_LOCAL_FILE_HEADER LocalFileHeader;
	ZIP_CENTRAL_DIRECTORY_HEADER CentralDirectoryHeader;
	ZIP_END_OF_CENTRAL_DIRECTORY CentralDirectoryEnd;
} ZIP_RECORD_HEADER, *PZIP_RECORD_HEADER;

#pragma pack(pop)

U32
KERNELAPI
ZipCrc32Message(
	IN U8 *Buffer,
	IN U32 Length);


//
// Zip Reader.
//


#define	ZIP_OFFSET_BEGIN		0
#define	ZIP_OFFSET_CURRENT		1
#define	ZIP_OFFSET_END			2


typedef struct _ZIP_CONTEXT {
	UPTR Buffer;
	U32 BufferLength;
	U32 Offset;

	U32 OffsetToCentralDirectoryEnd;
} ZIP_CONTEXT, *PZIP_CONTEXT;

BOOLEAN
KERNELAPI
ZipInitializeReaderContext(
	OUT ZIP_CONTEXT *ZipContext,
	IN PVOID Buffer,
	IN U32 BufferLength);

U32
KERNELAPI
ZipSetOffset(
	IN ZIP_CONTEXT *ZipContext,
	IN S32 Offset,
	IN U32 Type);
	
BOOLEAN
KERNELAPI
ZipGetRecord(
	IN ZIP_CONTEXT *ZipContext,
	OUT PVOID RecordHeader,
	OUT U32 *RecordHeaderLength,
	OUT U32 *RecordLength,
	OUT U32 *RecordOffset);

BOOLEAN
KERNELAPI
ZipLookupFile_U8(
	IN ZIP_CONTEXT *ZipContext,
	IN CHAR8 *FileName,
	OUT U32 *OffsetToCentralDirectory);

BOOLEAN
KERNELAPI
ZipGetFileAddress(
	IN ZIP_CONTEXT *ZipContext,
	IN U32 OffsetToCentralDirectory,
	OUT VOID **FileAddress,
	OUT U32 *FileSize);

