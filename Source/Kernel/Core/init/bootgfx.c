#include <base/base.h>
#include <init/bootgfx.h>
#include <init/zip.h>

// http://bytepointer.com/resources/win16_ne_exe_format_win3.0.htm

#define	FONTRACE								//printf

typedef struct _BOOT_GFX {
	BOOT_GFX_SCREEN Screen;

	UPTR ModesBuffer; // Mode list buffer
	U32 ModesBufferSize; // Mode list buffer size
	U32 ModeNumberCurrent; // Current mode number
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *ModeInfo; // Current mode info

	UPTR FontFile; // Boot font address (.fon format)
	SIZE_T FontFileSize; // Size of boot font
	FON_HEADER *BootFont; // .fnt address which resides in .fon file
} BOOT_GFX, *PBOOT_GFX;

BOOT_GFX PiBootGfx;


BOOLEAN
KERNELAPI
BootFonCharBlt(
	IN BOOT_GFX_SCREEN *Screen,
	IN U8 *Bits, // size: ((CharWidth + 7) >> 3) * CharHeight
	IN U32 CharWidth,
	IN U32 CharHeight,
	IN BOOLEAN TransparentBackground)
{
	U32 i, j, k;
	U32 u, v;
	U32 X, Y;
	U32 *BltBuffer;

	if (Screen->TextCursorX >= Screen->TextResolutionX ||
		Screen->TextCursorY >= Screen->TextResolutionY)
	{
		return FALSE;
	}

	if (!Bits)
	{
		// Returns whether the glyph can be blitted
		return TRUE;
	}

	u = (CharWidth + 0x07) >> 3;
	v = CharHeight;

	X = Screen->TextCursorX * CharWidth;
	Y = Screen->TextCursorY * CharHeight;

	BltBuffer = (U32 *)Screen->FrameBuffer.FrameBuffer;

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
				{
					break;
				}

				// CharBit[7 - k, j] -> Buf[X + k + (i << 3), Y + j]
				if (DestX >= Screen->FrameBuffer.HorizontalResolution ||
					DestY >= Screen->FrameBuffer.VerticalResolution)
				{
					break; // out of range
				}

				if (b & (1 << (7 - k)))
					BltBuffer[DestX + DestY * Screen->FrameBuffer.ScanLineWidth] = (Screen->TextColor & 0xffffff);
				else if (!TransparentBackground)
					BltBuffer[DestX + DestY * Screen->FrameBuffer.ScanLineWidth] = (Screen->TextBackgroundColor & 0xffffff);
			}
		}
	}

	Screen->TextCursorX++;

	if (Screen->TextCursorX >= Screen->TextResolutionX)
	{
		U32 TextCursorX = Screen->TextCursorX;
		Screen->TextCursorX %= Screen->TextResolutionX;
		Screen->TextCursorY += (TextCursorX / Screen->TextResolutionX);
	}

	return TRUE;
}

BOOLEAN
KERNELAPI
BootFonPrintTextN(
	IN BOOT_GFX_SCREEN *Screen,
	IN PFON_HEADER FontFile,
	IN CHAR8 *Text, 
	IN SIZE_T TextLength, 
	IN BOOLEAN TransparentBackground, 
	OUT SIZE_T *PrintLength)
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

	for(i = 0; i < TextLength; )
	{
		U8 Char = Text[i];
		U16 Index;

		U16 Width;
		U32 Offset;
		U16 BitmapLength;
		U8 *Bits;

		BOOLEAN Scroll = FALSE;
		U32 ScrollCount = 0;

		switch (Char)
		{
		case '\r':
			Screen->TextCursorX = 0;
			i++;
			break;

		case '\n':
			DbgTraceF(TraceLevelAll, "Cursor (%d, %d), TextRes (%d, %d)\n", 
				Screen->TextCursorX, Screen->TextCursorY, 
				Screen->TextResolutionX, Screen->TextResolutionY);

			Screen->TextCursorX = 0;
			Screen->TextCursorY++;
			i++;
			break;

		default:
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

			if (1)
			{
				U32 c;

				FONTRACE("Bitmap: Length %d\n", BitmapLength);
				for (c = 0; c < BitmapLength; c++)
					FONTRACE("%02hhx ", Bits[c]);
				FONTRACE("\n");
			}

			// Scroll if fails
			if (!BootFonCharBlt(Screen, Bits, Width, FontFile->PixHeight, TransparentBackground))
				Scroll = TRUE;
			else
				i++;

			break;
		}

		if (Scroll)
		{
			if (Screen->TextCursorX >= Screen->TextResolutionY)
			{
				Screen->TextCursorY++;
			}

			if (Screen->TextCursorY >= Screen->TextResolutionY)
			{
				Screen->TextCursorX = 0;
				ScrollCount = Screen->TextCursorY - Screen->TextResolutionY + 1;
				Screen->TextCursorY = Screen->TextResolutionY - 1;
			}
			else
			{
				DbgTraceF(TraceLevelDebug, "Unexpected buffer scroll!\n");
				for (;;);
			}

			DbgTraceF(TraceLevelAll, "ScrollCount = %d\n", ScrollCount);

			Screen->FrameBuffer.ScrollRoutine(Screen, FontFile->PixHeight * ScrollCount);
		}
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
	if (DosHeader->Magic != FON_DOS_SIGNATURE) // IMAGE_DOS_SIGNATURE
		return FALSE;

	if (BufferLength < DosHeader->Offset || 
		BufferLength < DosHeader->Offset + sizeof(*NeHeader))
		return FALSE;

	NeHeader = (FON_NE_HEADER *)((UPTR)Buffer + DosHeader->Offset);
	if (NeHeader->Magic != FON_NE_SIGNATURE) // IMAGE_OS2_SIGNATURE
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
BootGfxScrollBufferInternal(
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

	volatile U32 *FrameBuffer = (U32 *)PiBootGfx.Screen.FrameBuffer.FrameBuffer;
	U32 ScanLineWidth = PiBootGfx.ModeInfo->PixelsPerScanLine;
	U32 BackgroundColor = PiBootGfx.Screen.TextBackgroundColor;

	U32 u, v;

	DbgTraceF(TraceLevelAll, "ScrollBuffer start (%d, %d), size (%d, %d), scroll %d\n",
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
BootGfxScrollBuffer(
	IN OUT BOOT_GFX_SCREEN *Screen,
	IN U32 ScrollHeight)
{
	U32 Width = Screen->TextResolutionX * Screen->TextWidth;
	U32 Height = Screen->TextResolutionY * Screen->TextHeight;

	return BootGfxScrollBufferInternal(0, 0, Width, Height, ScrollHeight);
}

U32
KERNELAPI
BootGfxSetTextColor(
	IN U32 TextColor)
{
	U32 TextColorPrev = PiBootGfx.Screen.TextColor;
	PiBootGfx.Screen.TextColor = TextColor;
	return TextColorPrev;
}

U32
KERNELAPI
BootGfxSetBkColor(
	IN U32 BackgroundColor)
{
	U32 BkColorPrev = PiBootGfx.Screen.TextBackgroundColor;
	PiBootGfx.Screen.TextBackgroundColor = BackgroundColor;
	return BkColorPrev;
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

	volatile U32 *FrameBuffer = (U32 *)PiBootGfx.Screen.FrameBuffer.FrameBuffer;
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
	SIZE_T PrintLength;
	return BootFonPrintTextN(&PiBootGfx.Screen, PiBootGfx.BootFont, Text, strlen(Text), FALSE, &PrintLength);
}

BOOLEAN
KERNELAPI
BootGfxPrintTextFormat(
	IN CHAR8 *Format, 
	...)
{
	CHAR8 Buffer[512];
	SIZE_T BufferLength;
	SIZE_T PrintLength;
	va_list args;

	va_start(args, Format);
	BufferLength = ClStrFormatU8V(Buffer, ARRAY_SIZE(Buffer), Format, args);
	va_end(args);

	return BootFonPrintTextN(&PiBootGfx.Screen, PiBootGfx.BootFont, Buffer, BufferLength, FALSE, &PrintLength);
}

VOID
KERNELAPI
BootGfxFatalStop(
	IN CHAR8 *Format,
	...)
{
	CHAR8 Buffer[512];
	SIZE_T BufferLength;
	SIZE_T PrintLength;
	va_list args;

	va_start(args, Format);
	BufferLength = ClStrFormatU8V(Buffer, ARRAY_SIZE(Buffer), Format, args);
	va_end(args);

	DbgTraceN(TraceLevelError, "\n", 1);
	DbgTraceN(TraceLevelError, Buffer, BufferLength);
	DbgTraceN(TraceLevelError, "\n", 1);

	BootGfxSetBkColor(0);
	BootGfxSetTextColor(0xff0000);
	BootFonPrintTextN(&PiBootGfx.Screen, PiBootGfx.BootFont, Buffer, BufferLength, FALSE, &PrintLength);

	// System stop.
	__PseudoIntrin_DisableInterrupt();

	for (;;)
		__PseudoIntrin_Halt();
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

		DbgTraceF(TraceLevelDebug, "Mode 0x%02x: Resolution %dx%d, ScanLineWidth %d, PixelFormat %d\n",
			Record->Mode,
			Temp->HorizontalResolution, Temp->VerticalResolution, Temp->PixelsPerScanLine,
			Temp->PixelFormat);

		if(Temp->PixelFormat == PixelBitMask)
		{
			DbgTraceF(TraceLevelDebug, "+ PixelInfo (R:%d G:%d B:%d x:%d)\n",
				Temp->PixelInformation.RedMask, Temp->PixelInformation.GreenMask,
				Temp->PixelInformation.BlueMask, Temp->PixelInformation.ReservedMask);
		}

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

	PiBootGfx.Screen.FrameBuffer.FrameBuffer = (UPTR)FrameBuffer;
	PiBootGfx.Screen.FrameBuffer.FrameBufferSize = FrameBufferSize;
	PiBootGfx.Screen.FrameBuffer.HorizontalResolution = ModeInfo->HorizontalResolution;
	PiBootGfx.Screen.FrameBuffer.VerticalResolution = ModeInfo->VerticalResolution;
	PiBootGfx.Screen.FrameBuffer.ScanLineWidth = ModeInfo->PixelsPerScanLine;
	PiBootGfx.Screen.FrameBuffer.ScrollRoutine = BootGfxScrollBuffer;

	PiBootGfx.Screen.TextCursorX = 0;
	PiBootGfx.Screen.TextCursorY = 0;
	PiBootGfx.Screen.TextResolutionX = ModeInfo->HorizontalResolution / BootFont->PixWidth;
	PiBootGfx.Screen.TextResolutionY = ModeInfo->VerticalResolution / BootFont->PixHeight;
	PiBootGfx.Screen.TextWidth = BootFont->PixWidth;
	PiBootGfx.Screen.TextHeight = BootFont->PixHeight;
	PiBootGfx.Screen.TextColor = 0xcccccc;
	PiBootGfx.Screen.TextBackgroundColor = 0x000000;

	DbgTraceF(TraceLevelEvent, "Boot graphics initialized\n");

	return TRUE;
}
