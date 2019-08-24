
#pragma once

#define	REQUIREMENT_MEMORY_SIZE			0x2000000

#define	PAGE_SHIFT				12
#define	PAGE_SIZE				(1 << PAGE_SHIFT)

#define	SIZE_TO_PAGES(_sz)		( ((_sz) + (PAGE_SIZE - 1)) >> PAGE_SHIFT )
#define	PAGES_TO_SIZE(_cnt)		( (_cnt) << PAGE_SHIFT )


//
// Flags for PML4E, PDPTE, PDE, PTE. (4K paging)
//

#define	PAGE_EXECUTE_DISABLE	(1ULL << 63)
#define	PAGE_PRESENT			(1 << 0)
#define	PAGE_WRITABLE			(1 << 1)
#define	PAGE_USER				(1 << 2)
#define	PAGE_WRITE_THROUGH		(1 << 3)
#define	PAGE_CACHE_DISABLED		(1 << 4)

#define	PAGE_ACCESSED			(1 << 5)
#define	PAGE_ATTRIBUTE			(1 << 7) // for PTE with 4K paging only

// for PTE with 4K, PDE with 2M, PDPTE with 1G paging
#define	PAGE_DIRTY				(1 << 6) // PDPTE with 1G paging, 
#define	PAGE_LARGE				(1 << 7) // PDPTE with 1G paging, PDE with 2M paging
#define	PAGE_GLOBAL				(1 << 8)

#define	PAGE_PHYADDR_MASK		0x7ffffffff000ULL


INLINE
BOOLEAN
KERNELAPI
MmIsCanonicalAddress(
	IN PVOID VirtualAddress)
{
	UPTR p = (UPTR)VirtualAddress;
	UPTR Msb48 = 0x800000000000ULL;
	U16 HighPart = (U16)(p >> 48);
	U16 Compare = 0;

	Compare = !!(p & Msb48);
	Compare = ~Compare;

	return !!(HighPart == Compare);
}

typedef union _X64_VIRTUAL_ADDRESS {
	U64 Va;
	struct
	{
		U64 Offset : 12;
		U64 Table : 9;
		U64 Directory : 9;
		U64 DirectoryPtr : 9;
		U64 PML4 : 9;
		U64 SignExtended : 16;
	} Bits;
} X64_VIRTUAL_ADDRESS;



#define	MM_PDPTES_PER_PML4		512
#define	MM_PDES_PER_PDPTE		512
#define	MM_PTES_PER_PDE			512


BOOLEAN
KERNELAPI
MiDiscardFirmwareMemory(
	VOID);

VOID
KERNELAPI
MiInitializeMemoryMap(
	IN PVOID MemoryMap,
	IN SIZE_T MapCount,
	IN SIZE_T DescriptorSize,
	IN struct _PREINIT_PAGE_RESERVE *PageReserveList,
	IN SIZE_T ReserveListCount);


