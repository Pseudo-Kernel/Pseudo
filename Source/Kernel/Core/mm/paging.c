
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
#include <ke/lock.h>
#include <mm/pool.h>
#include <mm/mminit.h>
#include <misc/objpool.h>
#include <mm/paging.h>

U64 *MiPML4TBase; //!< PML4 table base.
U64 *MiRPML4TBase; //!< Reverse PML4 table base.

OBJECT_POOL MiPreInitPxePool; //!< Pre-init PXE pool


/**
 * @brief Allocates the 512 entries of PXE.
 * 
 * @return Address of allocated PXE entries.\n
 *         NULL is returned when allocation fails.
 */
U64 *
KERNELAPI
MiAllocatePxePreInit(
    VOID)
{
    return PoolAllocateObject(&MiPreInitPxePool);
}

/**
 * @brief Invalidates the TLB for given address.
 * 
 * @param [in] InvalidateAddress    Address to be invalidated.
 * 
 * @return None.
 */
static
VOID
KERNELAPI
MiArchX64InvalidateSinglePage(
    IN VIRTUAL_ADDRESS InvalidateAddress)
{
    __invlpg((void *)InvalidateAddress);
/*
    __asm__ __volatile__ (
        "invlpg [%0]"
        :
        : "r"(InvalidateAddress)
        : "memory"
    );
*/
}

/**
 * @brief Invalidates the TLB for given address range.
 * 
 * @param [in] InvalidateAddress    Address to be invalidated.
 * @param [in] Size                 Size to be invalidated.
 * 
 * @return None.
 */
VOID
KERNELAPI
MiArchX64InvalidatePage(
    IN VIRTUAL_ADDRESS InvalidateAddress, 
    IN SIZE_T Size)
{
    for (SIZE_T Offset = 0; Offset < Size; Offset += PAGE_SIZE)
    {
        MiArchX64InvalidateSinglePage(InvalidateAddress);
        InvalidateAddress += Size;
    }
}

/**
 * @brief Translates the physical address to virtual address.
 * 
 * @param [in] TopLevelTableReverse Pointer to top-level reverse mapping structure.
 * @param [in] SourceAddress        Physical address to be translated.
 * @param [out] DestinationAddress  Pointer to caller-supplied variable to receive translated virtual address.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
MiTranslatePhysicalToVirtual(
    IN U64 *TopLevelTableReverse,
    IN PHYSICAL_ADDRESS SourceAddress, 
    OUT VIRTUAL_ADDRESS *DestinationAddress)
{
    U64 SourcePageNumber = PAGE_TO_PAGE_NUMBER_4K(SourceAddress);

    //
    // Find address mapping.
    //

    U64 PML4TEi = ARCH_X64_PAGE_NUMBER_TO_PML4EI(SourcePageNumber);
    U64 PDPTEi = ARCH_X64_PAGE_NUMBER_TO_PDPTEI(SourcePageNumber);
    U64 PDEi = ARCH_X64_PAGE_NUMBER_TO_PDEI(SourcePageNumber);
    U64 PTEi = ARCH_X64_PAGE_NUMBER_TO_PTEI(SourcePageNumber);

    U64 PML4TE = TopLevelTableReverse[PML4TEi];
    if (!(PML4TE & ARCH_X64_PXE_PRESENT))
    {
        return FALSE;
    }

    // PML4T -> PDPT
    U64 *PDPTBase = (U64 *)ARCH_X64_TO_CANONICAL_ADDRESS(PML4TE & ARCH_X64_PXE_4K_BASE_MASK);
    U64 PDPTE = PDPTBase[PDPTEi];
    if (!(PDPTE & ARCH_X64_PXE_PRESENT))
    {
        return FALSE;
    }

    // PDPT -> PD
    U64 *PDBase = (U64 *)ARCH_X64_TO_CANONICAL_ADDRESS(PDPTE & ARCH_X64_PXE_4K_BASE_MASK);
    U64 PDE = PDBase[PDEi];
    if (!(PDE & ARCH_X64_PXE_PRESENT))
    {
        return FALSE;
    }

    if (PDE & ARCH_X64_PXE_LARGE_SIZE)
    {
        // 2M page
        *DestinationAddress = (SourceAddress & PAGE_MASK_2M) | 
            ARCH_X64_TO_CANONICAL_ADDRESS(PDE & ARCH_X64_PXE_2M_BASE_MASK);
    }
    else
    {
        // PD -> PT
        U64 *PTBase = (U64 *)ARCH_X64_TO_CANONICAL_ADDRESS(PDE & ARCH_X64_PXE_4K_BASE_MASK);
        U64 PTE = PTBase[PTEi];

        if (!(PTE & ARCH_X64_PXE_PRESENT))
        {
            return FALSE;
        }

        *DestinationAddress = (SourceAddress & PAGE_MASK) | 
            ARCH_X64_TO_CANONICAL_ADDRESS(PTE & ARCH_X64_PXE_4K_BASE_MASK);
    }

    return TRUE;
}

/**
 * @brief Translates the virtual address to physical address.
 * 
 * @param [in] TopLevelTable        Pointer to top-level paging structure.
 * @param [in] TopLevelTableReverse Pointer to top-level reverse mapping structure.
 * @param [in] SourceAddress        Virtual address to be translated.
 * @param [out] DestinationAddress  Pointer to caller-supplied variable to receive translated physical address.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
MiTranslateVirtualToPhysical(
    IN U64 *TopLevelTable,
    IN U64 *TopLevelTableReverse,
    IN VIRTUAL_ADDRESS SourceAddress, 
    OUT PHYSICAL_ADDRESS *DestinationAddress)
{
    U64 SourcePageNumber = PAGE_TO_PAGE_NUMBER_4K(SourceAddress);

    //
    // Find address mapping.
    //

    U64 PML4TEi = ARCH_X64_PAGE_NUMBER_TO_PML4EI(SourcePageNumber);
    U64 PDPTEi = ARCH_X64_PAGE_NUMBER_TO_PDPTEI(SourcePageNumber);
    U64 PDEi = ARCH_X64_PAGE_NUMBER_TO_PDEI(SourcePageNumber);
    U64 PTEi = ARCH_X64_PAGE_NUMBER_TO_PTEI(SourcePageNumber);

    U64 PML4TE = TopLevelTable[PML4TEi];
    if (!(PML4TE & ARCH_X64_PXE_PRESENT))
    {
        return FALSE;
    }

    // PML4T -> PDPT
    U64 *PDPTBase = NULL;
    if (!MiTranslatePhysicalToVirtual(TopLevelTableReverse, 
        (PML4TE & ARCH_X64_PXE_4K_BASE_MASK), (VIRTUAL_ADDRESS *)&PDPTBase))
    {
        return FALSE;
    }

    U64 PDPTE = PDPTBase[PDPTEi];
    if (!(PDPTE & ARCH_X64_PXE_PRESENT))
    {
        return FALSE;
    }

    // PDPT -> PD
    U64 *PDBase = NULL;
    if (!MiTranslatePhysicalToVirtual(TopLevelTableReverse, 
        (PDPTE & ARCH_X64_PXE_4K_BASE_MASK), (VIRTUAL_ADDRESS *)&PDBase))
    {
        return FALSE;
    }

    U64 PDE = PDBase[PDEi];
    if (!(PDE & ARCH_X64_PXE_PRESENT))
    {
        return FALSE;
    }

    if (PDE & ARCH_X64_PXE_LARGE_SIZE)
    {
        // 2M page
        *DestinationAddress = (SourceAddress & PAGE_MASK_2M) | 
            ARCH_X64_TO_CANONICAL_ADDRESS(PDE & ARCH_X64_PXE_2M_BASE_MASK);
    }
    else
    {
        // PD -> PT
        U64 *PTBase = NULL;
        if (!MiTranslatePhysicalToVirtual(TopLevelTableReverse, 
            (PDE & ARCH_X64_PXE_4K_BASE_MASK), (VIRTUAL_ADDRESS *)&PTBase))
        {
            return FALSE;
        }

        U64 PTE = PTBase[PTEi];

        if (!(PTE & ARCH_X64_PXE_PRESENT))
        {
            return FALSE;
        }

        *DestinationAddress = (SourceAddress & PAGE_MASK) | 
            ARCH_X64_TO_CANONICAL_ADDRESS(PTE & ARCH_X64_PXE_4K_BASE_MASK);
    }

    return TRUE;
}


/**
 * @brief Sets virtual-to-physical page mapping.\n
 *        Reverse mapping (physical-to-virtual) is also allowed.\n
 * 
 * @param [in] PML4TBase                Base address of PML4T.
 * @param [in] RPML4TBase               Base address of RPML4T.
 * @param [in] VirtualAddress           Virtual address.
 * @param [in] PhysicalAddress          Physical address.
 * @param [in] Size                     Map size.
 * @param [in] PteFlags                 PTE flags to be specified.
 * @param [in] ReverseMapping           Sets virtual-to-physical mapping to PML4T if FALSE.\n
 *                                      Otherwise, sets physical-to-virtual mapping.\n
 *                                      PML4T will be treated as reverse mapping table of PML4T.
 * @param [in] AllowNonDefaultPageSize  If TRUE, non-default page size is allowed.\n
 *                                      For page size, not only the 4K but also 2M will be used.\n
 * @param [in] PxePool                  PXE pool.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
MiArchX64SetPageMapping(
    IN U64 *PML4TBase, 
    IN U64 *RPML4TBase, 
    IN VIRTUAL_ADDRESS VirtualAddress, 
    IN PHYSICAL_ADDRESS PhysicalAddress, 
    IN SIZE_T Size, 
    IN U64 PteFlags,
    IN BOOLEAN ReverseMapping,
    IN BOOLEAN AllowNonDefaultPageSize,
    IN OBJECT_POOL *PxePool)
{
    U64 PageCount = SIZE_TO_PAGES(Size);

    U64 VfnStart = PAGE_TO_PAGE_NUMBER_4K(VirtualAddress);
    U64 PfnStart = PAGE_TO_PAGE_NUMBER_4K(PhysicalAddress);

    U64 SourcePageNumber = VfnStart;
    U64 DestinationPageNumber = PfnStart;

    U64 PxeDefaultFlags = ARCH_X64_PXE_PRESENT | ARCH_X64_PXE_USER | ARCH_X64_PXE_WRITABLE;

    if (ReverseMapping)
    {
        // Physical-to-virtual mapping.
        SourcePageNumber = PfnStart;
        DestinationPageNumber = VfnStart;

        PxeDefaultFlags = ARCH_X64_PXE_PRESENT;
    }

    U64 PageCount2M = SIZE_TO_PAGES(PAGE_SIZE_2M);

    for (U64 i = 0; i < PageCount; )
    {
        BOOLEAN UseMapping2M = FALSE;

        if (AllowNonDefaultPageSize && 
            !(SourcePageNumber & (PageCount2M - 1)) && 
            !(DestinationPageNumber & (PageCount2M - 1)) && 
            i + PageCount2M <= PageCount)
        {
            // Both source and destination are 2M-size aligned.
            UseMapping2M = TRUE;
        }

        //
        // Set address mapping.
        //

        U64 *NewTableBase = NULL;

        U64 PML4TEi = ARCH_X64_PAGE_NUMBER_TO_PML4EI(SourcePageNumber);
        U64 PDPTEi = ARCH_X64_PAGE_NUMBER_TO_PDPTEI(SourcePageNumber);
        U64 PDEi = ARCH_X64_PAGE_NUMBER_TO_PDEI(SourcePageNumber);
        U64 PTEi = ARCH_X64_PAGE_NUMBER_TO_PTEI(SourcePageNumber);

        U64 PhysicalTableBaseTemp = 0;

        U64 PML4TE = PML4TBase[PML4TEi];
        if (!(PML4TE & ARCH_X64_PXE_PRESENT))
        {
            // Allocate new PDPTEs
            NewTableBase = PoolAllocateObject(PxePool);
            if (!NewTableBase)
                return FALSE;

            DASSERT(MiTranslateVirtualToPhysical(PML4TBase, RPML4TBase, (U64)NewTableBase, &PhysicalTableBaseTemp));

            PML4TE = PxeDefaultFlags | (PhysicalTableBaseTemp & ARCH_X64_PXE_4K_BASE_MASK);
            PML4TBase[PML4TEi] = PML4TE;
        }

        // PML4T -> PDPT
        U64 *PDPTBase = NULL;
        DASSERT(MiTranslatePhysicalToVirtual(RPML4TBase, (PML4TE & ARCH_X64_PXE_4K_BASE_MASK), (U64 *)&PDPTBase));
        
        U64 PDPTE = PDPTBase[PDPTEi];
        if (!(PDPTE & ARCH_X64_PXE_PRESENT))
        {
            // Allocate new PDEs
            NewTableBase = PoolAllocateObject(PxePool);
            if (!NewTableBase)
                return FALSE;

            DASSERT(MiTranslateVirtualToPhysical(PML4TBase, RPML4TBase, (U64)NewTableBase, &PhysicalTableBaseTemp));

            PDPTE = PxeDefaultFlags | (PhysicalTableBaseTemp & ARCH_X64_PXE_4K_BASE_MASK);
            PDPTBase[PDPTEi] = PDPTE;
        }

        // PDPT -> PD
        U64 *PDBase = NULL;
        DASSERT(MiTranslatePhysicalToVirtual(RPML4TBase, (PDPTE & ARCH_X64_PXE_4K_BASE_MASK), (U64 *)&PDBase));
        
        U64 PDE = PDBase[PDEi];
        if (!(PDE & ARCH_X64_PXE_PRESENT))
        {
            if (UseMapping2M)
            {
                PDE = ARCH_X64_PXE_PRESENT | ARCH_X64_PXE_LARGE_SIZE | 
                    (PteFlags & ~ARCH_X64_PXE_2M_BASE_MASK) | 
                    ((DestinationPageNumber << PAGE_SHIFT) & ARCH_X64_PXE_2M_BASE_MASK);
            }
            else
            {
                // Allocate new PDEs
                NewTableBase = PoolAllocateObject(PxePool);
                if (!NewTableBase)
                    return FALSE;

                DASSERT(MiTranslateVirtualToPhysical(PML4TBase, RPML4TBase, (U64)NewTableBase, &PhysicalTableBaseTemp));

                PDE = PxeDefaultFlags | (PhysicalTableBaseTemp & ARCH_X64_PXE_4K_BASE_MASK);
            }

            PDBase[PDEi] = PDE;
        }

        if (UseMapping2M)
        {
            i += PageCount2M;
            SourcePageNumber += PageCount2M;
            DestinationPageNumber += PageCount2M;
        }
        else
        {
            // PD -> PT
            U64 *PTBase = NULL;
            DASSERT(MiTranslatePhysicalToVirtual(RPML4TBase, (PDE & ARCH_X64_PXE_4K_BASE_MASK), (U64 *)&PTBase));

            // Set new PTEs
            U64 PTE = ARCH_X64_PXE_PRESENT | 
                    (PteFlags & ~ARCH_X64_PXE_4K_BASE_MASK) | 
                    ((DestinationPageNumber << PAGE_SHIFT) & ARCH_X64_PXE_4K_BASE_MASK);

            PTBase[PTEi] = PTE;

            i++;
            SourcePageNumber++;
            DestinationPageNumber++;
        }
    }

    return TRUE;
}

/**
 * @brief Sets the attribute of page.\n
 * 
 * @param [in] PML4TBase                Base address of PML4T.
 * @param [in] RPML4TBase               Base address of RPML4T.
 * @param [in] VirtualAddress           Virtual address.
 * @param [in] Size                     Map size.
 * @param [in] PatFlags                 Page attribute flags. See ARCH_X64_PAT_Xxx.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
MiArchX64SetPageAttribute(
    IN U64 *PML4TBase, 
    IN U64 *RPML4TBase, 
    IN VIRTUAL_ADDRESS VirtualAddress, 
    IN SIZE_T Size, 
    IN U64 PatFlags)
{
    U64 PageCount = SIZE_TO_PAGES(Size);
    U64 VfnStart = PAGE_TO_PAGE_NUMBER_4K(VirtualAddress);
    U64 SourcePageNumber = VfnStart;

    for (U64 i = 0; i < PageCount; )
    {
        //
        // Set address mapping.
        //

        U64 PML4TEi = ARCH_X64_PAGE_NUMBER_TO_PML4EI(SourcePageNumber);
        U64 PDPTEi = ARCH_X64_PAGE_NUMBER_TO_PDPTEI(SourcePageNumber);
        U64 PDEi = ARCH_X64_PAGE_NUMBER_TO_PDEI(SourcePageNumber);
        U64 PTEi = ARCH_X64_PAGE_NUMBER_TO_PTEI(SourcePageNumber);

        U64 PML4TE = PML4TBase[PML4TEi];
        DASSERT(PML4TE & ARCH_X64_PXE_PRESENT);

        // PML4T -> PDPT
        U64 *PDPTBase = NULL;
        DASSERT(MiTranslatePhysicalToVirtual(RPML4TBase, (PML4TE & ARCH_X64_PXE_4K_BASE_MASK), (U64 *)&PDPTBase));
        
        U64 PDPTE = PDPTBase[PDPTEi];
        DASSERT(PDPTE & ARCH_X64_PXE_PRESENT);

        // PDPT -> PD
        U64 *PDBase = NULL;
        DASSERT(MiTranslatePhysicalToVirtual(RPML4TBase, (PDPTE & ARCH_X64_PXE_4K_BASE_MASK), (U64 *)&PDBase));
        
        U64 PDE = PDBase[PDEi];
        DASSERT(PDE & ARCH_X64_PXE_PRESENT);

        // Currently not supports large pages
        DASSERT(!(PDE & ARCH_X64_PXE_LARGE_SIZE));

        // PD -> PT
        U64 *PTBase = NULL;
        DASSERT(MiTranslatePhysicalToVirtual(RPML4TBase, (PDE & ARCH_X64_PXE_4K_BASE_MASK), (U64 *)&PTBase));

        BOOLEAN Present = FALSE;
        if (PTBase[PTEi] & ARCH_X64_PXE_PRESENT)
        {
            Present = TRUE;
        }

        // Set PTE flags with present bit clear.
        PTBase[PTEi] = (PTBase[PTEi] & ~(ARCH_X64_PAT_MASK_ALL_SET | ARCH_X64_PXE_PRESENT)) | 
            (PatFlags & ARCH_X64_PAT_MASK_ALL_SET);

        MiArchX64InvalidateSinglePage(SourcePageNumber << PAGE_SHIFT);

        if (Present)
        {
            PTBase[PTEi] |= ARCH_X64_PXE_PRESENT;
        }

        i++;
        SourcePageNumber++;
    }

    return TRUE;
}

