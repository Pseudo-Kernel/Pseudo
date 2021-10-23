#pragma once

#pragma pack(push, 1)

#define	FON_DOS_SIGNATURE				0x5a4d
#define	FON_NE_SIGNATURE				0x454e

typedef struct _FON_DOS_HEADER {
	U16 Magic;	// 'MZ'
	U8 Reserved[0x3a];
	U32 Offset;
} FON_DOS_HEADER, *PFON_DOS_HEADER;

typedef struct _FON_NE_HEADER {
	U16 Magic;	// 'NE'
	U8 MajorLinkerVersion;
	U8 MinorLinkerVersion;
	U16 OffsetToEntryTable;
	U16 SizeOfEntryTable;
	U32 Crc;
	U16 Flags;
	U16 AutomaticDataSegIndex;
	U16 InitHeapSize;
	U16 InitStackSize;
	U32 FarEntryPoint;
	U32 FarStackBase;
	U16 SegmentCount;
	U16 ModuleReferences;
	U16 SizeOfNonResidentNameTable;
	U16 OffsetToSegmentTable;
	U16 OffsetToResourceTable;
	U16 OffsetToResidentNamesTable;
	U16 OffsetToModuleReferenceTable;
	U16 OffsetToImportedNamesTable;
	U32 OffsetToNonResidentNamesTable;

	U16 Reserved1;
	U16 FileAlignmentShift;
	U16 NumberOfResourceTables;
	U8 Reserved2[1 + 1 + 2 + 2 + 2 + 2];
} FON_NE_HEADER, *PFON_NE_HEADER;

typedef struct _FON_TYPE_ENTRY {
	U16 OffsetToResourceData; // relative to BOF.
	U16 SizeOfResource;
	U16 Flags;
	U16 ResourceId; // integer type if msb is set, otherwise: offset to the type string (relative to offset table)
	U32 Reserved;
} FON_TYPE_ENTRY, *PFON_TYPE_ENTRY;

typedef struct _FON_TYPEINFO_BLOCK {
	U16 TypeId; // integer type if msb is set, otherwise: offset to the type string (relative to offset table)
	U16 NumberOfResources;
	U32 Reserved;
	// FON_TYPE_ENTRY Type[];
} FON_TYPEINFO_BLOCK, *PFON_TYPEINFO_BLOCK;

typedef struct _FON_RESOURCE_TABLE {
	U16 AlignmentShift;

	//	FON_TYPEINFO_BLOCK TypeInfoBlock[];
	//	U8 TypeOrNameLength;
	//	CHAR8 TypeName[TypeOrNameLength];
} FON_RESOURCE_TABLE, *PFON_RESOURCE_TABLE;

typedef struct _FON_HEADER {
	// http://archive.is/alHwy

	U16 Version;
	U32 FileSize;
	U8 Copyright[60];
	U16 Type; // raster font if Type[0] is cleared
	U16 NominalPointSize;
	U16 NominalVertDpi;
	U16 NominalHoriDpi;
	U16 Ascent; // distance to baseline from top

	U16 InternalLeading;
	U16 ExternalLeading;
	U8 Italic; // italic if Italic[0] is set
	U8 Underline; // underline if Underline[0] is set
	U8 StrikeOut;
	U16 Weight; // weight on a scale of 1 to 1000
	U8 Charset; // character set

	U16 PixWidth; // 0 if varies
	U16 PixHeight;

	U8 PitchAndFamily;
	U16 AvgWidth;
	U16 MaxWidth;

	// Fields that we're interested...
	U8 FirstChar;
	U8 LastChar;
	U8 DefaultChar;
	U8 BreakChar;
	U16 WidthBytes;

	U32 Device;
	U32 Face;
	U32 BitsPointer;
	U32 BitsOffset; // absolute offset if Type[3] is set.
					// see document.
					// http://archive.is/alHwy#selection-1633.7729-1633.8604
	U8 NotUsed;

	// V 3.x
	struct
	{
		U32 GlyphFlags;
		U16 GlobalASpace;
		U16 GlobalBSpace;
		U16 GlobalCSpace;

		U32 ColorPointer; // color table
		U8 Reserved1[16];
	} Extend;
	// U32 CharTable[]; { ??? }
} FON_HEADER, *PFON_HEADER;

typedef struct _FON_GLYPHENTRY {
	U16 Width;
	U16 OffsetToBits;
} FON_GLYPHENTRY, *PFON_GLYPHENTRY;

typedef struct _FON_GLYPHENTRY_V3 {
	U16 Width;
	U32 OffsetToBits;
} FON_GLYPHENTRY_V3, *PFON_GLYPHENTRY_V3;

#pragma pack(pop)

#define	FON_LOOKUP_BY_PIXEL_X		0
#define	FON_LOOKUP_BY_PIXEL_Y		1
#define	FON_LOOKUP_BY_POINT			2

#define	FON_DFF_FIXED				0x01
#define	FON_DFF_PROPORTIONAL		0x02




typedef struct _BOOT_GFX_SCREEN				BOOT_GFX_SCREEN, *PBOOT_GFX_SCREEN;


typedef
BOOLEAN
(KERNELAPI *PBOOT_GFX_FRAMEBUFFER_SCROLL)(
	IN OUT BOOT_GFX_SCREEN *Screen, 
	IN U32 ScrollHeight);

typedef struct _BOOT_GFX_FRAMEBUFFER {
	volatile UPTR FrameBuffer; // Linear framebuffer address
	UPTR FrameBufferCopy; // Copy of framebuffer
	SIZE_T FrameBufferSize; // Framebuffer size
	U32 HorizontalResolution;
	U32 VerticalResolution;
	U32 ScanLineWidth;

	PBOOT_GFX_FRAMEBUFFER_SCROLL ScrollRoutine;
} BOOT_GFX_FRAMEBUFFER, *PBOOT_GFX_FRAMEBUFFER;

typedef struct _BOOT_GFX_SCREEN {
	BOOT_GFX_FRAMEBUFFER FrameBuffer;

	// Used in text mode emulation
	U32 TextCursorX; // 0 to (ResX - 1)
	U32 TextCursorY; // 0 to (ResY - 1)
	U32 TextResolutionX;
	U32 TextResolutionY;
	U32 TextColor;
	U32 TextBackgroundColor;
	U32 TextWidth;
	U32 TextHeight;
} BOOT_GFX_SCREEN, *PBOOT_GFX_SCREEN;


#define FATAL(...) {    \
    CHAR8 __msg[1024];   \
    SIZE_T __cnt = ClStrFormatU8(__msg, COUNTOF(__msg), __VA_ARGS__);  \
    ClStrTerminateU8(__msg, COUNTOF(__msg), __cnt);    \
    BootGfxFatalStop(                       \
        "%s: file %s, line %d\r\n%s",       \
        __FUNCTION__, __FILE__, __LINE__,   \
        __msg);                             \
}

#define BGXTRACE                    BootGfxPrintTextFormat

#define BGXTRACE_C(_color, ...) {   \
    U32 __prev_color = BootGfxSetTextColor(_color); \
    BootGfxPrintTextFormat(__VA_ARGS__);    \
    BootGfxSetTextColor(__prev_color);      \
}

//
// BGRx8888 format color codes.
//

#define BGX_RGB(_r, _g, _b)    \
    ( (((_r) & 0xff) << 16) | (((_g) & 0xff) << 8) | (((_b) & 0xff) << 0) )

#define BGX_COLOR_BLACK             BGX_RGB(0x0c, 0x0c, 0x0c)
#define BGX_COLOR_BLUE              BGX_RGB(0x00, 0x37, 0xda)
#define BGX_COLOR_CYAN              BGX_RGB(0x32, 0x96, 0xdd)
#define BGX_COLOR_GREEN             BGX_RGB(0x13, 0xa1, 0x0e)
#define BGX_COLOR_PURPLE            BGX_RGB(0x88, 0x17, 0x98)
#define BGX_COLOR_RED               BGX_RGB(0xc5, 0x0f, 0x1f)
#define BGX_COLOR_WHITE             BGX_RGB(0xcc, 0xcc, 0xcc)
#define BGX_COLOR_YELLOW            BGX_RGB(0xc1, 0x9c, 0x00)
#define BGX_COLOR_LIGHT_BLACK       BGX_RGB(0x76, 0x76, 0x76)
#define BGX_COLOR_LIGHT_BLUE        BGX_RGB(0x3b, 0x78, 0xff)
#define BGX_COLOR_LIGHT_CYAN        BGX_RGB(0x61, 0xd6, 0xd6)
#define BGX_COLOR_LIGHT_GREEN       BGX_RGB(0x16, 0xc6, 0x0c)
#define BGX_COLOR_LIGHT_PURPLE      BGX_RGB(0xb4, 0x00, 0x9e)
#define BGX_COLOR_LIGHT_RED         BGX_RGB(0xe7, 0x48, 0x56)
#define BGX_COLOR_LIGHT_WHITE       BGX_RGB(0xf2, 0xf2, 0xf2)
#define BGX_COLOR_LIGHT_YELLOW      BGX_RGB(0xf9, 0xf1, 0xa5)



typedef struct _ZIP_CONTEXT			*PZIP_CONTEXT;

BOOLEAN
KERNELAPI
BootGfxInitialize(
	IN PVOID VideoModesBuffer, // Mode list buffer
	IN U32 VideoModesBufferSize, // Mode list buffer size
	IN U32 ModeNumberCurrent, // Current mode number
	IN PVOID FrameBuffer, // Linear framebuffer address
	IN PVOID FrameBufferCopy, // Copy framebuffer address
	IN SIZE_T FrameBufferSize, // Framebuffer size
	IN PZIP_CONTEXT BootImageContext
	);

BOOLEAN
KERNELAPI
BootFonLookupFont(
	IN PVOID Buffer,
	IN U32 BufferLength,
	IN U32 LookupBy,
	IN U16 Value,
	OUT FON_HEADER **FontFile);

VOID
KERNELAPI
BootGfxUpdateScreen(
    IN BOOT_GFX_SCREEN *Screen);

BOOLEAN
KERNELAPI
BootGfxPrintText(
	IN CHAR8 *Text);

BOOLEAN
VARCALL
BootGfxPrintTextFormat(
	IN CHAR8 *Format,
	...);

BOOLEAN
KERNELAPI
BootGfxFillBuffer(
	IN U32 X,
	IN U32 Y,
	IN U32 Width,
	IN U32 Height,
	IN U32 Color);

U32
KERNELAPI
BootGfxSetTextColor(
	IN U32 TextColor);

U32
KERNELAPI
BootGfxSetBkColor(
	IN U32 BackgroundColor);

VOID
VARCALL
BootGfxFatalStop(
	IN CHAR8 *Format,
	...);


