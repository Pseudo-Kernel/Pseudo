
/**
 * @file mminit.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements memory manager initialization routines.
 * @version 0.1
 * @date 2021-09-10
 * 
 * @copyright Copyright (c) 2021
 * 
 * @todo Need to implement PXE pool (including pre-init PXE pool)
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

typedef struct _POOL_ADDRESS
{
    BOOLEAN Valid;
    PHYSICAL_ADDRESSES *Addresses;
    SIZE_T PoolSize;
    PTR VirtualAddress;
} POOL_ADDRESS;

POOL_ADDRESS MiPoolAddresses[PoolTypeMaximum];

SIZE_T MiAvailableSystemMemory;

/*
UPTR MiLoaderSpaceStart;
UPTR MiLoaderSpaceEnd;
UPTR MiPadListStart;
UPTR MiPadListEnd;
UPTR MiVadListStart;
UPTR MiVadListEnd;
UPTR MiPxeAreaStart;
UPTR MiPxeAreaEnd;
*/



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
    U64 OffsetToVirtualBase = LoaderBlock->LoaderData.OffsetToVirtualBase;
    MiPML4TBase = (U64 *)((UPTR)LoaderBlock->LoaderData.PML4TBase + OffsetToVirtualBase);
    MiRPML4TBase = (U64 *)((UPTR)LoaderBlock->LoaderData.RPML4TBase + OffsetToVirtualBase);

    //
    // Initialize the pre-init pool.
    //

    if (!MiPrePoolInitialize(LoaderBlock))
    {
        return E_PREINIT_POOL_INIT_FAILED;
    }


    //
    // Initialize the pre-init PXE pool.
    //

    OBJECT_POOL MiPxePool;

    SIZE_T Pxe512EntriesSize = 512 * sizeof(U64);
    U32 PxeTableCount = LoaderBlock->LoaderData.PxeInitPoolSize / Pxe512EntriesSize;
    SIZE_T PxePoolBitmapSize = (PxeTableCount * 2) >> 3;

    U8 *InitialPxePoolBitmap = MmAllocatePool(PoolTypeNonPagedPreInit, PxePoolBitmapSize, 0x10, TAG4('I', 'N', 'I', 'T'));
    if (!InitialPxePoolBitmap)
    {
        return E_PREINIT_PXE_POOL_INIT_FAILED;
    }

    memset(InitialPxePoolBitmap, 0, PxePoolBitmapSize);
    
    if (!PoolInitialize(
        &MiPxePool, PxeTableCount, Pxe512EntriesSize, 
        InitialPxePoolBitmap, PxePoolBitmapSize, 
        (PVOID)((PTR)LoaderBlock->LoaderData.PxeInitPoolBase + OffsetToVirtualBase),
        LoaderBlock->LoaderData.PxeInitPoolSize))
    {
        return E_PREINIT_PXE_POOL_INIT_FAILED;
    }

    UINT PxeTableAllocatedCount = LoaderBlock->LoaderData.PxeInitPoolSizeUsed / Pxe512EntriesSize;
    for (UINT i = 0; i < PxeTableAllocatedCount; i++)
    {
        if (!PoolAllocateObject(&MiPxePool))
        {
            return E_PREINIT_PXE_POOL_INIT_FAILED;
        }
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

    ADDRESS InitialVaSpaces[] =
    {
        { .Range.Start = 0x0000000000000000ULL, .Range.End = 0x0000800000000000ULL, .Type = VadReserved },
        { .Range.Start = 0x0000800000000000ULL, .Range.End = 0xffff800000000000ULL, .Type = VadInaccessibleHole },
        { .Range.Start = 0xffff800000000000ULL, .Range.End = 0xfffffffffffff000ULL, .Type = VadFree },
    };

    for (U32 i = 0; i < COUNTOF(InitialVaSpaces); i++)
    {
        PTR VirtualAddress = InitialVaSpaces[i].Range.Start;
        Status = MmAllocateVirtualMemory2(NULL, &VirtualAddress, 
            InitialVaSpaces[i].Range.End - InitialVaSpaces[i].Range.Start, VadInitialReserved, 
            InitialVaSpaces[i].Type);

        if (!E_IS_SUCCESS(Status))
        {
            return Status;
        }
    }


    //
    // Allocates memory by memory map.
    //

    EFI_MEMORY_DESCRIPTOR *Descriptor = (EFI_MEMORY_DESCRIPTOR *)((PTR)
        LoaderBlock->Memory.Map + OffsetToVirtualBase);
    U32 Count = LoaderBlock->Memory.MapCount;
    U32 DescriptorSize = LoaderBlock->Memory.DescriptorSize;
    U64 RangesBitmap = LoaderBlock->LoaderData.PreserveRangesBitmap;
    SIZE_T AvailableMemory = 0;

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

        if (Type == EfiLoaderCode ||
            Type == EfiLoaderData ||
            Type == EfiBootServicesCode ||
            Type == EfiBootServicesData ||
            Type == EfiConventionalMemory ||
            Type == EfiPersistentMemory)
        {
            AvailableMemory += PAGES_TO_SIZE(Descriptor->NumberOfPages);
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

                        Status = MmAllocateVirtualMemory(NULL, &VirtualAddress, PreserveRange->Size, 
                            PreserveRange->Type);
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

    MiAvailableSystemMemory = AvailableMemory;
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

VOID
KERNELAPI
MmInitialize(
    VOID)
{
    //
    // Initialize the PXE pool.
    //



    //
    // Initialize the pool (except pre-init pool).
    //

    MiPoolAddresses[PoolTypeNonPaged] = (POOL_ADDRESS)
        { .Valid = TRUE, .PoolSize = ROUNDUP_TO_PAGE_SIZE(MiAvailableSystemMemory / 10), };

    MiPoolAddresses[PoolTypePaged] = (POOL_ADDRESS)
        { .Valid = TRUE, .PoolSize = ROUNDUP_TO_PAGE_SIZE(MiAvailableSystemMemory / 10), };

    for (U32 PoolType = 0; PoolType < COUNTOF(MiPoolAddresses); PoolType++)
    {
        if (!MiPoolAddresses[PoolType].Valid)
            continue;

        U32 AddressMaximumCount = 0x100;
        SIZE_T SizeOfAddresses = SIZEOF_PHYSICAL_ADDRESSES(AddressMaximumCount);

        PHYSICAL_ADDRESSES *Addresses = MmAllocatePool(PoolTypeNonPagedPreInit, 
            SizeOfAddresses, 0x10, POOLTAG_MMINIT);

        DbgTraceF(TraceLevelDebug, "Initializing pool (type %d)\n", PoolType);

        if (!Addresses)
        {
            FATAL("Failed to initialize pool");
        }

        memset(Addresses, 0, SizeOfAddresses);
        Addresses->PhysicalAddressMaximumCount = AddressMaximumCount;
        Addresses->PhysicalAddressCount = 0;

        PTR PoolVirtualBase = 0;
        SIZE_T PoolSize = MiPoolAddresses[PoolType].PoolSize;
        ESTATUS Status = E_SUCCESS;

        Status = MmAllocatePhysicalMemoryGather(Addresses, PoolSize, PadInUse);
        if (!E_IS_SUCCESS(Status))
        {
            FATAL("Failed to initialize pool (0x%08x)", Status);
        }

        Status = MmAllocateVirtualMemory(NULL, &PoolVirtualBase, PoolSize, VadInUse);
        if (!E_IS_SUCCESS(Status))
        {
            FATAL("Failed to initialize pool (0x%08x)", Status);
        }

        Status = MmMapMemory(Addresses, PoolVirtualBase, ARCH_X64_PXE_WRITABLE, TRUE);
        if (!E_IS_SUCCESS(Status))
        {
            FATAL("Failed to initialize pool (0x%08x)", Status);
        }

        if (!MiInitializePoolBlockList(&MiPoolList[PoolType], PoolVirtualBase, PoolSize, 0))
        {
            FATAL("Failed to initialize pool");
        }

        MiPoolAddresses[PoolType].VirtualAddress = PoolVirtualBase;
        MiPoolAddresses[PoolType].Addresses = Addresses;

        DbgTraceF(TraceLevelDebug, "Pool initialized (type %d) => 0x%016llx - 0x%016llx\n", 
            PoolType, PoolVirtualBase, PoolVirtualBase + PoolSize - 1);
    }

    DbgTraceF(TraceLevelDebug, "Leave\n");

    // Now it is safe to use other pools.
    MiXadContext.UsePreInitPool = FALSE;
}
