/** @file
  Header for Base Definitions.
**/

//
// Base Headers.
//

#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>

#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Guid/FileInfo.h>


#if 0
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
  { 0x0964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION 0x00010000'
#endif


//
// Wide-Char Version of Predefined Macro.
//

#if !defined __FUNCTIONW__

#define __WIDE(_x)            L ## _x
#define _WIDE(_x)             __WIDE(_x)
#define __FUNCTIONW__         _WIDE(__FUNCTION__)
#define __FILEW__             _WIDE(__FILE__)

#endif

//
// Debug Trace and Assertions. 
//

#define TRACEF(_str, ...)     Print(L"%s: " _WIDE(_str), __FUNCTIONW__, __VA_ARGS__)
#define TRACE(_str, ...)      Print(_WIDE(_str), __VA_ARGS__)

#define ASSERT(_expr) \
{ \
  if(!(_expr)) \
  { \
    TRACE(" ------------ Assertion Failed ------------ "); \
    TRACE("   Expression : %s\n", #_expr); \
    TRACE("   Function   : %d\n", __FUNCTIONW__); \
    TRACE("   Source     : %s\n", __FILEW__); \
    TRACE("   Line       : %d\n", __LINE__); \
    TRACE(" ------------------------------------------ "); \
  } \
}


//
// Memory Type.
//

typedef enum _OS_MEMORY_TYPE {
  OsMemTypeLoaderData
} OS_MEMORY_TYPE;


//
// OS Loader Block.
//

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
    void *KernelPhysicalBase;
    UINTN SizeOfKernelImage;
  } LoaderData;
} OS_LOADER_BLOCK, *POS_LOADER_BLOCK;

extern OS_LOADER_BLOCK *OslLoaderBlock;


EFI_STATUS
EFIAPI
OslMmQueryMemoryMap (
  IN OS_LOADER_BLOCK *LoaderBlock
  );

//
// PE Loader Routines.
//

VOID *
EFIAPI
OslPeLoadImage64 (
  IN OS_LOADER_BLOCK  *LoaderBlock, 
  IN void             *FileBuffer, 
  IN UINT32            BufferSize
  );

