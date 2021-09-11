
/**
 * @file main.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Kernel main entry point.
 * @version 0.1
 * @date 2021-09-12
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <base/base.h>
#include <init/preinit.h>
#include <init/bootgfx.h>

#pragma comment(linker, "/Entry:KiKernelStart")


U64
KERNELAPI
KiKernelStart(
	IN PTR LoadedBase, 
	IN OS_LOADER_BLOCK *LoaderBlock, 
	IN U32 SizeOfLoaderBlock, 
	IN PTR Reserved)
{
	DbgInitialize(TraceLevelDebug);

	DbgNullBreak();

	DbgTraceF(TraceLevelDebug, "%s (%p, %p, %X, %p)\n",
		__FUNCTION__, LoadedBase, LoaderBlock, SizeOfLoaderBlock, Reserved);

	//UPTR SafeStackTop = LoaderBlock->LoaderData.KernelStackBase + LoaderBlock->LoaderData.StackSize - 0x10;
	//PiPreStackSwitch(LoaderBlock, SizeOfLoaderBlock, (PVOID)SafeStackTop);
    PiPreInitialize(LoaderBlock, SizeOfLoaderBlock);

	for (;;)
	{
		__PseudoIntrin_DisableInterrupt();
		__PseudoIntrin_Halt();
	}

	return 0;
}

