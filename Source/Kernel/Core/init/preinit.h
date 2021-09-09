#pragma once

VOID
KERNELAPI
PiPreStackSwitch(
	IN OS_LOADER_BLOCK *LoaderBlock,
	IN U32 SizeOfLoaderBlock, 
	IN PVOID SafeStackTop);

VOID
KERNELAPI
PiPreInitialize(
	IN OS_LOADER_BLOCK *LoaderBlock,
	IN U32 LoaderBlockSize);


typedef struct _PREINIT_PAGE_RESERVE
{
	UPTR BaseAddress;
	SIZE_T Size;
} PREINIT_PAGE_RESERVE;


