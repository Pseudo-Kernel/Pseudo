/** @file
  Header for Base Definitions.
**/

// Define the OSLOADER_EFI_SCREEN_RESOLUTION_FIX_800_600 to fix screen resolution. 
#define	OSLOADER_EFI_SCREEN_RESOLUTION_FIX_800_600

#ifndef __FUNCTION__
#define	__FUNCTION__				__func__
#endif

//
// Base Headers.
//

#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>

#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Guid/FileInfo.h>
#include <Guid/Acpi.h>
#include <Guid/SmBios.h>

#include "SFormat.h"
#include "PeImage.h"



extern EFI_HANDLE gImageHandle;
extern EFI_SYSTEM_TABLE *gST;
extern EFI_BOOT_SERVICES *gBS;
extern EFI_RUNTIME_SERVICES *gRS;

extern EFI_SIMPLE_TEXT_INPUT_PROTOCOL *gConIn;
extern EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *gConOut;

#define	PRINT_MAX_BUFFER			500



#if 0
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
  { 0x0964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION 0x00010000
#endif



#define	__ASCII(_x)           #_x
#define	_ASCII(_x)            __ASCII(_x)
#define ASCII(_x)             _ASCII(_x)

//
// Debug Trace and Assertions. 
//

#define	IS_DEBUG_MODE(_ldrblk)	(!!((_ldrblk)->Debug.DebugPrint))

#define TRACEF(_str, ...)			Trace(L"%s: " _str, __FUNCTION__, ## __VA_ARGS__)
#define TRACE(_str, ...)			Trace(_str, ## __VA_ARGS__)

#define	DTRACEF(_ldrblk, _str, ...)			\
	do {									\
		if(IS_DEBUG_MODE(_ldrblk)) {		\
			TRACEF(_str, ## __VA_ARGS__);	\
		}									\
	} while(FALSE);

#define	DTRACE(_ldrblk, _str, ...)			\
	do {									\
		if(IS_DEBUG_MODE(_ldrblk)) {		\
			TRACE(_str, ## __VA_ARGS__);	\
		}									\
	} while(FALSE);



#define ASSERT(_expr) \
{ \
	if(!(_expr)) \
	{ \
		TRACE(" ------------ Assertion Failed ------------ "); \
		TRACE("   Expression : %s\n", #_expr); \
		TRACE("   Function   : %s\n", __FUNCTION__); \
		TRACE("   Source     : %s\n", __FILE__); \
		TRACE("   Line       : %d\n", __LINE__); \
		TRACE(" ------------------------------------------ "); \
	} \
}


//
// OS Loader Block.
//

#pragma pack(push, 8)

typedef struct _OS_VIDEO_MODE_RECORD {
	UINT32 Mode;
	UINT32 SizeOfInfo;

	// Copy of returned buffer from EFI_GRAPHICS_OUTPUT_PROTOCOL.QueryMode
	// UCHAR ModeInfo[SizeOfInfo];
} OS_VIDEO_MODE_RECORD, *POS_VIDEO_MODE_RECORD;

typedef struct _OS_LOADER_BLOCK {
	struct
	{
		EFI_SYSTEM_TABLE *SystemTable;
		EFI_HANDLE ImageHandle;

		EFI_BOOT_SERVICES *BootServices;
		EFI_RUNTIME_SERVICES *RuntimeServices;

		//
		// Handle of the boot volume root directory.
		//

		EFI_FILE *RootDirectory;
	} Base;

	struct
	{
		//
		// NOTE : ... OS software must use the DescriptorSize to find the start of
		//        each EFI_MEMORY_DESCRIPTOR in the MemoryMap array.
		//        [UEFI Specification, Version 2.7 Errata A]
		//

		EFI_MEMORY_DESCRIPTOR *Map;
		UINTN MapCount;
		UINTN MapKey;
		UINTN DescriptorSize;
		UINT32 DescriptorVersion;
	} Memory;

	struct
	{
		EFI_PHYSICAL_ADDRESS TempBase;
		UINTN TempSize;

		EFI_PHYSICAL_ADDRESS ShadowBase; // Low 1M Shadow
		UINTN ShadowSize;

		EFI_PHYSICAL_ADDRESS KernelPhysicalBase;
		UINTN MappedSize;

		EFI_PHYSICAL_ADDRESS KernelStackBase;
		UINTN StackSize;

		EFI_PHYSICAL_ADDRESS BootImageBase;
		UINTN BootImageSize;

		//
		// Video Modes.
		// VideoModes = [VideoModeRecord1] [VideoModeRecord2] ... [VideoModeRecordN]
		// VideoModeRecord = [ModeNumber] [SizeOfInfo] [VideoMode]
		// UINT32 ModeNumber;
		// UINT32 SizeOfInfo;
		// UCHAR ModeInfo[SizeOfMode]; // Returned from EFI_GRAPHICS_OUTPUT_PROTOCOL.QueryMode
		//

		EFI_PHYSICAL_ADDRESS VideoModeBase;
		EFI_PHYSICAL_ADDRESS VideoFramebufferBase;
		UINTN VideoFramebufferSize;
		UINTN VideoModeSize;
		UINT32 VideoMaxMode;
		UINT32 VideoModeSelected;
	} LoaderData;

	struct
	{
		EFI_PHYSICAL_ADDRESS AcpiTable;
	} Configuration;

	struct
	{
		BOOLEAN DebugPrint;
	} Debug;
} OS_LOADER_BLOCK, *POS_LOADER_BLOCK;

#pragma pack(pop)

extern OS_LOADER_BLOCK OslLoaderBlock;

#if 0

//
// X64 Paging Structures.
//

typedef union _X64_PML4E {
	struct
	{
		UINT64 Present : 1;
		UINT64 Writable : 1;
		UINT64 User : 1;
		UINT64 WriteThru : 1;
		UINT64 CacheDisable : 1;
		UINT64 Accessed : 1;
		UINT64 Ignored1 : 1;
		UINT64 ReservedZero : 1;
		UINT64 Ignored2 : 4;
		UINT64 PDPT : 40;
		UINT64 Ignored3 : 11;
		UINT64 ExecuteDisable : 1;
	} b;
	UINT64 Value;
} X64_PML4E;

typedef union _X64_PDPTE {
	struct
	{
		UINT64 Present : 1;
		UINT64 Writable : 1;
		UINT64 User : 1;
		UINT64 WriteThru : 1;
		UINT64 CacheDisable : 1;
		UINT64 Accessed : 1;
		UINT64 Ignored1 : 1;
		UINT64 ReservedZero : 1;
		UINT64 Ignored2 : 4;
		UINT64 PDPT : 40;
		UINT64 Ignored3 : 11;
		UINT64 ExecuteDisable : 1;
	} b;
	UINT64 Value;
} X64_PDPTE;

typedef union _X64_PDE {
	struct
	{
		UINT64 Present : 1;
		UINT64 Writable : 1;
		UINT64 User : 1;
		UINT64 WriteThru : 1;
		UINT64 CacheDisable : 1;
		UINT64 Accessed : 1;
		UINT64 Ignored1 : 1;
		UINT64 ReservedZero : 1;
		UINT64 Ignored2 : 4;
		UINT64 PDPT : 40;
		UINT64 Ignored3 : 11;
		UINT64 ExecuteDisable : 1;
	} b;
	UINT64 Value;
} X64_PDE;

typedef union _X64_PDE_2MB {
	struct
	{
		UINT64 Present : 1;
		UINT64 Writable : 1;
		UINT64 User : 1;
		UINT64 WriteThru : 1;
		UINT64 CacheDisable : 1;
		UINT64 Accessed : 1;
		UINT64 Dirty : 1;
		UINT64 PageSize2M : 1; // must be 1
		UINT32 Global : 1;
		UINT64 Ignored1 : 3;
		UINT64 PAT : 1;
		UINT64 Reserved : 8;
		UINT64 BaseAddress : 31;
		UINT64 Ignored3 : 7;
		UINT64 ProtectionKey : 4;
		UINT64 ExecuteDisable : 1;
	} b;
	UINT64 Value;
} X64_PDE_2MB;

typedef union _X64_PTE {
	struct
	{
		UINT64 Present : 1;
		UINT64 Writable : 1;
		UINT64 User : 1;
		UINT64 WriteThru : 1;
		UINT64 CacheDisable : 1;
		UINT64 Accessed : 1;
		UINT64 Dirty : 1;
		UINT64 PAT : 1;
		UINT64 Global : 1;
		UINT64 Ignored2 : 3;
		UINT64 BaseAddress : 40;
		UINT64 Ignored3 : 7;
		UINT64 ProtectionKey : 4;
		UINT64 ExecuteDisable : 1;
	} b;
	UINT64 Value;
} X64_PTE;

#endif


//
// Kernel Entry Point.
//

typedef
UINT64
(EFIAPI *PKERNEL_START_ENTRY)(
	IN UINT64 LoadedBase,
	IN UINT64 LoaderBlock,
	IN UINT32 SizeOfLoaderBlock,
	IN UINT64 Reserved);


//
// Efi Routines.
//

EFI_STATUS
EFIAPI
EfiAllocatePages(
	IN UINTN Size, 
	IN OUT EFI_PHYSICAL_ADDRESS *Address, 
	IN BOOLEAN AddressSpecified);

EFI_STATUS
EFIAPI
EfiFreePages(
	IN EFI_PHYSICAL_ADDRESS Address,
	IN UINTN Size);

//
// Misc Routines.
//

VOID
Trace(
	IN CHAR16 *Format,
	...);

//
// Keyboard Input Routines.
//

BOOLEAN
EFIAPI
OslWaitForKeyInput(
	IN UINT16 ScanCode,
	IN CHAR16 UnicodeCharacter, 
	IN UINT64 Timeout);

VOID
EFIAPI
OslDbgWaitEnterKey(
	IN OS_LOADER_BLOCK *LoaderBlock,
	IN CHAR16 *Message);

//
// Memory Routines.
//

EFI_STATUS
EFIAPI
OslMmQueryMemoryMap (
	IN OS_LOADER_BLOCK *LoaderBlock);

VOID
EFIAPI
OslMmFreeMemoryMap(
	IN OS_LOADER_BLOCK *LoaderBlock);

VOID
EFIAPI
OslMmDumpMemoryMap(
	IN EFI_MEMORY_DESCRIPTOR *Map,
	IN UINTN MemoryMapSize,
	IN UINTN DescriptorSize);

//
// PE Loader Routines.
//

PIMAGE_NT_HEADERS_3264
EFIAPI
OslPeImageBaseToNtHeaders(
	IN VOID *ImageBase);

EFI_PHYSICAL_ADDRESS
EFIAPI
OslPeLoadImage(
	IN VOID *FileBuffer,
	IN UINTN FileBufferLength,
	OUT UINTN *MappedSize);

//
// Transfer-to-Kernel Routines.
//

BOOLEAN
EFIAPI
OslTransferToKernel(
	IN OS_LOADER_BLOCK *LoaderBlock);

