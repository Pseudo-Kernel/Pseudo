#include <stdio.h>
#include <windows.h>

// http://bytepointer.com/resources/win16_ne_exe_format_win3.0.htm

#define	FONTRACE								printf


#pragma pack(push, 1)

typedef struct _FON_DOS_HEADER {
	USHORT Magic;	// 'MZ'
	UCHAR Reserved[0x3a];
	ULONG Offset;
} FON_DOS_HEADER, *PFON_DOS_HEADER;

typedef struct _FON_NE_HEADER {
	USHORT Magic;	// 'NE'
	UCHAR MajorLinkerVersion;
	UCHAR MinorLinkerVersion;
	USHORT OffsetToEntryTable;
	USHORT SizeOfEntryTable;
	ULONG Crc;
	USHORT Flags;
	USHORT AutomaticDataSegIndex;
	USHORT InitHeapSize;
	USHORT InitStackSize;
	ULONG FarEntryPoint;
	ULONG FarStackBase;
	USHORT SegmentCount;
	USHORT ModuleReferences;
	USHORT SizeOfNonResidentNameTable;
	USHORT OffsetToSegmentTable;
	USHORT OffsetToResourceTable;
	USHORT OffsetToResidentNamesTable;
	USHORT OffsetToModuleReferenceTable;
	USHORT OffsetToImportedNamesTable;
	ULONG OffsetToNonResidentNamesTable;

	USHORT Reserved1;
	USHORT FileAlignmentShift;
	USHORT NumberOfResourceTables;
	UCHAR Reserved2[1 + 1 + 2 + 2 + 2 + 2];
} FON_NE_HEADER, *PFON_NE_HEADER;

typedef struct _FON_TYPE_ENTRY {
	USHORT OffsetToResourceData; // relative to BOF.
	USHORT SizeOfResource;
	USHORT Flags;
	USHORT ResourceId; // integer type if msb is set, otherwise: offset to the type string (relative to offset table)
	ULONG Reserved;
} FON_TYPE_ENTRY, *PFON_TYPE_ENTRY;

typedef struct _FON_TYPEINFO_BLOCK {
	USHORT TypeId; // integer type if msb is set, otherwise: offset to the type string (relative to offset table)
	USHORT NumberOfResources;
	ULONG Reserved;
	// FON_TYPE_ENTRY Type[];
} FON_TYPEINFO_BLOCK, *PFON_TYPEINFO_BLOCK;

typedef struct _FON_RESOURCE_TABLE {
	USHORT AlignmentShift;

//	FON_TYPEINFO_BLOCK TypeInfoBlock[];
//	UCHAR TypeOrNameLength;
//	CHAR TypeName[TypeOrNameLength];
} FON_RESOURCE_TABLE, *PFON_RESOURCE_TABLE;

typedef struct _FON_HEADER {
	// http://archive.is/alHwy

	USHORT Version;
	ULONG FileSize;
	UCHAR Copyright[60];
	USHORT Type; // raster font if Type[0] is cleared
	USHORT NominalPointSize;
	USHORT NominalVertDpi;
	USHORT NominalHoriDpi;
	USHORT Ascent; // distance to baseline from top

	USHORT InternalLeading;
	USHORT ExternalLeading;
	UCHAR Italic; // italic if Italic[0] is set
	UCHAR Underline; // underline if Underline[0] is set
	UCHAR StrikeOut;
	USHORT Weight; // weight on a scale of 1 to 1000
	UCHAR Charset; // character set

	USHORT PixWidth; // 0 if varies
	USHORT PixHeight;

	UCHAR PitchAndFamily;
	USHORT AvgWidth;
	USHORT MaxWidth;

	// Fields that we're interested...
	UCHAR FirstChar;
	UCHAR LastChar;
	UCHAR DefaultChar;
	UCHAR BreakChar;
	USHORT WidthBytes;

	ULONG Device;
	ULONG Face;
	ULONG BitsPointer;
	ULONG BitsOffset; // absolute offset if Type[3] is set.
	                  // see document.
	                  // http://archive.is/alHwy#selection-1633.7729-1633.8604
	UCHAR NotUsed;

	// V 3.x
	struct
	{
		ULONG GlyphFlags;
		USHORT GlobalASpace;
		USHORT GlobalBSpace;
		USHORT GlobalCSpace;

		ULONG ColorPointer; // color table
		UCHAR Reserved1[16];
	} Extend;
	// ULONG CharTable[]; { ??? }
} FON_HEADER, *PFON_HEADER;

typedef struct _FON_GLYPHENTRY {
	USHORT Width;
	USHORT OffsetToBits;
} FON_GLYPHENTRY, *PFON_GLYPHENTRY;

typedef struct _FON_GLYPHENTRY_V3 {
	USHORT Width;
	ULONG OffsetToBits;
} FON_GLYPHENTRY_V3, *PFON_GLYPHENTRY_V3;

#pragma pack(pop)

#define	FON_LOOKUP_BY_PIXEL_X		0
#define	FON_LOOKUP_BY_PIXEL_Y		1
#define	FON_LOOKUP_BY_POINT			2

#define	FON_DFF_FIXED				0x01
#define	FON_DFF_PROPORTIONAL		0x02


ULONG gPixBuf[1024 * 60];
ULONG gBufWidth = 1024;
ULONG gBufHeight = 60;
ULONG gBufScanLine = 1024 * 4;


VOID
APIENTRY
RefreshPixBufScreen(
	VOID)
{
	HWND hWnd = GetConsoleWindow();
	HDC hDC = GetDC(hWnd);
	HDC hMemDC = CreateCompatibleDC(hDC);

	HBITMAP hBitmap = CreateCompatibleBitmap(hDC, gBufWidth, gBufHeight);
	SetBitmapBits(hBitmap, sizeof(gPixBuf), gPixBuf);

	SelectObject(hMemDC, hBitmap);

	BitBlt(hDC, 0, 0, gBufWidth, gBufHeight, hMemDC, 0, 0, SRCCOPY);

	DeleteDC(hMemDC);
	DeleteObject(hBitmap);

	ReleaseDC(hWnd, hDC);
}


VOID
APIENTRY
FonCharBlt(
	IN UCHAR *Bits, // size: ((CharWidth + 7) >> 3) * CharHeight
	IN ULONG CharWidth,
	IN ULONG CharHeight,
	IN ULONG X,
	IN ULONG Y,
	IN ULONG *BltBuffer,
	IN ULONG BufferWidth,
	IN ULONG BufferHeight,
	IN ULONG BltBytesPerScanLine,
	IN ULONG PixelText,
	IN ULONG PixelBackground,
	IN BOOLEAN TransparentBackground)
{
	ULONG i, j, k;
	ULONG u, v;

	u = (CharWidth + 0x07) >> 3;
	v = CharHeight;

	for (i = 0; i < u; i++) // x
	{
		for (j = 0; j < v; j++) // y
		{
			UCHAR b = *Bits++;

			for (k = 0; k < 8; k++)
			{
				ULONG DestX = X + k + (i << 3);
				ULONG DestY = Y + j;

				if (DestX >= X + CharWidth)
					break;

				// CharBit[7 - k, j] -> Buf[X + k + (i << 3), Y + j]
				if (DestX >= BufferWidth || DestY >= BufferHeight)
					break; // out of range

				if (b & (1 << (7 - k)))
					BltBuffer[DestX + DestY * BltBytesPerScanLine / sizeof(__int32)] = PixelText;
				else if (!TransparentBackground)
					BltBuffer[DestX + DestY * BltBytesPerScanLine / sizeof(__int32)] = PixelBackground;
			}
		}
	}
}

BOOLEAN
APIENTRY
FonPrintText(
	IN PFON_HEADER FontFile, 
	IN CHAR *Text, 
	IN ULONG X, 
	IN ULONG Y, 
	IN ULONG *BltBuffer, 
	IN ULONG BufferWidth, 
	IN ULONG BufferHeight, 
	IN ULONG BltBytesPerScanLine, 
	IN ULONG PixelText, 
	IN ULONG PixelBackground, 
	IN BOOLEAN TransparentBackground)
{
	CHAR *p = Text;
	union
	{
		ULONG_PTR Pointer;
		FON_GLYPHENTRY *Entry;
		FON_GLYPHENTRY_V3 *EntryV3;
	} u;
	UCHAR MajorVersion = (UCHAR)(FontFile->Version >> 0x08);

	if (MajorVersion <= 2)
	{
		// Version 2.x
		u.Pointer = (ULONG_PTR)&FontFile->Extend;
	}
	else
	{
		// Version 3.x
		if (!(FontFile->Extend.GlyphFlags & FON_DFF_FIXED))
			return FALSE;

		u.Pointer = (ULONG_PTR)(FontFile + 1);
	}

	//
	// Glyph Entry Table:
	// [Entry for FirstChar] [Entry for FirstChar+1] ... [Entry for LastChar] [Entry for BLANK]
	//

	while (*p)
	{
		UCHAR Char = *p++;
		USHORT Index;

		USHORT Width;
		ULONG Offset;
		USHORT BitmapLength;

		Index = Char - FontFile->FirstChar;

		if (Index > FontFile->LastChar)
			Index = FontFile->LastChar;

		if (MajorVersion <= 2)
		{
			Width = u.Entry[Index].Width;
			Offset = u.Entry[Index].OffsetToBits;
		}
		else
		{
			Width = u.EntryV3[Index].Width;
			Offset = u.EntryV3[Index].OffsetToBits;
		}

		BitmapLength = ((Width + 0x07) >> 3) * FontFile->PixHeight;

		FONTRACE("Char 0x%02hhx -> Width %d\n", Char, Width);

		{
			ULONG i;
			UCHAR *p = (UCHAR *)FontFile + Offset;

			FonCharBlt(
				p, Width, FontFile->PixHeight, X, Y, BltBuffer,
				BufferWidth, BufferHeight, BltBytesPerScanLine, 
				PixelText, PixelBackground, TransparentBackground);

			X += Width;

			FONTRACE("Bitmap: Length %d\n", BitmapLength);
			for (i = 0; i < BitmapLength; i++)
				FONTRACE("%02hhx ", *((UCHAR *)FontFile + Offset + i));
			FONTRACE("\n");
		}
	}

	return TRUE;
}


BOOLEAN
APIENTRY
FonLookupFont(
	IN PVOID Buffer,
	IN ULONG BufferLength, 
	IN ULONG LookupBy, 
	IN USHORT Value, 
	OUT FON_HEADER **FontFile)
{
	FON_DOS_HEADER *DosHeader;
	FON_NE_HEADER *NeHeader;
	IMAGE_OS2_HEADER *Ne2;
	FON_HEADER *FonHeader;
	FON_RESOURCE_TABLE *ResourceTable;
	FON_TYPEINFO_BLOCK *TypeInfo;
	ULONG i;

	if (BufferLength < sizeof(*DosHeader))
		return FALSE;

	DosHeader = (FON_DOS_HEADER *)Buffer;
	if (DosHeader->Magic != IMAGE_DOS_SIGNATURE)
		return FALSE;

	if (BufferLength < DosHeader->Offset || 
		BufferLength < DosHeader->Offset + sizeof(*NeHeader))
		return FALSE;

	NeHeader = (FON_NE_HEADER *)((ULONG_PTR)Buffer + DosHeader->Offset);
	Ne2 = (IMAGE_OS2_HEADER *)NeHeader;
	if (NeHeader->Magic != IMAGE_OS2_SIGNATURE)
		return FALSE;

	// http://bytepointer.com/resources/win16_ne_exe_format_win3.0.htm
	// [DosHeader]
	// [NeHeader]
	//  + [SegTable] [ResourceTable] [ResNameTable] [ModRefTable] [ImportedNamesTable] [EntryTable] [NonResNameTable]
	//  + [Seg1Data + Seg1Info] [Seg2Data + Seg2Info] ... [SegNData + SegNInfo]

	ResourceTable = (FON_RESOURCE_TABLE *)((ULONG_PTR)NeHeader + NeHeader->OffsetToResourceTable);

	for (TypeInfo = (FON_TYPEINFO_BLOCK *)(ResourceTable + 1); ; )
	{
		FON_TYPE_ENTRY *Type = (FON_TYPE_ENTRY *)(TypeInfo + 1);
		UCHAR TypeOrNameLength = 0;
		CHAR *String1 = NULL;
		CHAR *TypeOrNameString = NULL;

		// Is end of the table?
		if (!TypeInfo->TypeId)
			break;

		/*
			RT_ACCELERATOR	Accelerator table
			RT_BITMAP		Bitmap
			RT_CURSOR		Cursor
			RT_DIALOG		Dialog box
			RT_FONT			Font component
			RT_FONTDIR		Font directory
			RT_GROUP_CURSOR	Cursor directory
			RT_GROUP_ICON	Icon directory
			RT_ICON			Icon
			RT_MENU			Menu
			RT_RCDATA		Resource data
			RT_STRING		String table
		*/

		switch (TypeInfo->TypeId)
		{
		case 0x8000 | 0x08: //(USHORT)RT_FONT:
			FONTRACE("Type is RT_FONT\n");
			break;
		default:
			FONTRACE("Type is 0x%04x\n", TypeInfo->TypeId);
		}

		if (!(TypeInfo->TypeId & 0x8000))
			String1 = (CHAR *)ResourceTable + TypeInfo->TypeId;

		for (i = 0; i < TypeInfo->NumberOfResources; i++)
		{
			FONTRACE("Type[%d].OffsetToResourceData 0x%04x\n", i, Type[i].OffsetToResourceData);
			FONTRACE("Type[%d].SizeOfResource 0x%04x\n", i, Type[i].SizeOfResource);
			FONTRACE("Type[%d].Flags 0x%04x\n", i, Type[i].Flags);
			FONTRACE("Type[%d].ResourceId 0x%04x\n", i, Type[i].ResourceId);

			if (TypeInfo->TypeId == (0x8000 | 0x08)) // RT_FONT
			{
				// https://www.chiark.greenend.org.uk/~sgtatham/fonts/mkwinfont
				ULONG Offset = Type[i].OffsetToResourceData << ResourceTable->AlignmentShift;
				ULONG SizeOfResource = Type[i].SizeOfResource << ResourceTable->AlignmentShift;
				BOOLEAN Matched = FALSE;

				FonHeader = (FON_HEADER *)((ULONG_PTR)Buffer + Offset);

				if (SizeOfResource < sizeof(*FonHeader))
					continue;

				if (FonHeader->FileSize > SizeOfResource)
					continue;

				// We only supports raster, fixed pitch font
				if ((FonHeader->Type & 0x01) ||
					FonHeader->PitchAndFamily != (3 << 4) ||
					!FonHeader->PixHeight || !FonHeader->PixWidth)
					continue;

				if ((FonHeader->Version >> 0x08) > 2)
				{
					// File version is 3.x but does not contain fixed font
					if (!(FonHeader->Extend.GlyphFlags & FON_DFF_FIXED))
						continue;
				}

				switch (LookupBy)
				{
				case FON_LOOKUP_BY_PIXEL_X:
					if (FonHeader->PixWidth == Value)
						Matched = TRUE;
					break;
				case FON_LOOKUP_BY_PIXEL_Y:
					if (FonHeader->PixHeight == Value)
						Matched = TRUE;
					break;
				case FON_LOOKUP_BY_POINT:
					if (FonHeader->NominalPointSize == Value)
						Matched = TRUE;
					break;
				}

				if (Matched)
				{
					FONTRACE("matched! pixel %dx%d, point %d\n", 
						FonHeader->PixWidth, FonHeader->PixHeight, FonHeader->NominalPointSize);

					*FontFile = FonHeader;

					return TRUE;
				}
			}
		}

		TypeInfo = (FON_TYPEINFO_BLOCK *)((ULONG_PTR)TypeInfo
			+ sizeof(*TypeInfo) + sizeof(*Type) * TypeInfo->NumberOfResources);

		FONTRACE("\n");
	}

	FONTRACE("\n");

	return FALSE;
}

int main()
{
	FILE *in = fopen(
		//"Bm437_Wyse700a-2y.fon",
		//"Bm437_Verite_9x16.fon", 
		"bootfont.fon", 
		"rb");
	int size = 0;
	void *filebuf;
	FON_HEADER *fonhdr;

	memset(gPixBuf, 0xffffffff, sizeof(gPixBuf));

	if (!in)
	{
		FONTRACE("failed to open the file\n");
		return -1;
	}

	fseek(in, 0, SEEK_END);
	size = ftell(in);
	fseek(in, 0, SEEK_SET);

	filebuf = malloc(size);
	fread(filebuf, size, 1, in);

	fclose(in);

	if (FonLookupFont(filebuf, size, FON_LOOKUP_BY_POINT, 14, &fonhdr))
	{
		// KiLoaderBlock = 0xffff80007df80000
		FonPrintText(fonhdr, "123456789012345678901234567890123456789012345678901234567890", 0, 0,
			gPixBuf, gBufWidth, gBufHeight, gBufScanLine, 0x00ff00, 0x0000ff, FALSE);

		for (;;)
		{
			Sleep(10);
			RefreshPixBufScreen();
		}
	}

	return 0;
}
