
#include <Base.h>
#include <init/preinit.h>
#include <init/bootgfx.h>

#pragma comment(linker, "/Entry:KiKernelStart")

/*
	PseudoOS Virtual Memory Map

	1KiB = 00000000`00000400
	1MiB = 00000000`00100000
	1GiB = 00000000`40000000
	1TiB = 00000100`00000000
	1PiB = 00040000`00000000
	1EiB = 10000000`00000000

	00000000`00000000 - 00000000`00000fff (00000000`00001000 = 4KiB)                Inaccessible zero page
	00000000`00001000 - 00000000`ffffffff (00000000`ffffefff = 4GiB - 4KiB)         Reserved below 4GiB
	00000001`00000000 - 00007ffe`ffffffff (00007ffe`00000000 = 128TiB - 8GiB)       User pages
	00007fff`00000000 - 00007fff`ffffffff (00000001`00000000 = 4GiB)                User/Kernel shared data

	00008000`00000000 - ffff7fff`ffffffff (ffff0000`00000000 = 16EiB - 256TiB)      Intel reserved

	ffff8000`00000000 - ffff8000`ffffffff (00000001`00000000 = 4GiB)                Loader space (Pre-boot mappings)
	 + Kernel base, Boot image, Pre-init memory pool, etc.
	ffff8001`00000000 - ffff8eff`ffffffff                                           Reserved for future use
	ffff8f00`00000000 - ffff8fff`ffffffff (00000100`00000000 = 1TiB)                PFN management data
	ffff9000`00000000 - ffff90ff`ffffffff (00000100`00000000 = 1TiB)                PFN list
	ffff9100`00000000 - ffffa0ff`ffffffff (00001000`00000000 = 16TiB)               VAD list

	ffffa100`00000000 - ffffa1ff`ffffffff (00000100`00000000 = 1TiB)                Page table, Page directory,
	                                                                                Page directory pointer

	ffffa200`00000000 - ffffa2ff`ffffffff (00000100`00000000 = 1TiB)                Reserved for page swap
	ffffa300`00000000 - ????????`????????                                           Kernel pool area
	????????`???????? - ffffff7f`ffffffff                                           Reserved
	ffffff80`00000000 - ffffffff`ffffffff (00000080`00000000 = 512GiB)              Recursive page table mapping
	 + ffffff80`00000000 - ffffffff`ffffffff           PT base
	 + ffffffff`c0000000 - ffffffff`ffffffff           PD base
	 + ffffffff`ffe00000 - ffffffff`ffffffff           PDPT base
	 + ffffffff`fffff000 - ffffffff`ffffffff           PML4 base
	*/




U64
KERNELAPI
KiKernelStart(
	IN PTR LoadedBase, 
	IN OS_LOADER_BLOCK *LoaderBlock, 
	IN U32 SizeOfLoaderBlock, 
	IN PTR Reserved)
{
	UPTR SafeStackTop;

	DbgInitialize(TraceLevelDebug);

	DbgNullBreak();

	DbgTraceF(TraceLevelDebug, "%s (%p, %p, %X, %p)\n",
		__FUNCTION__, LoadedBase, LoaderBlock, SizeOfLoaderBlock, Reserved);

	SafeStackTop = LoaderBlock->LoaderData.KernelStackBase + LoaderBlock->LoaderData.StackSize - 0x10;
	PiPreStackSwitch(LoaderBlock, SizeOfLoaderBlock, (PVOID)SafeStackTop);



	for (;;)
	{
		__PseudoIntrin_DisableInterrupt();
		__PseudoIntrin_Halt();
	}

	return 0;
}

