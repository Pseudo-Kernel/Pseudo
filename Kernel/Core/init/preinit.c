
#include <Base.h>
#include <init/preinit.h>
#include <init/bootgfx.h>
#include <init/zip.h>
#include <Mm/pool.h>
#include <Mm/mminit.h>

typedef struct _MMPAGE {
	U64 Vpn;
	U64 Pfn;
	U64 Count;
} MMPAGE, *PMMPAGE;



OS_LOADER_BLOCK PiLoaderBlockTemporary;
PREINIT_PAGE_RESERVE PiPageReserve[32]; // Page list which must not be discarded
U32 PiPageReserveCount = 0;

ZIP_CONTEXT PiBootImageContext; // Boot image context


#define	PREINIT_PAGE_RESERVE_ADD(_addr, _size)	\
{												\
	PiPageReserve[PiPageReserveCount].BaseAddress = (UPTR)(_addr);	\
	PiPageReserve[PiPageReserveCount].Size = (SIZE_T)(_size);		\
	PiPageReserveCount++;											\
}


__attribute__((naked))
VOID
KERNELAPI
PiPreStackSwitch(
	IN OS_LOADER_BLOCK *LoaderBlock,
	IN U32 SizeOfLoaderBlock, 
	IN PVOID SafeStackTop)
{
	__asm__ __volatile__(
		"mov rcx, %0\n\t"
		"mov edx, %1\n\t"
		"mov r8, %2\n\t"
		"and r8, not 0x07\n\t"				// align to 16-byte boundary
		"sub r8, 0x20\n\t"					// shadow area
		"mov qword ptr [r8], rcx\n\t"		// 1st param
		"mov qword ptr [r8+0x08], rdx\n\t"	// 2nd param
		"xchg r8, rsp\n\t"					// switch the stack
		"call PiPreInitialize\n\t"
		"\n\t"								// MUST NOT REACH HERE!
		"__InitFailed:\n\t"
		"cli\n\t"
		"hlt\n\t"
		"jmp __InitFailed\n\t"
		:
		: "r"(LoaderBlock), "r"(SizeOfLoaderBlock), "r"(SafeStackTop)
		: "memory"
	);
}

VOID
KERNELAPI
PiPreInitialize(
	IN OS_LOADER_BLOCK *LoaderBlock,
	IN U32 LoaderBlockSize)
{
	OS_LOADER_BLOCK *LoaderBlockTemp;

	DbgTraceF(TraceLevelDebug, "%s (%p, %X)\n", __FUNCTION__, LoaderBlock, LoaderBlockSize);

	if (sizeof(*LoaderBlock) != LoaderBlockSize)
	{
		BootGfxFatalStop("Loader block size mismatch\n");
	}

	LoaderBlockTemp = &PiLoaderBlockTemporary;
	*LoaderBlockTemp = *LoaderBlock;

	LoaderBlockTemp->Base.BootServices = NULL;
	LoaderBlockTemp->Base.ImageHandle = NULL;
	LoaderBlockTemp->Base.RootDirectory = NULL;

	PREINIT_PAGE_RESERVE_ADD(LoaderBlockTemp->LoaderData.KernelPhysicalBase, LoaderBlockTemp->LoaderData.MappedSize);
	PREINIT_PAGE_RESERVE_ADD(LoaderBlockTemp->LoaderData.TempBase, LoaderBlockTemp->LoaderData.TempSize);
	PREINIT_PAGE_RESERVE_ADD(LoaderBlockTemp->LoaderData.ShadowBase, LoaderBlockTemp->LoaderData.ShadowSize);
	PREINIT_PAGE_RESERVE_ADD(LoaderBlockTemp->LoaderData.BootImageBase, LoaderBlockTemp->LoaderData.BootImageSize);
	PREINIT_PAGE_RESERVE_ADD(LoaderBlockTemp->LoaderData.VideoModeBase, LoaderBlockTemp->LoaderData.VideoModeSize);
	PREINIT_PAGE_RESERVE_ADD(LoaderBlockTemp->Memory.Map, LoaderBlockTemp->Memory.DescriptorSize * LoaderBlockTemp->Memory.MapCount);
	ASSERT(PiPageReserveCount < ARRAY_SIZE(PiPageReserve));

	//
	// Initialize the pre-init pool.
	//

	if (!MiPreInitialize(LoaderBlockTemp))
	{
		BootGfxFatalStop("Failed to initialize pre-init pool");
	}

	//
	// Initialize the boot image context.
	//

	if (!ZipInitializeReaderContext(&PiBootImageContext,
		(VOID *)LoaderBlockTemp->LoaderData.BootImageBase,
		(U32)LoaderBlockTemp->LoaderData.BootImageSize))
	{
		BootGfxFatalStop("Invalid boot image");
	}

	//
	// Initialize the pre-init graphics.
	//

	if (!BootGfxInitialize(
		(PVOID)LoaderBlockTemp->LoaderData.VideoModeBase,
		(U32)LoaderBlockTemp->LoaderData.VideoModeSize,
		LoaderBlockTemp->LoaderData.VideoModeSelected,
		(PVOID)LoaderBlockTemp->LoaderData.VideoFramebufferBase,
		LoaderBlockTemp->LoaderData.VideoFramebufferSize,
		&PiBootImageContext))
	{
		BootGfxFatalStop("Failed to initialize graphics");
	}

	//
	// Initialize the memory mapping.
	//

	MiInitializeMemoryMap(
		LoaderBlock->Memory.Map, 
		LoaderBlock->Memory.MapCount, 
		LoaderBlock->Memory.DescriptorSize, 
		PiPageReserve, 
		PiPageReserveCount);

	BootGfxPrintTextFormat("Memory mapping done.\n");

	for (;;)
	{
		__PseudoIntrin_DisableInterrupt();
		__PseudoIntrin_Halt();
	}

//	if (!PiDiscardFirmwareMemory())
//	{
//		DbgTraceF(TraceLevelError, "Failed to free unused memory\n");
//		return FALSE;
//	}
}

