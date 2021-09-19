
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
#include <mm/mm.h>


#define IS_IN_ADDRESS_RANGE(_test_addr, _test_size, _start_addr, _size) \
    ((_start_addr) <= (_test_addr) && (_test_addr) + (_test_size) <= (_start_addr) + (_size))


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


BOOLEAN
KERNELAPI
MiPrePoolInitialize(
    IN OS_LOADER_BLOCK *LoaderBlock)
{
    PTR PreInitPoolBase = LoaderBlock->LoaderData.PreInitPoolBase
        + LoaderBlock->LoaderData.OffsetToVirtualBase;

    return MiInitializePoolBlockList(&MiPoolList[PoolTypeNonPagedPreInit],
        PreInitPoolBase, LoaderBlock->LoaderData.PreInitPoolSize, 0);
}

ESTATUS
KERNELAPI
MiPreInitialize(
    IN OS_LOADER_BLOCK *LoaderBlock)
{
    //
    // Initialize the pre-init pool.
    //

    if (!MiPrePoolInitialize(LoaderBlock))
    {
        return E_PREINIT_POOL_INIT_FAILED;
    }


    //
    // Initialize the PAD/VAD tree.
    //

    MiXadContext.UsePreInitPool = TRUE;

    if (!MmXadInitializeTree(&MiPadTree, &MiXadContext) || 
        !MmXadInitializeTree(&MiVadTree, &MiXadContext))
    {
        return E_FAILED;
    }

    //
    // Initialize address ranges.
    //

    ADDRESS PhysicalRange = {
        .Range.Start = 0,
        .Range.End = 0xfffffffffffff000ULL,
        .Type = PadInitialReserved,
    };

    ESTATUS Status = MmXadInsertAddress(&MiPadTree, NULL, &PhysicalRange);
    if (!E_IS_SUCCESS(Status))
    {
        return Status;
    }

    ADDRESS VirtualRange = {
        .Range.Start = 0,
        .Range.End = 0xfffffffffffff000ULL,
        .Type = VadInitialReserved,
    };

    Status = MmXadInsertAddress(&MiVadTree, NULL, &VirtualRange);
    if (!E_IS_SUCCESS(Status))
    {
        return Status;
    }

    //
    // Allocates memory by memory map info.
    //

    PTR OffsetToVirtualBase = LoaderBlock->LoaderData.OffsetToVirtualBase;
    EFI_MEMORY_DESCRIPTOR *Descriptor = (EFI_MEMORY_DESCRIPTOR *)((PTR)
        LoaderBlock->Memory.Map + OffsetToVirtualBase);
    U32 Count = LoaderBlock->Memory.MapCount;
    U32 DescriptorSize = LoaderBlock->Memory.DescriptorSize;
    U64 RangesBitmap = LoaderBlock->LoaderData.PreserveRangesBitmap;

    for (U32 i = 0; i < Count; i++)
    {
        PTR PhysicalAddress = Descriptor->PhysicalStart;
        SIZE_T Size = PAGES_TO_SIZE(Descriptor->NumberOfPages);
        U32 Type = Descriptor->Type;

        Status = MmAllocatePhysicalMemory2(&PhysicalAddress, Size, PadInitialReserved, Type);
        if (!E_IS_SUCCESS(Status))
        {
            return Status;
        }

        if (Type == EfiLoaderData)
        {
            for (U32 j = 0; j < OS_PRESERVE_RANGE_MAX_COUNT; j++)
            {
                OS_PRESERVE_MEMORY_RANGE *PreserveRange = &LoaderBlock->LoaderData.PreserveRanges[j];

                BOOLEAN InRange = IS_IN_ADDRESS_RANGE(
                    PreserveRange->PhysicalStart,
                    PreserveRange->Size,
                    Descriptor->PhysicalStart,
                    Size);

                if (InRange && (RangesBitmap & (1ULL << j)))
                {
                    if (OsSpecificMemTypeStart <= PreserveRange->Type &&
                        PreserveRange->Type < OsSpecificMemTypeEnd)
                    {
                        // Reallocate memory blocks for specific range.
                        PTR PhysicalAddress = PreserveRange->PhysicalStart;
                        PTR VirtualAddress = PreserveRange->VirtualStart;

                        Status = MmReallocatePhysicalMemory(&PhysicalAddress, PreserveRange->Size, 
                            Type, PreserveRange->Type);
                        if (!E_IS_SUCCESS(Status))
                        {
                            return Status;
                        }

                        Status = MmAllocateVirtualMemory2(NULL, &VirtualAddress, PreserveRange->Size, 
                            VadInitialReserved, PreserveRange->Type);
                        if (!E_IS_SUCCESS(Status))
                        {
                            return Status;
                        }

                        RangesBitmap &= ~(1ULL << j);
                    }
                }
            }
        }        

        // Move to the next descriptor.
        Descriptor = (EFI_MEMORY_DESCRIPTOR *)((PTR)Descriptor + DescriptorSize);
    }

    MiXadContext.UsePreInitPool = FALSE;
    MiXadInitialized = TRUE;

    return E_SUCCESS;
}

VOID
KERNELAPI
MiPreDumpXad(
    VOID)
{
    BootGfxPrintTextFormat("\n");

    BootGfxPrintTextFormat("Listing PAD tree...\n");
    RsBtTraverse(&MiPadTree.Tree, NULL, (PRS_BINARY_TREE_TRAVERSE)MmXadDebugTraverse);
    BootGfxPrintTextFormat("\n");

    BootGfxPrintTextFormat("Listing VAD tree...\n");
    RsBtTraverse(&MiVadTree.Tree, NULL, (PRS_BINARY_TREE_TRAVERSE)MmXadDebugTraverse);
    BootGfxPrintTextFormat("\n");
}