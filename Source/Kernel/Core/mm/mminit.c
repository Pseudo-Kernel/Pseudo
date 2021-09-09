
/**
 * @file mminit.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements memory manager initialization routines.
 * @version 0.1
 * @date 2021-09-10
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <base/base.h>
#include <init/preinit.h>
#include <init/bootgfx.h>
#include <mm/pool.h>
#include <mm/mminit.h>
#include <mm/paging.h>


UPTR MiLoaderSpaceStart;
UPTR MiLoaderSpaceEnd;
UPTR MiPadListStart;
UPTR MiPadListEnd;
UPTR MiVadListStart;
UPTR MiVadListEnd;
UPTR MiPxeAreaStart;
UPTR MiPxeAreaEnd;
UPTR MiPoolStart;
UPTR MiPoolEnd;


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


BOOLEAN
KERNELAPI
MiDiscardFirmwareMemory(
    VOID)
{
    return FALSE;
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

//    MiLoaderSpaceStart = KERNEL_VASL_START_LOADER_SPACE;
//    MiLoaderSpaceEnd = KERNEL_VASL_END_LOADER_SPACE;
//    MiPfnMetadataStart = KERNEL_VASL_START_PFN_METADATA;
//    MiPfnMetadataEnd = KERNEL_VASL_END_PFN_METADATA;
//    MiPfnListStart = KERNEL_VASL_START_PFN_LIST;
//    MiPfnListEnd = KERNEL_VASL_END_PFN_LIST;
//    MiVadListStart = KERNEL_VASL_START_VAD_LIST;
//    MiVadListEnd = KERNEL_VASL_END_VAD_LIST;
//    MiPteListStart = KERNEL_VASL_START_PTE_LIST;
//    MiPteListEnd = KERNEL_VASL_END_PTE_LIST;
//    MiSystemPoolStart = KERNEL_VASL_START_SYSTEM_POOL;
//    MiSystemPoolEnd = KERNEL_VASL_START_SYSTEM_POOL + (SystemMemoryUsable * 15) / 100; // dynamic
//    MiSystemPoolEnd &= ~(PAGE_SIZE - 1);

    DbgTraceF(TraceLevelEvent, "Usable memory %d bytes\n", SystemMemoryUsable);
//    DbgTraceF(TraceLevelEvent, "Range List (Start, End):\n");
//    DbgTraceF(TraceLevelEvent, "LoaderSpace       = 0x%p, 0x%p\n", MiLoaderSpaceStart, MiLoaderSpaceEnd);
//    DbgTraceF(TraceLevelEvent, "PfnMetadata       = 0x%p, 0x%p\n", MiPfnMetadataStart, MiPfnMetadataEnd);
//    DbgTraceF(TraceLevelEvent, "PfnListStart      = 0x%p, 0x%p\n", MiPfnListStart, MiPfnListEnd);
//    DbgTraceF(TraceLevelEvent, "VadListStart      = 0x%p, 0x%p\n", MiVadListStart, MiVadListEnd);
//    DbgTraceF(TraceLevelEvent, "PteListStart      = 0x%p, 0x%p\n", MiPteListStart, MiPteListEnd);
//    DbgTraceF(TraceLevelEvent, "SystemPoolStart   = 0x%p, 0x%p\n", MiSystemPoolStart, MiSystemPoolEnd);
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

        MiArchX64AddPageMapping(
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

            MiArchX64AddPageMapping(
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

