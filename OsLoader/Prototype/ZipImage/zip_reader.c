
#include <stdio.h>
#include <windows.h>
#include <Shlwapi.h>
#include "Arch/Arch.h"

#include "zip.h"

#define	ZipGetPointer(_zctx)		( (UPTR)((UPTR)(_zctx)->Buffer + (_zctx)->Offset) )



static const U32 ZipCrc32Table[256] =
{
#if 0
	0x00000000L, 0x96300777L, 0x2c610eeeL, 0xba510999L, 0x19c46d07L,
	0x8ff46a70L, 0x35a563e9L, 0xa395649eL, 0x3288db0eL, 0xa4b8dc79L,
	0x1ee9d5e0L, 0x88d9d297L, 0x2b4cb609L, 0xbd7cb17eL, 0x072db8e7L,
	0x911dbf90L, 0x6410b71dL, 0xf220b06aL, 0x4871b9f3L, 0xde41be84L,
	0x7dd4da1aL, 0xebe4dd6dL, 0x51b5d4f4L, 0xc785d383L, 0x56986c13L,
	0xc0a86b64L, 0x7af962fdL, 0xecc9658aL, 0x4f5c0114L, 0xd96c0663L,
	0x633d0ffaL, 0xf50d088dL, 0xc8206e3bL, 0x5e10694cL, 0xe44160d5L,
	0x727167a2L, 0xd1e4033cL, 0x47d4044bL, 0xfd850dd2L, 0x6bb50aa5L,
	0xfaa8b535L, 0x6c98b242L, 0xd6c9bbdbL, 0x40f9bcacL, 0xe36cd832L,
	0x755cdf45L, 0xcf0dd6dcL, 0x593dd1abL, 0xac30d926L, 0x3a00de51L,
	0x8051d7c8L, 0x1661d0bfL, 0xb5f4b421L, 0x23c4b356L, 0x9995bacfL,
	0x0fa5bdb8L, 0x9eb80228L, 0x0888055fL, 0xb2d90cc6L, 0x24e90bb1L,
	0x877c6f2fL, 0x114c6858L, 0xab1d61c1L, 0x3d2d66b6L, 0x9041dc76L,
	0x0671db01L, 0xbc20d298L, 0x2a10d5efL, 0x8985b171L, 0x1fb5b606L,
	0xa5e4bf9fL, 0x33d4b8e8L, 0xa2c90778L, 0x34f9000fL, 0x8ea80996L,
	0x18980ee1L, 0xbb0d6a7fL, 0x2d3d6d08L, 0x976c6491L, 0x015c63e6L,
	0xf4516b6bL, 0x62616c1cL, 0xd8306585L, 0x4e0062f2L, 0xed95066cL,
	0x7ba5011bL, 0xc1f40882L, 0x57c40ff5L, 0xc6d9b065L, 0x50e9b712L,
	0xeab8be8bL, 0x7c88b9fcL, 0xdf1ddd62L, 0x492dda15L, 0xf37cd38cL,
	0x654cd4fbL, 0x5861b24dL, 0xce51b53aL, 0x7400bca3L, 0xe230bbd4L,
	0x41a5df4aL, 0xd795d83dL, 0x6dc4d1a4L, 0xfbf4d6d3L, 0x6ae96943L,
	0xfcd96e34L, 0x468867adL, 0xd0b860daL, 0x732d0444L, 0xe51d0333L,
	0x5f4c0aaaL, 0xc97c0dddL, 0x3c710550L, 0xaa410227L, 0x10100bbeL,
	0x86200cc9L, 0x25b56857L, 0xb3856f20L, 0x09d466b9L, 0x9fe461ceL,
	0x0ef9de5eL, 0x98c9d929L, 0x2298d0b0L, 0xb4a8d7c7L, 0x173db359L,
	0x810db42eL, 0x3b5cbdb7L, 0xad6cbac0L, 0x2083b8edL, 0xb6b3bf9aL,
	0x0ce2b603L, 0x9ad2b174L, 0x3947d5eaL, 0xaf77d29dL, 0x1526db04L,
	0x8316dc73L, 0x120b63e3L, 0x843b6494L, 0x3e6a6d0dL, 0xa85a6a7aL,
	0x0bcf0ee4L, 0x9dff0993L, 0x27ae000aL, 0xb19e077dL, 0x44930ff0L,
	0xd2a30887L, 0x68f2011eL, 0xfec20669L, 0x5d5762f7L, 0xcb676580L,
	0x71366c19L, 0xe7066b6eL, 0x761bd4feL, 0xe02bd389L, 0x5a7ada10L,
	0xcc4add67L, 0x6fdfb9f9L, 0xf9efbe8eL, 0x43beb717L, 0xd58eb060L,
	0xe8a3d6d6L, 0x7e93d1a1L, 0xc4c2d838L, 0x52f2df4fL, 0xf167bbd1L,
	0x6757bca6L, 0xdd06b53fL, 0x4b36b248L, 0xda2b0dd8L, 0x4c1b0aafL,
	0xf64a0336L, 0x607a0441L, 0xc3ef60dfL, 0x55df67a8L, 0xef8e6e31L,
	0x79be6946L, 0x8cb361cbL, 0x1a8366bcL, 0xa0d26f25L, 0x36e26852L,
	0x95770cccL, 0x03470bbbL, 0xb9160222L, 0x2f260555L, 0xbe3bbac5L,
	0x280bbdb2L, 0x925ab42bL, 0x046ab35cL, 0xa7ffd7c2L, 0x31cfd0b5L,
	0x8b9ed92cL, 0x1daede5bL, 0xb0c2649bL, 0x26f263ecL, 0x9ca36a75L,
	0x0a936d02L, 0xa906099cL, 0x3f360eebL, 0x85670772L, 0x13570005L,
	0x824abf95L, 0x147ab8e2L, 0xae2bb17bL, 0x381bb60cL, 0x9b8ed292L,
	0x0dbed5e5L, 0xb7efdc7cL, 0x21dfdb0bL, 0xd4d2d386L, 0x42e2d4f1L,
	0xf8b3dd68L, 0x6e83da1fL, 0xcd16be81L, 0x5b26b9f6L, 0xe177b06fL,
	0x7747b718L, 0xe65a0888L, 0x706a0fffL, 0xca3b0666L, 0x5c0b0111L,
	0xff9e658fL, 0x69ae62f8L, 0xd3ff6b61L, 0x45cf6c16L, 0x78e20aa0L,
	0xeed20dd7L, 0x5483044eL, 0xc2b30339L, 0x612667a7L, 0xf71660d0L,
	0x4d476949L, 0xdb776e3eL, 0x4a6ad1aeL, 0xdc5ad6d9L, 0x660bdf40L,
	0xf03bd837L, 0x53aebca9L, 0xc59ebbdeL, 0x7fcfb247L, 0xe9ffb530L,
	0x1cf2bdbdL, 0x8ac2bacaL, 0x3093b353L, 0xa6a3b424L, 0x0536d0baL,
	0x9306d7cdL, 0x2957de54L, 0xbf67d923L, 0x2e7a66b3L, 0xb84a61c4L,
	0x021b685dL, 0x942b6f2aL, 0x37be0bb4L, 0xa18e0cc3L, 0x1bdf055aL,
	0x8def022dL
#else
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
	0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
	0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
	0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
	0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
	0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
	0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
	0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
	0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
	0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
	0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
	0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
	0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
	0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
	0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
	0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
	0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
	0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
	0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
	0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
	0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
#endif
};

#if 0
U32
KERNELAPI
ZipCrc32Update(
	IN U32 Crc,
	IN U8 *Buffer,
	IN U32 Length)
{
	U32 c;
	U32 i;

#if 0
	for (c = Crc, i = 0; i < Length; i++)
		c = ZipCrc32Table[(c ^ Buffer[i]) & 0xff] ^ (c >> 8);
#else
	for (c = Crc, i = 0; i < Length; i++)
	{
		// c ^= Buffer[i]
		c = ZipCrc32Table[(c ^ Buffer[i]) & 0xff] ^ (c >> 8);
	}
#endif

	return c;
}
#endif

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
	IN U8 *FilePath1,
	IN U8 *FilePath2, 
	IN U32 Length1, 
	IN U32 Length2)
{
	U8 *s1 = FilePath1;
	U8 *s2 = FilePath2;
	U32 Offset = 0;
	U32 MaxLength = Length1 > Length2 ? Length1 : Length2;



	while (Offset < MaxLength)
	{
		U8 c1 = *s1++;
		U8 c2 = *s2++;

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
	U64 RangeStart = Offset;
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
		ZipContext, Pointer - (UPTR)ZipContext->Buffer, Length);
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
	IN U8 *FileName, 
	OUT U32 *OffsetToCentralDirectory)
{
	ZIP_RECORD_HEADER Header;
	U32 HeaderLength;
	U32 RecordLength;
	U32 RecordOffset;

	U32 CentralDirectoryStartOffset = 0;
	U32 CentralDirectoryCount = 0;
	U32 Count = 0;

	U32 FileNameLength = strlen(FileName);

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
		U8 *CentralDirectoryFileName;

		if (!ZipGetRecord(ZipContext, &Header, &HeaderLength, &RecordLength, &RecordOffset))
			break;

		if (Header.Signature != ZIP_SIGNATURE_CENTRAL_DIRECTORY)
			break;

		ZipSetOffset(ZipContext, RecordOffset, ZIP_OFFSET_BEGIN);

		if (!ZipAccessibleRange(ZipContext, RecordLength))
			break;

		ZipSetOffset(ZipContext, sizeof(Header.CentralDirectoryHeader), ZIP_OFFSET_CURRENT);
		CentralDirectoryFileName = (U8 *)ZipGetPointer(ZipContext);
		ZipSetOffset(ZipContext, RecordOffset + RecordLength, ZIP_OFFSET_BEGIN);

		printf("Record Offset 0x%08x, FileName `%.*s' ...\n", 
			RecordOffset, Header.CentralDirectoryHeader.FileNameLength, CentralDirectoryFileName);

		if (ZipCompareFilePathL_U8(CentralDirectoryFileName, FileName, Header.CentralDirectoryHeader.FileNameLength, FileNameLength))
		{
			printf("Found it!\n");
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


