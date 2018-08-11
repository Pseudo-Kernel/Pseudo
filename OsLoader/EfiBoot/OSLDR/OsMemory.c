/** @file
  Pseudo OS Loader Application for UEFI Environment.
**/
#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>

#include "OSLDR.h"

/**
  Allocates and querys the memory map.

  @param[in] LoaderBlock    The loader block which contains OS loader information.
  
  @retval EFI_SUCCESS       The operation is completed successfully.
  @retval other             An error occurred during the operation.

**/
EFI_STATUS
EFIAPI
OslMmQueryMemoryMap (
  IN OS_LOADER_BLOCK *LoaderBlock
  )
{
  EFI_MEMORY_DESCRIPTOR *Map;
  UINTN MemoryMapSize;
  UINTN MapKey;
  UINTN DescriptorSize;
  UINT32 DescriptorVersion;
  EFI_STATUS Status;
  UINTN TryCount;

  MemoryMapSize = sizeof(EFI_MEMORY_DESCRIPTOR);

  for(TryCount = 0; TryCount < 0x100; TryCount++)
  {
    Status = LoaderBlock->Base.BootServices->AllocatePool(EfiLoaderData, MemoryMapSize, &Map);
    if(Status != EFI_SUCCESS)
      return Status;

    Status = LoaderBlock->Base.BootServices->GetMemoryMap(&MemoryMapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
    if(Status == EFI_SUCCESS)
      break;

    LoaderBlock->Base.BootServices->FreePool(Map);
  }

  if(Status != EFI_SUCCESS)
  {
    TRACEF("Failed to query the memory map, Status 0x%08x\n", Status);
    return Status;
  }


  LoaderBlock->Memory.Map = Map;
  LoaderBlock->Memory.MapKey = MapKey;
  LoaderBlock->Memory.MapCount = MemoryMapSize / DescriptorSize;
  LoaderBlock->Memory.DescriptorSize = DescriptorSize;
  LoaderBlock->Memory.DescriptorVersion = DescriptorVersion;

  return EFI_SUCCESS;
}
