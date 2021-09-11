
#pragma once


//
// Kernel entry point.
//

typedef
UINT64
(EFIAPI *PKERNEL_START_ENTRY)(
    IN UINT64 LoadedBase,
    IN UINT64 LoaderBlock,
    IN UINT32 SizeOfLoaderBlock,
    IN UINT64 Reserved);


//
// Transfer to kernel.
//

BOOLEAN
EFIAPI
OslTransferToKernel(
    IN OS_LOADER_BLOCK *LoaderBlock);


