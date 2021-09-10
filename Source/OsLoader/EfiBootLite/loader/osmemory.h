
#pragma once


//
// Memory routines.
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


