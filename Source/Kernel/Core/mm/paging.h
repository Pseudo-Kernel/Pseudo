
#pragma once


#define PAGE_SHIFT              12
#define PAGE_SIZE               (1 << PAGE_SHIFT)
#define PAGE_MASK               (PAGE_SIZE - 1)

#define PAGE_SHIFT_2M           21
#define PAGE_SIZE_2M            (1 << PAGE_SHIFT_2M)
#define PAGE_MASK_2M            (PAGE_SIZE_2M - 1)

#define PAGE_SHIFT_1G           30
#define PAGE_SIZE_1G            (1 << PAGE_SHIFT_1G)
#define PAGE_MASK_1G            (PAGE_SIZE_1G - 1)

#define SIZE_TO_PAGES(_sz)      ( ((_sz) + PAGE_MASK) >> PAGE_SHIFT )
#define PAGES_TO_SIZE(_cnt)     ( (_cnt) << PAGE_SHIFT )

#define SIZE_TO_PAGES_2M(_sz)   ( ((_sz) + PAGE_MASK_2M) >> PAGE_SHIFT_2M )
#define PAGES_TO_SIZE_2M(_cnt)  ( (_cnt) << PAGE_SHIFT_2M )

#define SIZE_TO_PAGES_1G(_sz)   ( ((_sz) + PAGE_MASK_1G) >> PAGE_SHIFT_1G )
#define PAGES_TO_SIZE_1G(_cnt)  ( (_cnt) << PAGE_SHIFT_1G )

//
// Flags for PML4E, PDPTE, PDE, PTE. (4K paging)
//

#define PAGE_TO_PAGE_NUMBER_4K(_va)             (((U64)(_va)) >> PAGE_SHIFT)

#define ARCH_X64_PAGE_NUMBER_TO_PML4EI(_va)     ((((U64)(_va)) >> 27) & 0x1ff)
#define ARCH_X64_PAGE_NUMBER_TO_PDPTEI(_va)     ((((U64)(_va)) >> 18) & 0x1ff)
#define ARCH_X64_PAGE_NUMBER_TO_PDEI(_va)       ((((U64)(_va)) >> 9) & 0x1ff)
#define ARCH_X64_PAGE_NUMBER_TO_PTEI(_va)       ((((U64)(_va)) >> 0) & 0x1ff)

#define ARCH_X64_ADDRESS_TO_PML4EI(_va)         ARCH_X64_PAGE_NUMBER_TO_PML4EI( PAGE_TO_PAGE_NUMBER_4K(_va) )
#define ARCH_X64_ADDRESS_TO_PDPTEI(_va)         ARCH_X64_PAGE_NUMBER_TO_PDPTEI( PAGE_TO_PAGE_NUMBER_4K(_va) )
#define ARCH_X64_ADDRESS_TO_PDEI(_va)           ARCH_X64_PAGE_NUMBER_TO_PDEI( PAGE_TO_PAGE_NUMBER_4K(_va) )
#define ARCH_X64_ADDRESS_TO_PTEI(_va)           ARCH_X64_PAGE_NUMBER_TO_PTEI( PAGE_TO_PAGE_NUMBER_4K(_va) )
#define ARCH_X64_ADDRESS_TO_OFFSET(_va)         (((U64)(_va)) & PAGE_MASK)

#define ARCH_X64_PXE_PRESENT                    (1ULL << 0)
#define ARCH_X64_PXE_WRITABLE                   (1ULL << 1)
#define ARCH_X64_PXE_USER                       (1ULL << 2)
#define ARCH_X64_PXE_WRITE_THROUGH              (1ULL << 3)
#define ARCH_X64_PXE_CACHE_DISABLED             (1ULL << 4)
#define ARCH_X64_PXE_LARGE_SIZE                 (1ULL << 7)
#define ARCH_X64_PXE_EXECUTE_DISABLED           (1ULL << 63)

#define ARCH_X64_PXE_4K_BASE_MASK               0x000ffffffffff000ULL // [51:12], 4K
#define ARCH_X64_PXE_2M_BASE_MASK               0x000fffffffe00000ULL // [51:21], 2M











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



extern U64 *MiPML4TBase; //!< PML4 table base.


BOOLEAN
KERNELAPI
MiArchX64SetPageMapping(
    IN U64 *PML4TBase, 
    IN VIRTUAL_ADDRESS VirtualAddress, 
    IN PHYSICAL_ADDRESS PhysicalAddress, 
    IN SIZE_T Size, 
    IN U64 PteFlags,
    IN BOOLEAN ReverseMapping,
    IN BOOLEAN AllowNonDefaultPageSize);

BOOLEAN
KERNELAPI
MiArchX64SetPageMappingNotPresent(
    IN U64 *PML4TBase, 
    IN VIRTUAL_ADDRESS VirtualAddress, 
    IN SIZE_T Size);

BOOLEAN
KERNELAPI
MiArchX64IsPageMappingExists(
    IN U64 *PML4TBase, 
    IN EFI_VIRTUAL_ADDRESS VirtualAddress, 
    IN SIZE_T Size);



