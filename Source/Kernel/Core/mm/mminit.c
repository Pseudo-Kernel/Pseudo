
#include <Base.h>
#include <init/preinit.h>
#include <init/bootgfx.h>
#include <Mm/pool.h>
#include <Mm/mminit.h>

UPTR MiLoaderSpaceStart;
UPTR MiLoaderSpaceEnd;
UPTR MiPfnMetadataStart;
UPTR MiPfnMetadataEnd;
UPTR MiPfnListStart;
UPTR MiPfnListEnd;
UPTR MiVadListStart;
UPTR MiVadListEnd;
UPTR MiPteListStart;
UPTR MiPteListEnd;
UPTR MiSystemPoolStart;
UPTR MiSystemPoolEnd;


#if 0
BOOLEAN
KERNELAPI
MiInitializeMemory(
	VOID)
{
	U64 *PML4Base;
	SIZE_T Size;

	// Initializes the 4-Level paging.

	PML4Base = MmAllocatePool(PoolTypeNonPagedPreInit, PAGE_SIZE, PAGE_SIZE, TAG4('M', 'M', 'I', 'N'));

	// PML4 virtual base at 0xffffffff`fffff000
	PML4Base[]

	return FALSE;
}
#endif

#define	KERNEL_VASL_START_LOADER_SPACE					0xffff800000000000ULL
#define	KERNEL_VASL_END_LOADER_SPACE					0xffff800100000000ULL
#define	KERNEL_VASL_START_PFN_METADATA					0xffff8f0000000000ULL
#define	KERNEL_VASL_END_PFN_METADATA					0xffff900000000000ULL

#define	KERNEL_VASL_START_PFN_LIST						KERNEL_VASL_END_PFN_METADATA // 0xffff900000000000ULL
#define	KERNEL_VASL_END_PFN_LIST						0xffff910000000000ULL
#define	KERNEL_VASL_START_VAD_LIST						KERNEL_VASL_END_PFN_LIST // 0xffff910000000000ULL
#define	KERNEL_VASL_END_VAD_LIST						0xffffa10000000000ULL
#define	KERNEL_VASL_START_PTE_LIST						KERNEL_VASL_END_VAD_LIST
#define	KERNEL_VASL_END_PTE_LIST						0xffffa20000000000ULL
#define	KERNEL_VASL_START_PAGE_SWAP_SPACE_RESERVED		KERNEL_VASL_END_PTE_LIST
#define	KERNEL_VASL_END_PAGE_SWAP_SPACE_RESERVED		0xffffa30000000000ULL
#define	KERNEL_VASL_START_SYSTEM_POOL					KERNEL_VASL_END_PAGE_SWAP_SPACE_RESERVED

#define	KERNEL_VASL_PML4_BASE							0xfffffffffffff000ULL
#define	KERNEL_VASL_PDPT_BASE							0xffffffffffe00000ULL
#define	KERNEL_VASL_PD_BASE								0xffffffffc0000000ULL
#define	KERNEL_VASL_PT_BASE								0xffffff8000000000ULL


#define	KERNEL_VA_PML4_BASE								KERNEL_VA_LAYOUT_PML4_BASE
#define	KERNEL_VA_PDPT_BASE								KERNEL_VA_LAYOUT_PDPT_BASE
#define	KERNEL_VA_PD_BASE								KERNEL_VA_LAYOUT_PD_BASE
#define	KERNEL_VA_PT_BASE								KERNEL_VA_LAYOUT_PT_BASE


VOID
KERNELAPI
MiInitializeKernelVaSpaceLayout(
	VOID)
{
	MiLoaderSpaceStart = KERNEL_VASL_START_LOADER_SPACE;
	MiLoaderSpaceEnd = KERNEL_VASL_END_LOADER_SPACE;
	MiPfnMetadataStart = KERNEL_VASL_START_PFN_METADATA;
	MiPfnMetadataEnd = KERNEL_VASL_END_PFN_METADATA;
	MiPfnListStart = KERNEL_VASL_START_PFN_LIST;
	MiPfnListEnd = KERNEL_VASL_END_PFN_LIST;
	MiVadListStart = KERNEL_VASL_START_VAD_LIST;
	MiVadListEnd = KERNEL_VASL_END_VAD_LIST;
	MiPteListStart = KERNEL_VASL_START_PTE_LIST;
	MiPteListEnd = KERNEL_VASL_END_PTE_LIST;
	MiSystemPoolStart = KERNEL_VASL_START_SYSTEM_POOL;
	MiSystemPoolEnd = 0; // dynamic
}

BOOLEAN
KERNELAPI
MiDiscardFirmwareMemory(
	VOID)
{
	return FALSE;
}


#define	PAGE_LOWER_MAP_GET(_map, _idx, _lower_map)	{			\
	/* _entry type must be (U64 *) */							\
	if(!( (_map)[_idx] & PAGE_PRESENT ))	{					\
		(_lower_map) = MmAllocatePool(PoolTypeNonPagedPreInit,	\
			PAGE_SIZE, PAGE_SIZE, TAG4('I', 'N', 'I', 'T'));	\
		DbgTraceF(TraceLevelDebug, "Allocated %s = 0x%llx\n",	\
			_ASCII(_lower_map), (U64)(_lower_map));				\
		if(!(_lower_map)) {										\
			BootGfxFatalStop("Failed to create page mapping");	\
		}														\
		memset((_lower_map), 0, PAGE_SIZE);						\
		/* New entry */											\
		(_map)[_idx] = ((UPTR)(_lower_map)) |					\
			PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;			\
	}															\
	(_lower_map) = (U64 *)((_map)[_idx] & PAGE_PHYADDR_MASK);	\
}																\

VOID
KERNELAPI
MiX64AddPageMapping(
	IN U64 *PML4Base, 
	IN UPTR VirtualAddress, 
	IN UPTR PhysicalAddress, 
	IN SIZE_T Size, 
	IN UPTR PteFlag, 
	IN BOOLEAN IgnoreAlignment, 
	IN BOOLEAN IgnoreIfExists)
{
	UPTR VaStart;
	UPTR VaEnd;
	X64_VIRTUAL_ADDRESS Va;
	UPTR Pa;

	DbgTraceF(TraceLevelDebug, "%s (%llX, %llX, %llX, %llX, %llX, %d, %d)\n", __FUNCTION__, 
		PML4Base, VirtualAddress, PhysicalAddress, Size, PteFlag, IgnoreAlignment, IgnoreIfExists);

	VaStart = VirtualAddress;
	VaEnd = VirtualAddress + Size;
	Pa = PhysicalAddress;

	if (IgnoreAlignment)
	{
		VaStart &= ~(PAGE_SIZE - 1);
		VaEnd = (VaEnd + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
		Pa &= ~(PAGE_SIZE - 1);
	}

	DASSERT(!(VaStart & (PAGE_SIZE - 1)));
	DASSERT(!(Pa & (PAGE_SIZE - 1)));

	PteFlag &= ~PAGE_PHYADDR_MASK;

	for (Va.Va = VaStart; Va.Va < VaEnd; Va.Va += PAGE_SIZE)
	{
		U64 *PDPT;
		U64 *PD;
		U64 *PT;

		// PML4 -> PDPT
		PAGE_LOWER_MAP_GET(PML4Base, Va.Bits.PML4, PDPT);

		// PDPT -> PD
		PAGE_LOWER_MAP_GET(PDPT, Va.Bits.DirectoryPtr, PD);

		// PD -> PT
		PAGE_LOWER_MAP_GET(PD, Va.Bits.Directory, PT);

		if (PT[Va.Bits.Table] & PAGE_PRESENT)
		{
			if (!IgnoreIfExists)
			{
				DASSERT(FALSE);
			}
		}

		PT[Va.Bits.Table] = Pa | PteFlag;

		Pa += PAGE_SIZE;
	}
}

U64
KERNELAPI
MiEfiMapAttributeToPxeFlag(
	IN U64 Attribute)
{
	U64 PxeFlag = 0;

	if (Attribute & EFI_MEMORY_UC)
		PxeFlag |= PAGE_CACHE_DISABLED;

	if (Attribute & EFI_MEMORY_WC) // Write-Coalescing
		;

	if (Attribute & EFI_MEMORY_WT) // Write-Through
		PxeFlag |= PAGE_WRITE_THROUGH;

	if (Attribute & EFI_MEMORY_WB) // Write-Back
		;

	if (Attribute & EFI_MEMORY_UCE) // Uncached??
		;

	if (!(Attribute & EFI_MEMORY_WP)) // Write-Protected
		PxeFlag |= PAGE_WRITABLE;

	if (Attribute & EFI_MEMORY_RP) // Read-Protected
		;

	if (Attribute & EFI_MEMORY_XP) // Execute-Protected
		PxeFlag |= PAGE_EXECUTE_DISABLE;

	if (Attribute & EFI_MEMORY_RO) // ????
		;

	if (Attribute & EFI_MEMORY_NV) // Non-volatile
		;

	if (Attribute & EFI_MEMORY_MORE_RELIABLE) // ????
		;

	if (Attribute & EFI_MEMORY_RUNTIME) // Runtime
		;

	return PxeFlag;
}

VOID
KERNELAPI
MiInitializeMemoryMap(
	IN PVOID MemoryMap,
	IN SIZE_T MapCount,
	IN SIZE_T DescriptorSize, 
	IN struct _PREINIT_PAGE_RESERVE *PageReserveList, 
	IN SIZE_T ReserveListCount)
{
	// Initialize the memory map.

	EFI_MEMORY_DESCRIPTOR *Entry;
	SIZE_T SystemMemoryUsable = 0;
	U64 *PML4;

	Entry = (EFI_MEMORY_DESCRIPTOR *)MemoryMap;
	for (SIZE_T i = 0; i < MapCount; i++)
	{
		switch (Entry->Type)
		{
		case EfiLoaderCode:
		case EfiBootServicesCode:
		case EfiBootServicesData:
		case EfiConventionalMemory:
			// Usable
			SystemMemoryUsable += Entry->NumberOfPages;
			break;

		case EfiLoaderData:
			// Usable data or reserved by bootloader
			SystemMemoryUsable += Entry->NumberOfPages;
			break;

		case EfiRuntimeServicesCode:
		case EfiRuntimeServicesData:
		case EfiACPIReclaimMemory:
		case EfiACPIMemoryNVS:
		case EfiPersistentMemory:
			// Usable but already used
			SystemMemoryUsable += Entry->NumberOfPages;
			break;

		case EfiUnusableMemory:
		case EfiMemoryMappedIO:
		case EfiMemoryMappedIOPortSpace:
		case EfiPalCode:
		default: // vendor-specific
			// Unusable
			break;
		}


		// Move to the next entry.
		Entry = (EFI_MEMORY_DESCRIPTOR *)((UPTR)Entry + DescriptorSize);
	}

	SystemMemoryUsable <<= PAGE_SHIFT;

	MiLoaderSpaceStart = KERNEL_VASL_START_LOADER_SPACE;
	MiLoaderSpaceEnd = KERNEL_VASL_END_LOADER_SPACE;
	MiPfnMetadataStart = KERNEL_VASL_START_PFN_METADATA;
	MiPfnMetadataEnd = KERNEL_VASL_END_PFN_METADATA;
	MiPfnListStart = KERNEL_VASL_START_PFN_LIST;
	MiPfnListEnd = KERNEL_VASL_END_PFN_LIST;
	MiVadListStart = KERNEL_VASL_START_VAD_LIST;
	MiVadListEnd = KERNEL_VASL_END_VAD_LIST;
	MiPteListStart = KERNEL_VASL_START_PTE_LIST;
	MiPteListEnd = KERNEL_VASL_END_PTE_LIST;
	MiSystemPoolStart = KERNEL_VASL_START_SYSTEM_POOL;
	MiSystemPoolEnd = KERNEL_VASL_START_SYSTEM_POOL + (SystemMemoryUsable * 15) / 100; // dynamic
	MiSystemPoolEnd &= ~(PAGE_SIZE - 1);

	DbgTraceF(TraceLevelEvent, "Usable memory %d bytes\n", SystemMemoryUsable);
	DbgTraceF(TraceLevelEvent, "Range List (Start, End):\n");
	DbgTraceF(TraceLevelEvent, "LoaderSpace       = 0x%p, 0x%p\n", MiLoaderSpaceStart, MiLoaderSpaceEnd);
	DbgTraceF(TraceLevelEvent, "PfnMetadata       = 0x%p, 0x%p\n", MiPfnMetadataStart, MiPfnMetadataEnd);
	DbgTraceF(TraceLevelEvent, "PfnListStart      = 0x%p, 0x%p\n", MiPfnListStart, MiPfnListEnd);
	DbgTraceF(TraceLevelEvent, "VadListStart      = 0x%p, 0x%p\n", MiVadListStart, MiVadListEnd);
	DbgTraceF(TraceLevelEvent, "PteListStart      = 0x%p, 0x%p\n", MiPteListStart, MiPteListEnd);
	DbgTraceF(TraceLevelEvent, "SystemPoolStart   = 0x%p, 0x%p\n", MiSystemPoolStart, MiSystemPoolEnd);
	DbgTraceF(TraceLevelEvent, "\n");

	if (SystemMemoryUsable < REQUIREMENT_MEMORY_SIZE)
	{
		BootGfxFatalStop("Not enough system memory (Expected = %lld, Current = %lld)",
			(SIZE_T)REQUIREMENT_MEMORY_SIZE, SystemMemoryUsable);
	}


	// 512 entries of PML4E.
	PML4 = MmAllocatePool(PoolTypeNonPagedPreInit, PAGE_SIZE, PAGE_SIZE, TAG4('I', 'N', 'I', 'T'));
	memset(PML4, 0, PAGE_SIZE);

	if (!PML4)
	{
		BootGfxFatalStop("Failed to allocate PML4");
	}

	//
	// Following address ranges must be identity mapped:
	// 1. Address described in PageReserveList[]
	// 2. Address which is used by firmware, ACPI, or device.
	//    (EfiACPIReclaimMemory, EfiACPIMemoryNVS, EfiPersistentMemory, EfiMemoryMappedIO, 
	//     EfiMemoryMappedIOPortSpace, EfiPalCode, EfiRuntimeServicesCode, EfiRuntimeServicesData, 
	//     EfiUnusableMemory, other vendor-specific types)
	//

	DbgTraceF(TraceLevelDebug, "Page reserve list count = %lld\n", ReserveListCount);

	for (SIZE_T i = 0; i < ReserveListCount; i++)
	{
		DbgTraceF(TraceLevelDebug, "Adding identity page mapping 0x%llX, size 0x%llX\n",
			PageReserveList[i].BaseAddress, PageReserveList[i].Size);

		MiX64AddPageMapping(
			PML4, 
			PageReserveList[i].BaseAddress, 
			PageReserveList[i].BaseAddress,
			PageReserveList[i].Size, 
			PAGE_PRESENT | PAGE_WRITABLE, 
			TRUE,
			FALSE);
	}

	Entry = (EFI_MEMORY_DESCRIPTOR *)MemoryMap;
	for (SIZE_T i = 0; i < MapCount; i++)
	{
		switch (Entry->Type)
		{
		case EfiLoaderCode:
		case EfiBootServicesCode:
		case EfiBootServicesData:
		case EfiConventionalMemory:
		case EfiLoaderData:
			break;

		case EfiRuntimeServicesCode:
		case EfiRuntimeServicesData:
		case EfiACPIReclaimMemory:
		case EfiACPIMemoryNVS:
		case EfiPersistentMemory:
		case EfiUnusableMemory:
		case EfiMemoryMappedIO:
		case EfiMemoryMappedIOPortSpace:
		case EfiPalCode:
		default: // vendor-specific
			// Identity mapping
			DbgTraceF(TraceLevelDebug, 
				"Adding page mapping physical 0x%llX -> virtual 0x%llX, size 0x%llX, attributes 0x%llX\n",
				Entry->PhysicalStart, Entry->PhysicalStart, PAGES_TO_SIZE(Entry->NumberOfPages),
				Entry->Attribute);

			MiX64AddPageMapping(
				PML4,
				Entry->PhysicalStart, // normally Entry->VirtualStart == 0
				Entry->PhysicalStart,
				PAGES_TO_SIZE(Entry->NumberOfPages),
				MiEfiMapAttributeToPxeFlag(Entry->Attribute) | PAGE_PRESENT, 
				TRUE,
				FALSE);
			break;
		}


		// Move to the next entry.
		Entry = (EFI_MEMORY_DESCRIPTOR *)((UPTR)Entry + DescriptorSize);
	}

	DbgTraceF(TraceLevelDebug, "Identity mapping done.\n");
}

