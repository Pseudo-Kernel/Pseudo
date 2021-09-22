
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
U64 *MiRPML4TBase; //!< Reverse PML4 table base.


U64 *
KERNELAPI
MiAllocatePxe(
    VOID)
{
    return 0;
}

/**
 * @brief Sets virtual-to-physical page mapping.\n
 *        Reverse mapping (physical-to-virtual) is also allowed.\n
 * 
 * @param [in] PML4TBase                Base address of PML4T.
 * @param [in] VirtualAddress           Virtual address.
 * @param [in] PhysicalAddress          Physical address.
 * @param [in] Size                     Map size.
 * @param [in] PteFlags                 PTE flags to be specified.
 * @param [in] ReverseMapping           Sets virtual-to-physical mapping to PML4T if FALSE.\n
 *                                      Otherwise, sets physical-to-virtual mapping.\n
 *                                      PML4T will be treated as reverse mapping table of PML4T.
 * @param [in] AllowNonDefaultPageSize  If TRUE, non-default page size is allowed.\n
 *                                      For page size, not only the 4K but also 2M will be used.\n
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
MiArchX64SetPageMapping(
    IN U64 *PML4TBase, 
    IN VIRTUAL_ADDRESS VirtualAddress, 
    IN PHYSICAL_ADDRESS PhysicalAddress, 
    IN SIZE_T Size, 
    IN U64 PteFlags,
    IN BOOLEAN ReverseMapping,
    IN BOOLEAN AllowNonDefaultPageSize)
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

        U64 PML4TE = PML4TBase[PML4TEi];
        if (!(PML4TE & ARCH_X64_PXE_PRESENT))
        {
            // Allocate new PDPTEs
            NewTableBase = (U64 *)MiAllocatePxe();
            if (!NewTableBase)
                return FALSE;

            PML4TE = PxeDefaultFlags | ((U64)NewTableBase & ARCH_X64_PXE_4K_BASE_MASK);
            PML4TBase[PML4TEi] = PML4TE;
        }

        // PML4T -> PDPT
        U64 *PDPTBase = (U64 *)(PML4TE & ARCH_X64_PXE_4K_BASE_MASK);
        U64 PDPTE = PDPTBase[PDPTEi];
        if (!(PDPTE & ARCH_X64_PXE_PRESENT))
        {
            // Allocate new PDEs
            NewTableBase = (U64 *)MiAllocatePxe();
            if (!NewTableBase)
                return FALSE;

            PDPTE = PxeDefaultFlags | ((U64)NewTableBase & ARCH_X64_PXE_4K_BASE_MASK);
            PDPTBase[PDPTEi] = PDPTE;
        }

        // PDPT -> PD
        U64 *PDBase = (U64 *)(PDPTE & ARCH_X64_PXE_4K_BASE_MASK);
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
                NewTableBase = (U64 *)MiAllocatePxe();
                if (!NewTableBase)
                    return FALSE;

                PDE = PxeDefaultFlags | ((U64)NewTableBase & ARCH_X64_PXE_4K_BASE_MASK);
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
            U64 *PTBase = (U64 *)(PDE & ARCH_X64_PXE_4K_BASE_MASK);

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
 * @brief Clears virtual-to-physical page mapping.\n
 * 
 * @param [in] PML4TBase        Base address of PML4T.
 * @param [in] VirtualAddress   Virtual address.
 * @param [in] Size             Map size.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
MiArchX64SetPageMappingNotPresent(
    IN U64 *PML4TBase, 
    IN VIRTUAL_ADDRESS VirtualAddress, 
    IN SIZE_T Size)
{
    U64 PageCount = SIZE_TO_PAGES(Size);
    U64 VfnStart = PAGE_TO_PAGE_NUMBER_4K(VirtualAddress);
    U64 SourcePageNumber = VfnStart;

    for (U64 i = 0; i < PageCount; )
    {
        U64 PML4TEi = ARCH_X64_PAGE_NUMBER_TO_PML4EI(SourcePageNumber);
        U64 PDPTEi = ARCH_X64_PAGE_NUMBER_TO_PDPTEI(SourcePageNumber);
        U64 PDEi = ARCH_X64_PAGE_NUMBER_TO_PDEI(SourcePageNumber);
        U64 PTEi = ARCH_X64_PAGE_NUMBER_TO_PTEI(SourcePageNumber);

        U64 PML4TE = PML4TBase[PML4TEi];
        if (!(PML4TE & ARCH_X64_PXE_PRESENT))
        {
            return FALSE;
        }

        // PML4T -> PDPT
        U64 *PDPTBase = (U64 *)(PML4TE & ARCH_X64_PXE_4K_BASE_MASK);
        U64 PDPTE = PDPTBase[PDPTEi];
        if (!(PDPTE & ARCH_X64_PXE_PRESENT))
        {
            return FALSE;
        }

        // PDPT -> PD
        U64 *PDBase = (U64 *)(PDPTE & ARCH_X64_PXE_4K_BASE_MASK);
        U64 PDE = PDBase[PDEi];
        if (!(PDE & ARCH_X64_PXE_PRESENT))
        {
            return FALSE;
        }

        // PD -> PT
        U64 *PTBase = (U64 *)(PDE & ARCH_X64_PXE_4K_BASE_MASK);
        U64 PTE = PTBase[PTEi];

        // Clear present bit
        PTE &= ~ARCH_X64_PXE_PRESENT;

        PTBase[PTEi] = PTE;

        i++;
        SourcePageNumber++;
    }

    return TRUE;
}


/**
 * @brief Checks whether the virtual-to-physical is exists.\n
 * 
 * @param [in] PML4TBase        Base address of PML4T.
 * @param [in] VirtualAddress   Virtual address.
 * @param [in] Size             Map size.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
MiArchX64IsPageMappingExists(
    IN U64 *PML4TBase, 
    IN EFI_VIRTUAL_ADDRESS VirtualAddress, 
    IN SIZE_T Size)
{
    U64 PageCount = SIZE_TO_PAGES(Size);
    U64 VfnStart = PAGE_TO_PAGE_NUMBER_4K(VirtualAddress);

    U64 SourcePageNumber = VfnStart;

    U64 PageCount2M = SIZE_TO_PAGES(PAGE_SIZE_2M);

    for (U64 i = 0; i < PageCount; )
    {
        U64 PML4TEi = ARCH_X64_PAGE_NUMBER_TO_PML4EI(SourcePageNumber);
        U64 PDPTEi = ARCH_X64_PAGE_NUMBER_TO_PDPTEI(SourcePageNumber);
        U64 PDEi = ARCH_X64_PAGE_NUMBER_TO_PDEI(SourcePageNumber);
        U64 PTEi = ARCH_X64_PAGE_NUMBER_TO_PTEI(SourcePageNumber);

        U64 PML4TE = PML4TBase[PML4TEi];
        if (!(PML4TE & ARCH_X64_PXE_PRESENT))
        {
            return FALSE;
        }

        // PML4T -> PDPT
        U64 *PDPTBase = (U64 *)(PML4TE & ARCH_X64_PXE_4K_BASE_MASK);
        U64 PDPTE = PDPTBase[PDPTEi];
        if (!(PDPTE & ARCH_X64_PXE_PRESENT))
        {
            return FALSE;
        }

        // PDPT -> PD
        U64 *PDBase = (U64 *)(PDPTE & ARCH_X64_PXE_4K_BASE_MASK);
        U64 PDE = PDBase[PDEi];
        if (!(PDE & ARCH_X64_PXE_PRESENT))
        {
            return FALSE;
        }

        if (PDE & ARCH_X64_PXE_LARGE_SIZE)
        {
            i += PageCount2M;
            SourcePageNumber += PageCount2M;
        }
        else
        {
            // PD -> PT
            U64 *PTBase = (U64 *)(PDE & ARCH_X64_PXE_4K_BASE_MASK);
            U64 PTE = PTBase[PTEi];

            if (!(PTE & ARCH_X64_PXE_PRESENT))
            {
                return FALSE;
            }

            i++;
            SourcePageNumber++;
        }
    }

    return TRUE;
}

