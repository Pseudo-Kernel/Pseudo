#include <Base.h>
#include <init/bootgfx.h>
#include <init/zip.h>

// http://bytepointer.com/resources/win16_ne_exe_format_win3.0.htm

#define	FONTRACE								//printf

typedef struct _BOOT_GFX {
	UPTR FrameBuffer; // Linear framebuffer address
	SIZE_T FrameBufferSize; // Framebuffer size
	UPTR ModesBuffer; // Mode list buffer
	U32 ModesBufferSize; // Mode list buffer size
	U32 ModeNumberCurrent; // Current mode number

	UPTR FontFile; // Boot font address (.fon format)
	SIZE_T FontFileSize; // Size of boot font

	FON_HEADER *BootFont; // .fnt address

	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *ModeInfo; // Current mode info

	// Used in text mode emulation
	U32 TextCursorX; // 0 to (ResX - 1)
	U32 TextCursorY; // 0 to (ResY - 1)
	U32 TextResolutionX;
	U32 TextResolutionY;
	U32 TextColor;
	U32 TextBackgroundColor;
} BOOT_GFX, *PBOOT_GFX;

BOOT_GFX PiBootGfx;


VOID
KERNELAPI
BootFonCharBlt(
	IN U8 *Bits, // size: ((CharWidth + 7) >> 3) * CharHeight
	IN U32 CharWidth,
	IN U32 CharHeight,
	IN U32 X,
	IN U32 Y,
	IN U32 *BltBuffer,
	IN U32 BufferWidth,
	IN U32 BufferHeight,
	IN U32 BltBytesPerScanLine,
	IN U32 PixelText,
	IN U32 PixelBackground,
	IN BOOLEAN TransparentBackground)
{
	U32 i, j, k;
	U32 u, v;

	u = (CharWidth + 0x07) >> 3;
	v = CharHeight;

	for (i = 0; i < u; i++) // x
	{
		for (j = 0; j < v; j++) // y
		{
			U8 b = *Bits++;

			for (k = 0; k < 8; k++)
			{
				U32 DestX = X + k + (i << 3);
				U32 DestY = Y + j;

				if (DestX >= X + CharWidth)
					break;

				// CharBit[7 - k, j] -> Buf[X + k + (i << 3), Y + j]
				if (DestX >= BufferWidth || DestY >= BufferHeight)
					break; // out of range

				if (b & (1 << (7 - k)))
					BltBuffer[DestX + DestY * BltBytesPerScanLine / sizeof(__int32)] = (PixelText & 0xffffff);
				else if (!TransparentBackground)
					BltBuffer[DestX + DestY * BltBytesPerScanLine / sizeof(__int32)] = (PixelBackground & 0xffffff);
			}
		}
	}
}

BOOLEAN
KERNELAPI
BootFonPrintTextN(
	IN PFON_HEADER FontFile, 
	IN CHAR8 *Text, 
	IN U32 TextLength, 
	IN U32 *X, 
	IN U32 Y, 
	IN U32 *BltBuffer, 
	IN U32 BufferWidth, 
	IN U32 BufferHeight, 
	IN U32 BltBytesPerScanLine, 
	IN U32 PixelText, 
	IN U32 PixelBackground, 
	IN BOOLEAN TransparentBackground, 
	OUT U32 *PrintLength, 
	OUT BOOLEAN *WidthExceeded)
{
	union
	{
		UPTR Pointer;
		FON_GLYPHENTRY *Entry;
		FON_GLYPHENTRY_V3 *EntryV3;
	} u;
	U8 MajorVersion = (U8)(FontFile->Version >> 0x08);
	U32 i;

	if (MajorVersion <= 2)
	{
		// Version 2.x
		u.Pointer = (UPTR)&FontFile->Extend;
	}
	else
	{
		// Version 3.x
		if (!(FontFile->Extend.GlyphFlags & FON_DFF_FIXED))
			return FALSE;

		u.Pointer = (UPTR)(FontFile + 1);
	}

	//
	// Glyph Entry Table:
	// [Entry for FirstChar] [Entry for FirstChar+1] ... [Entry for LastChar] [Entry for BLANK]
	//

	*WidthExceeded = FALSE;

	for(i = 0; i < TextLength; i++)
	{
		U8 Char = Text[i];
		U16 Index;

		U16 Width;
		U32 Offset;
		U16 BitmapLength;
		U8 *Bits;

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
		Bits = (U8 *)FontFile + Offset;


		if(1)
		{
			U32 c;

			FONTRACE("Bitmap: Length %d\n", BitmapLength);
			for (c = 0; c < BitmapLength; c++)
				FONTRACE("%02hhx ", Bits[c]);
			FONTRACE("\n");
		}

		if ((*X) + Width > BufferWidth)
		{
			*WidthExceeded = TRUE;
			break;
		}

		BootFonCharBlt(
			Bits, Width, FontFile->PixHeight, *X, Y, BltBuffer,
			BufferWidth, BufferHeight, BltBytesPerScanLine,
			PixelText, PixelBackground, TransparentBackground);

		*X += Width;
	}

	*PrintLength = i;

	return TRUE;
}


BOOLEAN
KERNELAPI
BootFonLookupFont(
	IN PVOID Buffer,
	IN U32 BufferLength, 
	IN U32 LookupBy, 
	IN U16 Value, 
	OUT FON_HEADER **FontFile)
{
	FON_DOS_HEADER *DosHeader;
	FON_NE_HEADER *NeHeader;
	FON_HEADER *FonHeader;
	FON_RESOURCE_TABLE *ResourceTable;
	FON_TYPEINFO_BLOCK *TypeInfo;
	U32 i;

	if (BufferLength < sizeof(*DosHeader))
		return FALSE;

	DosHeader = (FON_DOS_HEADER *)Buffer;
	if (DosHeader->Magic != 'ZM') // IMAGE_DOS_SIGNATURE
		return FALSE;

	if (BufferLength < DosHeader->Offset || 
		BufferLength < DosHeader->Offset + sizeof(*NeHeader))
		return FALSE;

	NeHeader = (FON_NE_HEADER *)((UPTR)Buffer + DosHeader->Offset);
	if (NeHeader->Magic != 'EN') // IMAGE_OS2_SIGNATURE
		return FALSE;

	// http://bytepointer.com/resources/win16_ne_exe_format_win3.0.htm
	// [DosHeader]
	// [NeHeader]
	//  + [SegTable] [ResourceTable] [ResNameTable] [ModRefTable] [ImportedNamesTable] [EntryTable] [NonResNameTable]
	//  + [Seg1Data + Seg1Info] [Seg2Data + Seg2Info] ... [SegNData + SegNInfo]

	ResourceTable = (FON_RESOURCE_TABLE *)((UPTR)NeHeader + NeHeader->OffsetToResourceTable);

	for (TypeInfo = (FON_TYPEINFO_BLOCK *)(ResourceTable + 1); ; )
	{
		FON_TYPE_ENTRY *Type = (FON_TYPE_ENTRY *)(TypeInfo + 1);
		U8 TypeOrNameLength = 0;
		CHAR8 *String1 = NULL;
		CHAR8 *TypeOrNameString = NULL;

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
		case 0x8000 | 0x08: //(U16)RT_FONT:
			FONTRACE("Type is RT_FONT\n");
			break;
		default:
			FONTRACE("Type is 0x%04x\n", TypeInfo->TypeId);
		}

		if (!(TypeInfo->TypeId & 0x8000))
			String1 = (CHAR8 *)ResourceTable + TypeInfo->TypeId;

		for (i = 0; i < TypeInfo->NumberOfResources; i++)
		{
			FONTRACE("Type[%d].OffsetToResourceData 0x%04x\n", i, Type[i].OffsetToResourceData);
			FONTRACE("Type[%d].SizeOfResource 0x%04x\n", i, Type[i].SizeOfResource);
			FONTRACE("Type[%d].Flags 0x%04x\n", i, Type[i].Flags);
			FONTRACE("Type[%d].ResourceId 0x%04x\n", i, Type[i].ResourceId);

			if (TypeInfo->TypeId == (0x8000 | 0x08)) // RT_FONT
			{
				// https://www.chiark.greenend.org.uk/~sgtatham/fonts/mkwinfont
				U32 Offset = Type[i].OffsetToResourceData << ResourceTable->AlignmentShift;
				U32 SizeOfResource = Type[i].SizeOfResource << ResourceTable->AlignmentShift;
				BOOLEAN Matched = FALSE;

				FonHeader = (FON_HEADER *)((UPTR)Buffer + Offset);

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

				// Must contain number and alphabet characters [0-9A-Za-z]
				if ('0' < FonHeader->FirstChar || 'z' > FonHeader->LastChar)
					continue;

				// Make sure that FirstChar < LastChar.
				if (FonHeader->FirstChar > FonHeader->LastChar)
					continue;

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

		TypeInfo = (FON_TYPEINFO_BLOCK *)((UPTR)TypeInfo
			+ sizeof(*TypeInfo) + sizeof(*Type) * TypeInfo->NumberOfResources);

		FONTRACE("\n");
	}

	FONTRACE("\n");

	return FALSE;
}

BOOLEAN
KERNELAPI
BootGfxScrollBuffer(
	IN U32 X,
	IN U32 Y,
	IN U32 Width,
	IN U32 Height,
	IN U32 ScrollHeight)
{
	U32 X1 = X, Y1 = Y;
	U32 SX = Width, SY = Height;

	U32 ResolutionX = PiBootGfx.ModeInfo->HorizontalResolution;
	U32 ResolutionY = PiBootGfx.ModeInfo->VerticalResolution;

	volatile U32 *FrameBuffer = (U32 *)PiBootGfx.FrameBuffer;
	U32 ScanLineWidth = PiBootGfx.ModeInfo->PixelsPerScanLine;
	U32 BackgroundColor = PiBootGfx.TextBackgroundColor;

	U32 u, v;

	DbgTraceF(TraceLevelDebug, "ScrollBuffer start (%d, %d), size (%d, %d), scroll %d\n",
		X, Y, Width, Height, ScrollHeight);

	if (X1 > ResolutionX || X1 + SX > ResolutionX ||
		Y1 > ResolutionY || Y1 + SY > ResolutionY)
		return FALSE;

	if (X1 + SX > ResolutionX)
		SX = ResolutionX - X1;
	if (Y1 + SY > ResolutionY)
		SY = ResolutionY - Y1;

	if (ScrollHeight > SY)
		ScrollHeight = SY;

	//
	// Copy Src [X1, Y1 + ScrollHeight] - [X1 + SX - 1, Y1 + SY - 1]
	//  -> Dest [X1, Y1] - [X1 + SX - 1, Y1 + SY - ScrollHeight - 1]
	//

	for (v = Y1; v + ScrollHeight < Y1 + SY; v++)
	{
		for (u = X1; u < X1 + SX; u++)
		{
			FrameBuffer[u + v * ScanLineWidth] =
				FrameBuffer[u + (v + ScrollHeight) * ScanLineWidth];
		}
	}

	for (; v < Y1 + SY; v++)
	{
		for (u = X1; u < X1 + SX; u++)
		{
			FrameBuffer[u + v * ScanLineWidth] = BackgroundColor;
		}
	}

	return TRUE;
}

BOOLEAN
KERNELAPI
BootGfxFillBuffer(
	IN U32 X,
	IN U32 Y,
	IN U32 Width,
	IN U32 Height,
	IN U32 Color)
{
	U32 X1 = X, Y1 = Y;
	U32 SX = Width, SY = Height;

	U32 ResolutionX = PiBootGfx.ModeInfo->HorizontalResolution;
	U32 ResolutionY = PiBootGfx.ModeInfo->VerticalResolution;

	volatile U32 *FrameBuffer = (U32 *)PiBootGfx.FrameBuffer;
	U32 ScanLineWidth = PiBootGfx.ModeInfo->PixelsPerScanLine;

	U32 u, v;

	if (X1 > ResolutionX || X1 + SX > ResolutionX ||
		Y1 > ResolutionY || Y1 + SY > ResolutionY)
		return FALSE;

	if (X1 + SX > ResolutionX)
		SX = ResolutionX - X1;
	if (Y1 + SY > ResolutionY)
		SY = ResolutionY - Y1;

	for (v = Y1; v < Y1 + SY; v++)
	{
		for (u = X1; u < X1 + SX; u++)
		{
			FrameBuffer[u + v * ScanLineWidth] = (Color & 0xffffff);
		}
	}

	return TRUE;
}

BOOLEAN
KERNELAPI
BootGfxPrintText(
	IN CHAR8 *Text)
{
	U32 CharWidth = PiBootGfx.BootFont->PixWidth;
	U32 CharHeight = PiBootGfx.BootFont->PixHeight;

	U32 RealCursorX = PiBootGfx.TextCursorX * CharWidth;
	U32 RealCursorY = PiBootGfx.TextCursorY * CharHeight;
	U32 ResolutionX = PiBootGfx.TextResolutionX * CharWidth;
	U32 ResolutionY = PiBootGfx.TextResolutionY * CharHeight;

	while (TRUE)
	{
		CHAR8 c = *Text;
		U32 PrintLength = 0;
		BOOLEAN WidthExceeded = FALSE;

		DbgTraceF(TraceLevelAll, "RealCursor (%d, %d)\n", RealCursorX, RealCursorY);

		if (c == '\r')
		{
			RealCursorX = 0;
			Text++;
		}
		else if (c == '\n')
		{
			RealCursorX = 0;
			if (RealCursorY + CharHeight >= ResolutionY)
			{
				BootGfxScrollBuffer(0, 0, ResolutionX, RealCursorY, CharHeight);
				RealCursorY = (PiBootGfx.TextResolutionY - 1) * CharHeight;
			}
			else
			{
				RealCursorY += CharHeight;
			}

			Text++;
		}
		else if (c == '\0')
		{
			break;
		}
		else
		{
			BootFonPrintTextN(
				PiBootGfx.BootFont,
				Text,
				1,
				&RealCursorX,
				RealCursorY,
				(U32 *)PiBootGfx.FrameBuffer,
				PiBootGfx.ModeInfo->HorizontalResolution,
				PiBootGfx.ModeInfo->VerticalResolution,
				PiBootGfx.ModeInfo->PixelsPerScanLine * sizeof(U32),
				PiBootGfx.TextColor,
				PiBootGfx.TextBackgroundColor,
				FALSE,
				&PrintLength,
				&WidthExceeded);

			Text += PrintLength;

			if (WidthExceeded)
			{
				RealCursorX = 0;
				if (RealCursorY + CharHeight >= ResolutionY)
				{
					BootGfxScrollBuffer(0, 0, ResolutionX, RealCursorY, CharHeight);
					RealCursorY = (PiBootGfx.TextResolutionY - 1) * CharHeight;
				}
				else
				{
					RealCursorY += CharHeight;
				}
			}
		}
	}

	PiBootGfx.TextCursorX = RealCursorX / CharWidth;
	PiBootGfx.TextCursorY = RealCursorY / CharHeight;

	return TRUE;
}

BOOLEAN
KERNELAPI
BootGfxInitialize(
	IN PVOID VideoModesBuffer, // Mode list buffer
	IN U32 VideoModesBufferSize, // Mode list buffer size
	IN U32 ModeNumberCurrent, // Current mode number
	IN PVOID FrameBuffer, // Linear framebuffer address
	IN SIZE_T FrameBufferSize, // Framebuffer size
	IN ZIP_CONTEXT *BootImageContext
	)
{
	U32 Offset;
	VOID *FontFile;
	U32 FontFileSize;
	FON_HEADER *BootFont;

	UPTR VideoModeRecord;
	UPTR VideoModeEnd;

	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *ModeInfo = NULL;
	U16 FontPixelWidth;

	CHAR8 *FilePath = u8"init/bootfont.fon";

	DbgTraceF(TraceLevelDebug, "%s (%p, %X, %X, %p, %X, %p)\n", 
		__FUNCTION__, VideoModesBuffer, VideoModesBufferSize, 
		ModeNumberCurrent, FrameBuffer, FrameBufferSize, BootImageContext);

	if (!ZipLookupFile_U8(BootImageContext, FilePath, &Offset) ||
		!ZipGetFileAddress(BootImageContext, Offset, &FontFile, &FontFileSize))
	{
		DbgTraceF(TraceLevelWarning, "Cannot find `%s' in boot image\n", FilePath);
		return FALSE;
	}

	DbgTraceF(TraceLevelDebug, "Framebuffer address 0x%p (%llX size)\n", 
		FrameBuffer, FrameBufferSize);

	DbgTraceF(TraceLevelDebug, "Lookup current video mode...\n");

	VideoModeRecord = (UPTR)VideoModesBuffer;
	VideoModeEnd = (UPTR)VideoModesBuffer + VideoModesBufferSize;

	while (VideoModeRecord < VideoModeEnd)
	{
		OS_VIDEO_MODE_RECORD *Record = (OS_VIDEO_MODE_RECORD *)VideoModeRecord;
		EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Temp;

		VideoModeRecord += sizeof(*Record);
		if (VideoModeRecord >= VideoModeEnd)
			break;

		Temp = (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *)VideoModeRecord;

		VideoModeRecord += Record->SizeOfInfo;
		if (VideoModeRecord > VideoModeEnd) // NOT '>='
			break;

		DbgTraceF(TraceLevelDebug, "Mode 0x%02x: Resolution %dx%d, ScanLineWidth %d, PixelFormat %d, PixelInfo (R:%d G:%d B:%d x:%d)\n",
			Record->Mode,
			Temp->HorizontalResolution, Temp->VerticalResolution, Temp->PixelsPerScanLine,
			Temp->PixelFormat,
			Temp->PixelInformation.RedMask, Temp->PixelInformation.GreenMask,
			Temp->PixelInformation.BlueMask, Temp->PixelInformation.ReservedMask);

		if (Record->Mode == ModeNumberCurrent)
		{
			DbgTraceF(TraceLevelDebug, "Mode found!\n");
			ModeInfo = Temp;
			break;
		}
	}

	if (!ModeInfo)
	{
		DbgTraceF(TraceLevelWarning, "Cannot find video mode number 0x%x\n", ModeNumberCurrent);
		return FALSE;
	}

	DbgTraceF(TraceLevelDebug, "Looking up boot font size\n");

	for (BootFont = NULL, FontPixelWidth = 24; FontPixelWidth >= 6; FontPixelWidth--)
	{
		if (BootFonLookupFont(FontFile, FontFileSize, FON_LOOKUP_BY_PIXEL_X, FontPixelWidth, &BootFont))
		{
			DbgTraceF(TraceLevelDebug, "Font size by pixel: %dx%d\n", 
				BootFont->PixWidth, BootFont->PixHeight);
			break;
		}
	}

	if (!BootFont)
	{
		DbgTraceF(TraceLevelWarning, "Failed to lookup suitable font\n");
		return FALSE;
	}


	//
	// Initialize the boot graphics structure.
	//

	PiBootGfx.FontFile = (UPTR)FontFile;
	PiBootGfx.FontFileSize = FontFileSize;
	PiBootGfx.ModesBuffer = (UPTR)VideoModesBuffer;
	PiBootGfx.ModesBufferSize = VideoModesBufferSize;
	PiBootGfx.ModeNumberCurrent = ModeNumberCurrent;
	PiBootGfx.ModeInfo = ModeInfo;
	PiBootGfx.BootFont = BootFont;
	PiBootGfx.FrameBuffer = (UPTR)FrameBuffer;
	PiBootGfx.FrameBufferSize = FrameBufferSize;

	PiBootGfx.TextCursorX = 0;
	PiBootGfx.TextCursorY = 0;
	PiBootGfx.TextResolutionX = ModeInfo->HorizontalResolution / BootFont->PixWidth;
	PiBootGfx.TextResolutionY = ModeInfo->VerticalResolution / BootFont->PixHeight;
	PiBootGfx.TextColor = 0xcccccc;
	PiBootGfx.TextBackgroundColor = 0x000000;

	DbgTraceF(TraceLevelEvent, "Boot graphics initialized\n");

	return TRUE;
}

#if 0
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
#endif
