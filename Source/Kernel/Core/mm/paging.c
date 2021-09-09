
/**
 * @file paging.c
 * @author Pseudo (sandbox.isolated@gmail.com)
 * @brief Implements paging-related routines.
 * @version 0.1
 * @date 2021-09-10
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <base/base.h>
#include <mm/pool.h>
#include <mm/mminit.h>
#include <mm/paging.h>

U64 *MiPML4TBase; //!< PML4 table base.



#define PAGE_LOWER_MAP_GET(_map, _idx, _lower_map)  {           \
    /* _entry type must be (U64 *) */                           \
    if(!( (_map)[_idx] & PAGE_PRESENT ))    {                   \
        (_lower_map) = MmAllocatePool(PoolTypeNonPagedPreInit,  \
            PAGE_SIZE, PAGE_SIZE, TAG4('I', 'N', 'I', 'T'));    \
        DbgTraceF(TraceLevelDebug, "Allocated %s = 0x%llx\n",   \
            _ASCII(_lower_map), (U64)(_lower_map));             \
        if(!(_lower_map)) {                                     \
            BootGfxFatalStop("Failed to create page mapping");  \
        }                                                       \
        memset((_lower_map), 0, PAGE_SIZE);                     \
        /* New entry */                                         \
        (_map)[_idx] = ((UPTR)(_lower_map)) |                   \
            PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;           \
    }                                                           \
    (_lower_map) = (U64 *)((_map)[_idx] & PAGE_PHYADDR_MASK);   \
}                                                               \

VOID
KERNELAPI
MiArchX64AddPageMapping(
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

