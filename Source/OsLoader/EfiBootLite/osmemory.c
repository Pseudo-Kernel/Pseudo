
/**
 * @file osmemory.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements memory related routines.
 * @version 0.1
 * @date 2019-09-10
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "OsLoader.h"
#include "osmisc.h"
#include "osdebug.h"
#include "osmemory.h"


/**
 * @brief Allocates physical pages below 4GB.
 * 
 * @param [in] Size                 Size to allocate.
 * @param [in,out] Address          Pointer to caller-supplied variable which specifies preferred address.\n
 * @param [in] AddressSpecified     If AddressSpecified != FALSE, OslAllocatePages tries to allocate page at preferred address.\n
 *                                  if AddressSpecified == FALSE, OslAllocatePages determines address automatically.\n
 *                                  Allocated address will be copied to *Address. Zero means allocation failure.
 * @param [in] MemoryType           Type of memory. If zero is specified, EfiLoaderData will be used.
 * 
 * @return EFI_SUCCESS  The operation is completed successfully.
 * @return else         An error occurred during the operation.
 */
EFI_STATUS
EFIAPI
OslAllocatePages(
    IN UINTN Size, 
    IN OUT EFI_PHYSICAL_ADDRESS *Address, 
    IN BOOLEAN AddressSpecified,
    IN OS_MEMORY_TYPE MemoryType)
{
    UINT32 AllocateMemoryType = MemoryType;

    if (!AllocateMemoryType)
    {
        AllocateMemoryType = EfiLoaderData;
    }

    EFI_PHYSICAL_ADDRESS TargetAddress = 0xffff0000ULL;
    EFI_ALLOCATE_TYPE AllocateType = AllocateMaxAddress;
    
    if (AddressSpecified)
    {
        TargetAddress = *Address;
        AllocateType = AllocateAddress;
    }
    
    UINTN PageCount = EFI_SIZE_TO_PAGES(Size);
    EFI_STATUS Status = gBS->AllocatePages(AllocateType, AllocateMemoryType, PageCount, &TargetAddress);

    if (Status == EFI_SUCCESS)
    {
        // Zeroes the memory.
        gBS->SetMem((void *)TargetAddress, PageCount << EFI_PAGE_SHIFT, 0);
        *Address = TargetAddress;
    }
    else
    {
        *Address = 0;
    }

    return Status;
}


/**
 * @brief Frees the page.
 * 
 * @param [in] Address      4K-aligned address which we want to free.
 * @param [in] Size         Size to free.
 * 
 * @return EFI_SUCCESS      The operation is completed successfully.
 * @return else             An error occurred during the operation.
 */
EFI_STATUS
EFIAPI
OslFreePages(
    IN EFI_PHYSICAL_ADDRESS Address, 
    IN UINTN Size)
{
    return gBS->FreePages(Address, EFI_SIZE_TO_PAGES(Size));
}

/**
 * @brief Frees the memory map.
 * 
 * @param [in] LoaderBlock  The loader block which contains OS loader information.
 * 
 * @return None.
 */
VOID
EFIAPI
OslFreeMemoryMap(
	IN OS_LOADER_BLOCK *LoaderBlock)
{
	VOID *Map = (VOID *)LoaderBlock->Memory.Map;

    OslFreePages((EFI_PHYSICAL_ADDRESS)Map, LoaderBlock->Memory.MapSize);

	LoaderBlock->Memory.Map = NULL;
	LoaderBlock->Memory.MapKey = 0;
	LoaderBlock->Memory.MapCount = 0;
	LoaderBlock->Memory.DescriptorSize = 0;
	LoaderBlock->Memory.DescriptorVersion = 0;
}

/**
 * @brief Allocates and querys the memory map.
 * 
 * @param [in] LoaderBlock  The loader block which contains OS loader information.
 * 
 * @return EFI_SUCCESS      The operation is completed successfully.
 * @return else             An error occurred during the operation.
 */
EFI_STATUS
EFIAPI
OslQueryMemoryMap(
	IN OS_LOADER_BLOCK *LoaderBlock)
{
	EFI_MEMORY_DESCRIPTOR *Map;
	UINTN MapSize;
	UINTN MapKey;
	UINTN DescriptorSize;
	UINT32 DescriptorVersion;
	EFI_STATUS Status;
	UINTN TryCount;

	MapSize = 0;
	gBS->GetMemoryMap(&MapSize, NULL, &MapKey, &DescriptorSize, &DescriptorVersion);

	for (TryCount = 0; TryCount < 0x10; TryCount++)
	{
		Status = OslAllocatePages(MapSize, (EFI_PHYSICAL_ADDRESS *)&Map, FALSE, OsLoaderData);
		if (Status != EFI_SUCCESS)
			return Status;

		Status = gBS->GetMemoryMap(&MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
		if (Status == EFI_SUCCESS)
			break;

        OslFreePages((EFI_PHYSICAL_ADDRESS)Map, MapSize);
	}

	if (Status != EFI_SUCCESS)
	{
		TRACEF(L"Failed to query the memory map, Status 0x%08x\r\n", Status);
		return Status;
	}

	LoaderBlock->Memory.Map = Map;
	LoaderBlock->Memory.MapKey = MapKey;
	LoaderBlock->Memory.MapCount = MapSize / DescriptorSize;
	LoaderBlock->Memory.DescriptorSize = DescriptorSize;
    LoaderBlock->Memory.MapSize = MapSize;
	LoaderBlock->Memory.DescriptorVersion = DescriptorVersion;

	return EFI_SUCCESS;
}


/**
 * @brief Prints the memory map.
 * 
 * @param [in] Map              Pointer to memory descriptor list.
 * @param [in] MemoryMapSize    Size of memory descriptors.
 * @param [in] DescriptorSize   Size of memory descriptor entry.
 * 
 * @return None. 
 */
VOID
EFIAPI
OslDumpMemoryMap(
	IN EFI_MEMORY_DESCRIPTOR *Map, 
	IN UINTN MemoryMapSize, 
	IN UINTN DescriptorSize)
{
	UINTN MapCount;
	UINTN i;

	if (!IS_DEBUG_MODE(&OslLoaderBlock))
	{
		return;
	}

	MapCount = MemoryMapSize / DescriptorSize;

	DTRACE(
		&OslLoaderBlock, 
		L"\r\n"
		L"Firmware Memory Map (MapCount = %d)\r\n"
		L"  i         AddressStart          AddressEnd                        Type\r\n",
		MapCount);

	for (i = 0; i < MapCount; i++)
	{
		CHAR16 TypeString[26 + 1] = { 0 };
		UINTN Length;
		EFI_MEMORY_DESCRIPTOR *MapEntry = (EFI_MEMORY_DESCRIPTOR *)((INT8 *)Map + i * DescriptorSize);

		#define	CASE_COPY_STRING(_copy_to, _val_name)	\
			case _val_name:								\
				Length = StrFormat(TypeString, ARRAY_SIZE(TypeString), L"%s", ASCII(_val_name));\
				StrTerminate(TypeString, ARRAY_SIZE(TypeString), Length);\
				break;

		switch (MapEntry->Type)
		{
		CASE_COPY_STRING(TypeString, EfiReservedMemoryType)
		CASE_COPY_STRING(TypeString, EfiLoaderCode)
		CASE_COPY_STRING(TypeString, EfiLoaderData)
		CASE_COPY_STRING(TypeString, EfiBootServicesCode)
		CASE_COPY_STRING(TypeString, EfiBootServicesData)
		CASE_COPY_STRING(TypeString, EfiRuntimeServicesCode)
		CASE_COPY_STRING(TypeString, EfiRuntimeServicesData)
		CASE_COPY_STRING(TypeString, EfiConventionalMemory)
		CASE_COPY_STRING(TypeString, EfiUnusableMemory)
		CASE_COPY_STRING(TypeString, EfiACPIReclaimMemory)
		CASE_COPY_STRING(TypeString, EfiACPIMemoryNVS)
		CASE_COPY_STRING(TypeString, EfiMemoryMappedIO)
		CASE_COPY_STRING(TypeString, EfiMemoryMappedIOPortSpace)
		CASE_COPY_STRING(TypeString, EfiPalCode)
		CASE_COPY_STRING(TypeString, EfiPersistentMemory)
		CASE_COPY_STRING(TypeString, EfiMaxMemoryType)
		default:
			Length = StrFormat(TypeString, ARRAY_SIZE(TypeString) - 1, L"UnknownType (0x%08X)", MapEntry->Type);
			StrTerminate(TypeString, ARRAY_SIZE(TypeString), Length);
//			AsciiSPrint(TypeString, ARRAY_SIZE(TypeString), "UnknownType (0x%08X)", MapEntry->Type);
			break;
		}

		DTRACE(
			&OslLoaderBlock, 
			L" %2d:  0x%016lX  0x%016lX  %26ls\r\n",
			i,
			MapEntry->PhysicalStart,
			MapEntry->PhysicalStart + (MapEntry->NumberOfPages << EFI_PAGE_SHIFT) - 1,
			TypeString);

		if (!((i + 1) % 20))
		{
			OslDbgWaitEnterKey(&OslLoaderBlock, NULL);
		}

		#undef CASE_COPY_STRING
	}

	DTRACE(&OslLoaderBlock, L"End of Memory Map.\r\n");
	OslDbgWaitEnterKey(&OslLoaderBlock, NULL);

	TRACE(L"\r\n");
}

/**
 * @brief Allocates the 512-entry of PXE.
 * 
 * @param [in] LoaderBlock      Loader block.
 * 
 * @return Non-zero PXE base address if succeeds, zero otherwise.
 */
static
EFI_PHYSICAL_ADDRESS
EFIAPI
OslAllocatePxe(
    IN OS_LOADER_BLOCK *LoaderBlock)
{
    UINTN PxeSize = sizeof(UINT64) * 512;

    if (LoaderBlock->LoaderData.PxeInitPoolSizeUsed + PxeSize > LoaderBlock->LoaderData.PxeInitPoolSize)
        return 0;

    EFI_PHYSICAL_ADDRESS Allocated = 
        LoaderBlock->LoaderData.PxeInitPoolBase + LoaderBlock->LoaderData.PxeInitPoolSizeUsed;

    LoaderBlock->LoaderData.PxeInitPoolSizeUsed += PxeSize;

    return Allocated;
}

/**
 * @brief Resets the PXE pool.
 * 
 * @param [in] LoaderBlock      Loader block.
 * 
 * @return None.
 */
static
VOID
EFIAPI
OslResetPxePool(
    IN OS_LOADER_BLOCK *LoaderBlock)
{
    LoaderBlock->LoaderData.PxeInitPoolSizeUsed = 0;
}

/**
 * @brief Sets virtual-to-physical page mapping.\n
 *        Reverse mapping (physical-to-virtual) is also allowed.\n
 * 
 * @param [in] PML4TBase        Base address of PML4T.
 * @param [in] VirtualAddress   Virtual address.
 * @param [in] PhysicalAddress  Physical address.
 * @param [in] Size             Map size.
 * @param [in] PteFlags         PTE flags to be specified.
 * @param [in] ReverseMapping   Sets virtual-to-physical mapping to PML4T if FALSE.\n
 *                              Otherwise, sets physical-to-virtual mapping.\n
 *                              PML4T will be treated as reverse mapping table of PML4T.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
EFIAPI
OslArchX64SetPageMapping(
    IN UINT64 *PML4TBase, 
    IN EFI_VIRTUAL_ADDRESS VirtualAddress, 
    IN EFI_PHYSICAL_ADDRESS PhysicalAddress, 
    IN UINT64 Size, 
    IN UINT64 PteFlags,
    IN BOOLEAN ReverseMapping)
{
    UINT64 PageCount = EFI_SIZE_TO_PAGES(Size);

    UINT64 VfnStart = ARCH_X64_VA_TO_4K_VFN(VirtualAddress);
    UINT64 PfnStart = ARCH_X64_PA_TO_4K_PFN(PhysicalAddress);

    UINT64 SourcePageNumber = VfnStart;
    UINT64 DestinationPageNumber = PfnStart;

    UINT64 PxeDefaultFlags = ARCH_X64_PXE_PRESENT | ARCH_X64_PXE_USER | ARCH_X64_PXE_WRITABLE;

    if (ReverseMapping)
    {
        // Physical-to-virtual mapping.
        SourcePageNumber = PfnStart;
        DestinationPageNumber = VfnStart;

        PxeDefaultFlags = ARCH_X64_PXE_PRESENT;
    }

    UINT64 PageCount2M = EFI_SIZE_TO_PAGES(0x200000);

    for (UINT64 i = 0; i < PageCount; )
    {
        BOOLEAN UseMapping2M = FALSE;

        if (!(SourcePageNumber & (PageCount2M - 1)) && 
            !(DestinationPageNumber & (PageCount2M - 1)) && 
            i + PageCount2M <= PageCount)
        {
            // Both source and destination are 2M-size aligned.
            UseMapping2M = TRUE;
        }

        //
        // Set address mapping.
        //

        UINT64 *NewTableBase = NULL;

        UINT64 PML4TEi = ARCH_X64_VFN_TO_PML4EI(SourcePageNumber);
        UINT64 PDPTEi = ARCH_X64_VFN_TO_PDPTEI(SourcePageNumber);
        UINT64 PDEi = ARCH_X64_VFN_TO_PDEI(SourcePageNumber);
        UINT64 PTEi = ARCH_X64_VFN_TO_PTEI(SourcePageNumber);

        UINT64 PML4TE = PML4TBase[PML4TEi];
        if (!(PML4TE & ARCH_X64_PXE_PRESENT))
        {
            // Allocate new PDPTEs
            NewTableBase = (UINT64 *)OslAllocatePxe(&OslLoaderBlock);
            if (!NewTableBase)
                return FALSE;

            PML4TE = PxeDefaultFlags | ((UINT64)NewTableBase & ARCH_X64_PXE_4K_BASE_MASK);
            PML4TBase[PML4TEi] = PML4TE;
        }

        // PML4T -> PDPT
        UINT64 *PDPTBase = (UINT64 *)(PML4TE & ARCH_X64_PXE_4K_BASE_MASK);
        UINT64 PDPTE = PDPTBase[PDPTEi];
        if (!(PDPTE & ARCH_X64_PXE_PRESENT))
        {
            // Allocate new PDEs
            NewTableBase = (UINT64 *)OslAllocatePxe(&OslLoaderBlock);
            if (!NewTableBase)
                return FALSE;

            PDPTE = PxeDefaultFlags | ((UINT64)NewTableBase & ARCH_X64_PXE_4K_BASE_MASK);
            PDPTBase[PDPTEi] = PDPTE;
        }

        // PDPT -> PD
        UINT64 *PDBase = (UINT64 *)(PDPTE & ARCH_X64_PXE_4K_BASE_MASK);
        UINT64 PDE = PDBase[PDEi];
        if (!(PDE & ARCH_X64_PXE_PRESENT))
        {
            if (UseMapping2M)
            {
                PDE = ARCH_X64_PXE_PRESENT | ARCH_X64_PXE_LARGE_SIZE | 
                    (PteFlags & ~ARCH_X64_PXE_2M_BASE_MASK) | 
                    ((DestinationPageNumber << EFI_PAGE_SHIFT) & ARCH_X64_PXE_2M_BASE_MASK);
            }
            else
            {
                // Allocate new PDEs
                NewTableBase = (UINT64 *)OslAllocatePxe(&OslLoaderBlock);
                if (!NewTableBase)
                    return FALSE;

                PDE = PxeDefaultFlags | ((UINT64)NewTableBase & ARCH_X64_PXE_4K_BASE_MASK);
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
            UINT64 *PTBase = (UINT64 *)(PDE & ARCH_X64_PXE_4K_BASE_MASK);

            // Set new PTEs
            UINT64 PTE = ARCH_X64_PXE_PRESENT | 
                    (PteFlags & ~ARCH_X64_PXE_4K_BASE_MASK) | 
                    ((DestinationPageNumber << EFI_PAGE_SHIFT) & ARCH_X64_PXE_4K_BASE_MASK);

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
EFIAPI
OslArchX64SetPageMappingNotPresent(
    IN UINT64 *PML4TBase, 
    IN EFI_VIRTUAL_ADDRESS VirtualAddress, 
    IN UINT64 Size)
{
    UINT64 PageCount = EFI_SIZE_TO_PAGES(Size);
    UINT64 VfnStart = ARCH_X64_VA_TO_4K_VFN(VirtualAddress);
    UINT64 SourcePageNumber = VfnStart;

    for (UINT64 i = 0; i < PageCount; )
    {
        UINT64 PML4TEi = ARCH_X64_VFN_TO_PML4EI(SourcePageNumber);
        UINT64 PDPTEi = ARCH_X64_VFN_TO_PDPTEI(SourcePageNumber);
        UINT64 PDEi = ARCH_X64_VFN_TO_PDEI(SourcePageNumber);
        UINT64 PTEi = ARCH_X64_VFN_TO_PTEI(SourcePageNumber);

        UINT64 PML4TE = PML4TBase[PML4TEi];
        if (!(PML4TE & ARCH_X64_PXE_PRESENT))
        {
            return FALSE;
        }

        // PML4T -> PDPT
        UINT64 *PDPTBase = (UINT64 *)(PML4TE & ARCH_X64_PXE_4K_BASE_MASK);
        UINT64 PDPTE = PDPTBase[PDPTEi];
        if (!(PDPTE & ARCH_X64_PXE_PRESENT))
        {
            return FALSE;
        }

        // PDPT -> PD
        UINT64 *PDBase = (UINT64 *)(PDPTE & ARCH_X64_PXE_4K_BASE_MASK);
        UINT64 PDE = PDBase[PDEi];
        if (!(PDE & ARCH_X64_PXE_PRESENT))
        {
            return FALSE;
        }

        // PD -> PT
        UINT64 *PTBase = (UINT64 *)(PDE & ARCH_X64_PXE_4K_BASE_MASK);
        UINT64 PTE = PTBase[PTEi];

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
EFIAPI
OslArchX64IsPageMappingExists(
    IN UINT64 *PML4TBase, 
    IN EFI_VIRTUAL_ADDRESS VirtualAddress, 
    IN UINT64 Size)
{
    UINT64 PageCount = EFI_SIZE_TO_PAGES(Size);
    UINT64 VfnStart = ARCH_X64_VA_TO_4K_VFN(VirtualAddress);

    UINT64 SourcePageNumber = VfnStart;

    UINT64 PageCount2M = EFI_SIZE_TO_PAGES(0x200000);

    for (UINT64 i = 0; i < PageCount; )
    {
        UINT64 PML4TEi = ARCH_X64_VFN_TO_PML4EI(SourcePageNumber);
        UINT64 PDPTEi = ARCH_X64_VFN_TO_PDPTEI(SourcePageNumber);
        UINT64 PDEi = ARCH_X64_VFN_TO_PDEI(SourcePageNumber);
        UINT64 PTEi = ARCH_X64_VFN_TO_PTEI(SourcePageNumber);

        UINT64 PML4TE = PML4TBase[PML4TEi];
        if (!(PML4TE & ARCH_X64_PXE_PRESENT))
        {
            return FALSE;
        }

        // PML4T -> PDPT
        UINT64 *PDPTBase = (UINT64 *)(PML4TE & ARCH_X64_PXE_4K_BASE_MASK);
        UINT64 PDPTE = PDPTBase[PDPTEi];
        if (!(PDPTE & ARCH_X64_PXE_PRESENT))
        {
            return FALSE;
        }

        // PDPT -> PD
        UINT64 *PDBase = (UINT64 *)(PDPTE & ARCH_X64_PXE_4K_BASE_MASK);
        UINT64 PDE = PDBase[PDEi];
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
            UINT64 *PTBase = (UINT64 *)(PDE & ARCH_X64_PXE_4K_BASE_MASK);
            UINT64 PTE = PTBase[PTEi];

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

/**
 * @brief Converts EFI_MEMORY_DESCRIPTOR.Attribute to PXE flag.
 * 
 * @param [in] Attribute    Attribute
 * 
 * @return PXE flag. 
 */
UINT64
EFIAPI
OslArchX64EfiMapAttributeToPxeFlag(
    IN UINT64 Attribute)
{
    UINT64 PxeFlag = 0;

    if (Attribute & EFI_MEMORY_UC)
        PxeFlag |= ARCH_X64_PXE_CACHE_DISABLED;

    if (Attribute & EFI_MEMORY_WC) // Write-Coalescing
        ;

    if (Attribute & EFI_MEMORY_WT) // Write-Through
        PxeFlag |= ARCH_X64_PXE_WRITE_THROUGH;

    if (Attribute & EFI_MEMORY_WB) // Write-Back
        ;

    if (Attribute & EFI_MEMORY_UCE) // Uncached??
        ;

    if (!(Attribute & EFI_MEMORY_WP)) // Write-Protected
        PxeFlag |= ARCH_X64_PXE_WRITABLE;

    if (Attribute & EFI_MEMORY_RP) // Read-Protected
        ;

    if (Attribute & EFI_MEMORY_XP) // Execute-Protected
        PxeFlag |= ARCH_X64_PXE_EXECUTE_DISABLED;

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

/**
 * @brief Reads CR3 register.
 * 
 * @return Value of CR3 register.
 */
__attribute__((naked))
UINT64
EFIAPI
OslArchX64GetCr3(
    VOID)
{
    __asm__ __volatile__
    (
        "mov rax, cr3\n\t"
        "ret\n\t"
        :
        :
        :
    );
}

/**
 * @brief Writes to CR3 register.\n
 *        This will invalidate the TLB.
 * 
 * @param [in] Cr3  New value of CR3.
 * 
 * @return None.
 */
__attribute__((naked))
VOID
EFIAPI
OslArchX64SetCr3(
    IN UINT64 Cr3)
{
    __asm__ __volatile__
    (
        "mov rax, %0\n\t"
        "mov cr3, rax\n\t"
        "ret\n\t"
        :
        : "r"(Cr3)
        :
    );
}

/**
 * @brief Sets up the paging.
 * 
 * @param [in] LoaderBlock      Loader block.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
EFIAPI
OslSetupPaging(
	IN OS_LOADER_BLOCK *LoaderBlock)
{
#if 1
    UINTN MemoryMapSize = LoaderBlock->Memory.MapSize;
    UINTN DescriptorSize = LoaderBlock->Memory.DescriptorSize;
    UINTN DescriptorVersion = LoaderBlock->Memory.DescriptorVersion;
	UINTN MapCount = MemoryMapSize / DescriptorSize;

    EFI_MEMORY_DESCRIPTOR *Map = LoaderBlock->Memory.Map;
#else
    #define MEM_DESC(_ps, _pe, _vs, _ty)          { (_ty), (_ps), (_vs), EFI_SIZE_TO_PAGES((_pe) - (_ps) + 1), 1, }
    #define MEM_DESC_0(_ps, _pe, _ty)             MEM_DESC((_ps), (_pe), (_ps), (_ty))
    static EFI_MEMORY_DESCRIPTOR PseudoMapList[] = 
    {
        MEM_DESC_0(0x0, 0x9dfff, 0),
        MEM_DESC_0(0x9e000, 0x9efff, 1),
        MEM_DESC_0(0x9f000, 0x9ffff, 2),
        MEM_DESC_0(0x100000, 0x1fffff, 3),
        MEM_DESC_0(0x200000, 0x23efff, 4),
        MEM_DESC_0(0x23f000, 0x5939ffff, 5),
        MEM_DESC_0(0x593a0000, 0x593dffff, 6),
        MEM_DESC_0(0x593e0000, 0x6571dfff, 7),
        MEM_DESC_0(0x6571e000, 0x65788fff, 8),
        MEM_DESC_0(0x65789000, 0x6578ffff, 9), 
        MEM_DESC_0(0x65790000, 0x657effff, 10),
        MEM_DESC_0(0x657f0000, 0x667effff, 11),
        MEM_DESC_0(0x667f0000, 0x668effff, 12),
        MEM_DESC_0(0x668f0000, 0x669effff, 13),
        MEM_DESC_0(0x669f0000, 0x671effff, 14),
        MEM_DESC_0(0x671f0000, 0x69286fff, 15),
        MEM_DESC_0(0x69287000, 0x69287fff, 16),
        MEM_DESC_0(0x69288000, 0x69288fff, 17),
        MEM_DESC_0(0x69289000, 0x6938ffff, 18),
        MEM_DESC_0(0x69390000, 0x6939cfff, 19),
        MEM_DESC_0(0x6939d000, 0x6939efff, 20),
        MEM_DESC_0(0x6939f000, 0x77c98fff, 21),
        MEM_DESC_0(0x77c99000, 0x78467fff, 22),
        MEM_DESC_0(0x78468000, 0x78cf3fff, 23),
        MEM_DESC_0(0x78cf4000, 0x7a46bfff, 24),
        MEM_DESC_0(0x7a46c000, 0x7a4e8fff, 25),
        MEM_DESC_0(0x7a4e9000, 0x7ae5dfff, 26),
        MEM_DESC_0(0x7ae5e000, 0x7b51afff, 27),
        MEM_DESC_0(0x7b51b000, 0x7b5fdfff, 28),
        MEM_DESC_0(0x7b5fe000, 0x7b5fefff, 29), // !!
        MEM_DESC_0(0x100000000, 0x87e7fffff, 30),
        MEM_DESC_0(0xa0000, 0xfffff, 31),
        MEM_DESC_0(0x7b5ff000, 0x7b5fffff, 32),
        MEM_DESC_0(0x7b600000, 0x7f7fffff, 33),
        MEM_DESC_0(0xf0000000, 0xf7ffffff, 34),
        MEM_DESC_0(0xfe000000, 0xfe010fff, 35),
        MEM_DESC_0(0xfec00000, 0xfec00fff, 36),
        MEM_DESC_0(0xfee00000, 0xfee00fff, 37),
        MEM_DESC_0(0xff000000, 0xffffffff, 38),
    };

    UINTN DescriptorSize = sizeof(EFI_MEMORY_DESCRIPTOR);
    UINTN MemoryMapSize = sizeof(PseudoMapList);
    UINTN MapCount = MemoryMapSize / DescriptorSize;

    EFI_MEMORY_DESCRIPTOR *Map = &PseudoMapList;
#endif

    OslResetPxePool(LoaderBlock);

    EFI_PHYSICAL_ADDRESS PML4TBase = OslAllocatePxe(LoaderBlock);
    EFI_PHYSICAL_ADDRESS RPML4TBase = OslAllocatePxe(LoaderBlock);

    if (!PML4TBase || !RPML4TBase)
        return FALSE;

	for (UINTN i = 0; i < MapCount; i++)
	{
		EFI_MEMORY_DESCRIPTOR *MapEntry = (EFI_MEMORY_DESCRIPTOR *)((INT8 *)Map + i * DescriptorSize);

        UINT64 PxeFlag = OslArchX64EfiMapAttributeToPxeFlag(MapEntry->Attribute);

        EFI_VIRTUAL_ADDRESS PhysicalAddress = MapEntry->PhysicalStart;
        EFI_VIRTUAL_ADDRESS VirtualAddress = MapEntry->PhysicalStart; // identity mapping for default
        UINTN Size = EFI_PAGES_TO_SIZE(MapEntry->NumberOfPages);

        if (MapEntry->Type == OsLoaderData || 
            MapEntry->Type == OsLowMemory1M || 
            MapEntry->Type == OsKernelImage || 
            MapEntry->Type == OsKernelStack || 
            MapEntry->Type == OsBootImage || 
            MapEntry->Type == OsPreInitPool)
        {
            VirtualAddress += LoaderBlock->LoaderData.OffsetToVirtualBase;

            // 1:1 Virtual-to-physical mapping.
            if (!OslArchX64SetPageMapping((UINT64 *)PML4TBase, VirtualAddress, PhysicalAddress, 
                Size, PxeFlag, FALSE))
            {
                TRACEF(L"Failed to set mapping, PXE pool %lld / %lld\r\n", 
                    LoaderBlock->LoaderData.PxeInitPoolSizeUsed,
                    LoaderBlock->LoaderData.PxeInitPoolSize);

                return FALSE;
            }

            // 1:1 Reverse mapping (physical-to-virtual).
            if (!OslArchX64SetPageMapping((UINT64 *)RPML4TBase, VirtualAddress, PhysicalAddress, 
                Size, 0, TRUE))
            {
                return FALSE;
            }
        }
        else
        {
            // Virtual-to-physical mapping (identity mapping).
            if (!OslArchX64SetPageMapping((UINT64 *)PML4TBase, VirtualAddress, PhysicalAddress, 
                Size, PxeFlag, FALSE))
            {
                return FALSE;
            }
        }
	}

    //
    // Map video framebuffer.
    //

    UINT64 FramebufferBase = LoaderBlock->LoaderData.VideoFramebufferBase;
    UINTN FramebufferSize = LoaderBlock->LoaderData.VideoFramebufferSize;

    if (!OslArchX64IsPageMappingExists((UINT64 *)PML4TBase, 
        FramebufferBase, FramebufferSize))
    {
        // Set framebuffer mapping (identity mapping).
        if (!OslArchX64SetPageMapping((UINT64 *)PML4TBase, FramebufferBase, FramebufferBase, 
            FramebufferSize, ARCH_X64_PXE_WRITABLE | ARCH_X64_PXE_CACHE_DISABLED, FALSE))
        {
            return FALSE;
        }
    }

    //
    // Prevent null page from getting accessed.
    //

    if (!OslArchX64SetPageMappingNotPresent((UINT64 *)PML4TBase, 0, EFI_PAGE_SIZE))
    {
        return FALSE;
    }

    LoaderBlock->LoaderData.PML4TBase = PML4TBase;
    LoaderBlock->LoaderData.RPML4TBase = RPML4TBase;

    return TRUE;
}
