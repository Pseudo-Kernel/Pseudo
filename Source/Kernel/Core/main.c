
/**
 * @file main.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Kernel main entry point.
 * @version 0.1
 * @date 2021-09-12
 * 
 * @copyright Copyright (c) 2021
 * 
 * @todo There are many works to do:\n
 *       - Pool initialization
 *       - Interrupt registration and dispatch
 *       - Processor initialization (IOAPIC, LAPIC, per-processor data and tables)
 */

#include <base/base.h>
#include <init/preinit.h>
#include <init/bootgfx.h>

/**
 * @brief Kernel main entry point.
 * 
 * @param LoadedBase            Kernel base.
 * @param LoaderBlock           Loader block provided by Osloader.
 * @param SizeOfLoaderBlock     Size of loader block.
 * @param Reserved              Reserved. Currently zero.
 * 
 * @return This function never returns.
 */
__attribute__((ms_abi))
U64
KERNELAPI
KiKernelStart(
	IN PTR LoadedBase, 
	IN OS_LOADER_BLOCK *LoaderBlock, 
	IN U32 SizeOfLoaderBlock, 
	IN PTR Reserved)
{
	DbgInitialize(TraceLevelDebug);

	DbgTraceF(TraceLevelDebug, "%s (%p, %p, %X, %p)\n",
		__FUNCTION__, LoadedBase, LoaderBlock, SizeOfLoaderBlock, Reserved);

    PiPreInitialize(LoaderBlock, SizeOfLoaderBlock);

	for (;;)
	{
		__PseudoIntrin_DisableInterrupt();
		__PseudoIntrin_Halt();
	}

	return 0;
}

