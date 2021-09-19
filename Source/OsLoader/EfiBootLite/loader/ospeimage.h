
#pragma once

#include "peimage.h"


//
// PE loader routines.
//

PIMAGE_NT_HEADERS_3264
EFIAPI
OslPeImageBaseToNtHeaders(
    IN VOID *ImageBase);

EFI_PHYSICAL_ADDRESS
EFIAPI
OslPeLoadImage(
    IN OS_LOADER_BLOCK *LoaderBlock,
    IN VOID *FileBuffer,
    IN UINTN FileBufferLength,
    OUT UINTN *MappedSize,
    IN UINT64 FixupBase);

