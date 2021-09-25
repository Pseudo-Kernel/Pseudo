
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
 *       - Interrupt registration and dispatch
 *       - Processor initialization (IOAPIC, LAPIC, per-processor data and tables)
 */

#include <base/base.h>
#include <init/preinit.h>
#include <init/bootgfx.h>
#include <mm/mminit.h>
#include <mm/pool.h>

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
	DbgInitialize(LoaderBlock, TraceLevelDebug);

	DbgTraceF(TraceLevelDebug, "%s (%p, %p, %X, %p)\n",
		__FUNCTION__, LoadedBase, LoaderBlock, SizeOfLoaderBlock, Reserved);

    PiPreInitialize(LoaderBlock, SizeOfLoaderBlock);

    MmInitialize();


    BGXTRACE_C(BGX_COLOR_LIGHT_YELLOW, "NonPagedPool/PagedPool allocation test\n");

    PVOID Test = NULL;
    Test = MmAllocatePool(PoolTypeNonPaged, 0x1234, 0, 0);
    BGXTRACE_C(BGX_COLOR_LIGHT_YELLOW, "NonPagedPool allocation result 0x%llx\n", Test);

    if (Test)
    {
        MmFreePool(Test);
    }

    Test = MmAllocatePool(PoolTypePaged, 0x4321, 0, 0);
    BGXTRACE_C(BGX_COLOR_LIGHT_YELLOW, "PagedPool allocation result 0x%llx\n", Test);

    if (Test)
    {
        MmFreePool(Test);
    }

    BGXTRACE_C(BGX_COLOR_LIGHT_YELLOW, "Test done. System halt.\n");

	for (;;)
	{
		__PseudoIntrin_DisableInterrupt();
		__PseudoIntrin_Halt();
	}

	return 0;
}

