
#pragma once

// Define the OSLOADER_EFI_SCREEN_RESOLUTION_FIX_800_600 to fix screen resolution. 
//#define OSLOADER_EFI_SCREEN_RESOLUTION_FIX_800_600

#ifndef __FUNCTION__
#define __FUNCTION__                __func__
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
#include <Protocol/Rng.h>
#include <Guid/FileInfo.h>
#include <Guid/Acpi.h>
#include <Guid/SmBios.h>

#include "SFormat.h"



extern EFI_HANDLE gImageHandle;
extern EFI_SYSTEM_TABLE *gST;
extern EFI_BOOT_SERVICES *gBS;
extern EFI_RUNTIME_SERVICES *gRS;

extern EFI_SIMPLE_TEXT_INPUT_PROTOCOL *gConIn;
extern EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *gConOut;

#define PRINT_MAX_BUFFER            500



#if 0
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
  { 0x0964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION 0x00010000
#endif



#define __ASCII(_x)           #_x
#define _ASCII(_x)            __ASCII(_x)
#define ASCII(_x)             _ASCII(_x)

//
// Debug Trace and Assertions. 
//

#define IS_DEBUG_MODE(_ldrblk)  (!!((_ldrblk)->Debug.DebugPrint))

#define TRACEF(_str, ...)           Trace(L"%s: " _str, __FUNCTION__, ## __VA_ARGS__)
#define TRACE(_str, ...)            Trace(_str, ## __VA_ARGS__)

#define DTRACEF(_ldrblk, _str, ...)         \
    do {                                    \
        if(IS_DEBUG_MODE(_ldrblk)) {        \
            TRACEF(_str, ## __VA_ARGS__);   \
        }                                   \
    } while(FALSE);

#define DTRACE(_ldrblk, _str, ...)          \
    do {                                    \
        if(IS_DEBUG_MODE(_ldrblk)) {        \
            TRACE(_str, ## __VA_ARGS__);    \
        }                                   \
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

typedef struct _OS_VIDEO_MODE_RECORD
{
    UINT32 Mode;
    UINT32 SizeOfInfo;

    // Copy of returned buffer from EFI_GRAPHICS_OUTPUT_PROTOCOL.QueryMode
    // UCHAR ModeInfo[SizeOfInfo];
} OS_VIDEO_MODE_RECORD, *POS_VIDEO_MODE_RECORD;


#define OSL_LOADER_PREINIT_POOL_SIZE            0x800000 // 8M
#define OSL_LOADER_LOW_1M_SHADOW_SIZE           0x100000 // 1M
#define OSL_LOADER_KERNEL_STACK_SIZE            0x100000 // 1M

typedef struct _OS_PRESERVE_MEMORY_RANGE
{
    EFI_PHYSICAL_ADDRESS PhysicalStart;
    EFI_VIRTUAL_ADDRESS VirtualStart;
    UINT64 Size;
    UINT32 Type;
    UINT32 Reserved;
} OS_PRESERVE_MEMORY_RANGE;

#define OS_PRESERVE_RANGE_MAX_COUNT         64
#define OS_PRESERVE_RANGE_BITMAP_FULL       0xffffffffffffffffULL

typedef struct _OS_LOADER_BLOCK
{
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
        UINTN MapSize;
        UINT32 DescriptorVersion;
    } Memory;

    struct
    {
        UINT8 Random[128];
        EFI_VIRTUAL_ADDRESS OffsetToVirtualBase;

        EFI_PHYSICAL_ADDRESS PreInitPoolBase;
        UINTN PreInitPoolSize;

        EFI_PHYSICAL_ADDRESS ShadowBase; // Low 1M Shadow
        UINTN ShadowSize;

        EFI_PHYSICAL_ADDRESS KernelPhysicalBase;
        UINTN MappedSize;

        EFI_PHYSICAL_ADDRESS KernelStackBase;
        UINTN StackSize;

        EFI_PHYSICAL_ADDRESS BootImageBase;
        UINTN BootImageSize;

        EFI_PHYSICAL_ADDRESS PxeInitPoolBase;
        UINTN PxeInitPoolSize;
        UINTN PxeInitPoolSizeUsed;

        EFI_PHYSICAL_ADDRESS PML4TBase;
        EFI_PHYSICAL_ADDRESS RPML4TBase;

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
        EFI_PHYSICAL_ADDRESS VideoFramebufferCopy;
        UINTN VideoFramebufferSize;
        UINTN VideoModeSize;
        UINT32 VideoMaxMode;
        UINT32 VideoModeSelected;

        OS_PRESERVE_MEMORY_RANGE PreserveRanges[OS_PRESERVE_RANGE_MAX_COUNT];
        UINT64 PreserveRangesBitmap;
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


extern EFI_GUID gEfiRngProtocolGuid;


//
// Loader space.
//

#define KERNEL_VA_SIZE_LOADER_SPACE                     0x0000001000000000ULL // 64G
#define KERNEL_VA_SIZE_LOADER_SPACE_ASLR_GAP            0x0000008000000000ULL // 512G

#define KERNEL_VA_START_LOADER_SPACE                    0xffff8f0000000000ULL // Used in initialization
#define KERNEL_VA_END_LOADER_SPACE                      (KERNEL_VA_START_LOADER_SPACE + KERNEL_VA_SIZE_LOADER_SPACE + KERNEL_VA_SIZE_LOADER_SPACE_ASLR_GAP)


