
/**
 * @file mm.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements memory management functions.
 * @version 0.1
 * @date 2021-09-10
 * 
 * @copyright Copyright (c) 2021
 * 
 * @todo Acquire lock when accessing XAD structure.
 * 
 */

#include <mm/mm.h>
#include <mm/pool.h>
#include <mm/paging.h>
#include <init/bootgfx.h>

MMXAD_TREE MiPadTree; //!< Physical address tree.
MMXAD_TREE MiVadTree; //!< Virtual address tree for shared kernel space.
BOOLEAN MiXadInitialized = FALSE;

XAD_CONTEXT MiXadContext;




/**
 * @brief Reallocates virtual memory for given address.
 * 
 * @param [in] ReservedZero Reserved for future use. Currently zero.
 * @param [in,out] Address  Pointer to caller-supplied variable which contains
 *                          desired address to allocate. if *Address == zero,
 *                          address will be determined automatically.
 * @param [in] Size         Allocation size.
 * @param [in] SourceType   Address type to lookup free memory.
 * @param [in] Type         New address type after allocation.
 * 
 * @return ESTATUS code.
 */
ESTATUS
KERNELAPI
MmReallocateVirtualMemory(
    IN PVOID ReservedZero,
    IN OUT PTR *Address,
    IN SIZE_T Size,
    IN VAD_TYPE SourceType,
    IN VAD_TYPE Type)
{
    if (!Address)
    {
        return E_INVALID_PARAMETER;
    }

    if (SourceType == Type)
    {
        return E_INVALID_PARAMETER;
    }

    PTR CapturedAddressHint = *Address;
    MMXAD *XadTemp = NULL;
    SIZE_T RoundupSize = ROUNDUP_TO_PAGE_SIZE(Size);
    U32 Options = XAD_LAF_SIZE | XAD_LAF_TYPE;

    if (CapturedAddressHint)
    {
        CapturedAddressHint = ROUNDDOWN_TO_PAGE_SIZE(CapturedAddressHint);
        Options |= XAD_LAF_ADDRESS;
    }

    // Acquire XAD lock.
    MmXadAcquireLock(&MiVadTree);

    ESTATUS Status = MmXadLookupAddress(&MiVadTree, &XadTemp, CapturedAddressHint, RoundupSize, SourceType, Options);

    if (!E_IS_SUCCESS(Status))
    {
        MmXadReleaseLock(&MiVadTree);
        return Status;
    }

    U64 AddressStart = CapturedAddressHint ? CapturedAddressHint : XadTemp->Address.Range.Start;

    ADDRESS AddressReclaim = 
    {
        .Range.Start = AddressStart,
        .Range.End = AddressStart + RoundupSize,
        .Type = Type,
    };

    Status = MmXadReclaimAddress(&MiVadTree, XadTemp, NULL, &AddressReclaim);
    if (!E_IS_SUCCESS(Status))
    {
        MmXadReleaseLock(&MiVadTree);
        return Status;
    }

    if (SourceType == VadInitialReserved)
    {
        // Override previous type.
        XadTemp->PrevType = VadFree;
    }

    MmXadReleaseLock(&MiVadTree);

    *Address = AddressStart;

    return E_SUCCESS;
}

/**
 * @brief Allocates virtual memory for given address.
 * 
 * @param [in] ReservedZero Reserved for future use. Currently zero.
 * @param [in,out] Address  Pointer to caller-supplied variable which contains
 *                          desired address to allocate. if *Address == zero,
 *                          address will be determined automatically.
 * @param [in] Size         Allocation size.
 * @param [in] SourceType   Address type to lookup free memory.
 * @param [in] Type         New address type after allocation.
 * 
 * @return ESTATUS code.
 */
ESTATUS
KERNELAPI
MmAllocateVirtualMemory2(
    IN PVOID ReservedZero,
    IN OUT PTR *Address,
    IN SIZE_T Size,
    IN VAD_TYPE SourceType,
    IN VAD_TYPE Type)
{
    if (!Address)
    {
        return E_INVALID_PARAMETER;
    }

    if (SourceType == VadInitialReserved && MiXadInitialized)
    {
        // Cannot allocate reserved area because XAD is already initialized
        return E_INVALID_PARAMETER;
    }

    if (SourceType != VadFree && SourceType != VadInitialReserved)
    {
        return E_INVALID_PARAMETER;
    }

    if (SourceType == Type)
    {
        return E_INVALID_PARAMETER;
    }

    return MmReallocateVirtualMemory(ReservedZero, Address, Size, SourceType, Type);
}

/**
 * @brief Allocates virtual memory for given address.
 * 
 * @param [in] ReservedZero Reserved for future use. Currently zero.
 * @param [in,out] Address  Pointer to caller-supplied variable which contains
 *                          desired address to allocate. if *Address == zero,
 *                          address will be determined automatically.
 * @param [in] Size         Allocation size.
 * @param [in] Type         New address type after allocation.
 *
 * @return ESTATUS code.
 */
KEXPORT
ESTATUS
KERNELAPI
MmAllocateVirtualMemory(
    IN PVOID ReservedZero,
    IN OUT PTR *Address,
    IN SIZE_T Size,
    IN VAD_TYPE Type)
{
    return MmAllocateVirtualMemory2(ReservedZero, Address, Size, VadFree, Type);
}

/**
 * @brief Frees virtual memory.
 * 
 * @param [in] Address      Address to free.
 * @param [in] Size         Size to free.
 *
 * @return ESTATUS code.
 */
KEXPORT
ESTATUS
KERNELAPI
MmFreeVirtualMemory(
    IN PTR Address,
    IN SIZE_T Size)
{
    SIZE_T RoundupSize = ROUNDUP_TO_PAGE_SIZE(Size);
    U32 Options = XAD_LAF_ADDRESS;
    PTR FreeAddress = ROUNDDOWN_TO_PAGE_SIZE(Address);

    if (RoundupSize)
    {
        Options |= XAD_LAF_SIZE;
    }

    MmXadAcquireLock(&MiVadTree);

    MMXAD *Xad = NULL;
    ESTATUS Status = MmXadLookupAddress(&MiVadTree, &Xad, FreeAddress, RoundupSize, 0, Options);

    if (!E_IS_SUCCESS(Status))
    {
        MmXadReleaseLock(&MiVadTree);
        return Status;
    }

    if (Xad->Address.Type == VadFree || Xad->Address.Type == VadInitialReserved)
    {
        // Already freed
        MmXadReleaseLock(&MiVadTree);
        return Status;
    }

    SIZE_T FreeSize = RoundupSize ? RoundupSize :
        Xad->Address.Range.End - Xad->Address.Range.Start;

    ADDRESS AddressReclaim =
    {
        .Range.Start = FreeAddress,
        .Range.End = FreeAddress + FreeSize,
        .Type = Xad->PrevType,
    };

    Status = MmXadReclaimAddress(&MiVadTree, Xad, NULL, &AddressReclaim);

    MmXadReleaseLock(&MiVadTree);

    return Status;
}


/**
 * @brief Reallocates physical memory for given address.
 *
 * @param [in,out] Address  Pointer to caller-supplied variable which contains
 *                          desired address to allocate. if *Address == zero,
 *                          address will be determined automatically.
 * @param [in] Size         Allocation size.
 * @param [in] SourceType   Address type to lookup free memory.
 * @param [in] Type         New address type after allocation.
 *
 * @return ESTATUS code.
 */
ESTATUS
KERNELAPI
MmReallocatePhysicalMemory(
    IN OUT PTR *Address,
    IN SIZE_T Size,
    IN PAD_TYPE SourceType,
    IN PAD_TYPE Type)
{
    if (!Address)
    {
        return E_INVALID_PARAMETER;
    }

    if (SourceType == Type)
    {
        return E_INVALID_PARAMETER;
    }

    PTR CapturedAddressHint = *Address;
    MMXAD *XadTemp = NULL;
    SIZE_T RoundupSize = ROUNDUP_TO_PAGE_SIZE(Size);
    U32 Options = XAD_LAF_SIZE | XAD_LAF_TYPE;

    if (CapturedAddressHint)
    {
        CapturedAddressHint = ROUNDDOWN_TO_PAGE_SIZE(CapturedAddressHint);
        Options |= XAD_LAF_ADDRESS;
    }

    // Acquire XAD lock.
    MmXadAcquireLock(&MiPadTree);

    ESTATUS Status = MmXadLookupAddress(&MiPadTree, &XadTemp, CapturedAddressHint, RoundupSize, SourceType, Options);

    if (!E_IS_SUCCESS(Status))
    {
        MmXadReleaseLock(&MiPadTree);
        return Status;
    }

    U64 AddressStart = CapturedAddressHint ? CapturedAddressHint : XadTemp->Address.Range.Start;

    ADDRESS AddressReclaim =
    {
        .Range.Start = AddressStart,
        .Range.End = AddressStart + RoundupSize,
        .Type = Type,
    };

    Status = MmXadReclaimAddress(&MiPadTree, XadTemp, NULL, &AddressReclaim);
    if (!E_IS_SUCCESS(Status))
    {
        MmXadReleaseLock(&MiPadTree);
        return Status;
    }

    if (SourceType == PadInitialReserved)
    {
        // Override previous type.
        XadTemp->PrevType = PadFree;
    }

    MmXadReleaseLock(&MiPadTree);

    *Address = AddressStart;

    return E_SUCCESS;
}


/**
 * @brief Allocates physical memory for given address.
 *
 * @param [in,out] Address  Pointer to caller-supplied variable which contains
 *                          desired address to allocate. if *Address == zero,
 *                          address will be determined automatically.
 * @param [in] Size         Allocation size.
 * @param [in] SourceType   Address type to lookup free memory.
 * @param [in] Type         New address type after allocation.
 *
 * @return ESTATUS code.
 */
ESTATUS
KERNELAPI
MmAllocatePhysicalMemory2(
    IN OUT PTR *Address,
    IN SIZE_T Size,
    IN PAD_TYPE SourceType,
    IN PAD_TYPE Type)
{
    if (!Address)
    {
        return E_INVALID_PARAMETER;
    }

    if (SourceType != PadFree && SourceType != PadInitialReserved)
    {
        return E_INVALID_PARAMETER;
    }

    return MmReallocatePhysicalMemory(Address, Size, SourceType, Type);
}

/**
 * @brief Allocates physical memory for given address.
 *
 * @param [in,out] Address  Pointer to caller-supplied variable which contains
 *                          desired address to allocate. if *Address == zero,
 *                          address will be determined automatically.
 * @param [in] Size         Allocation size.
 * @param [in] Type         New address type after allocation.
 *
 * @return ESTATUS code.
 */
KEXPORT
ESTATUS
KERNELAPI
MmAllocatePhysicalMemory(
    IN OUT PTR *Address,
    IN SIZE_T Size,
    IN PAD_TYPE Type)
{
    return MmAllocatePhysicalMemory2(Address, Size, PadFree, Type);
}

/**
 * @brief Frees physical memory.
 *
 * @param [in] Address      Address to free.
 * @param [in] Size         Size to free.
 *
 * @return ESTATUS code.
 */
KEXPORT
ESTATUS
KERNELAPI
MmFreePhysicalMemory(
    IN PTR Address,
    IN SIZE_T Size)
{
    SIZE_T RoundupSize = ROUNDUP_TO_PAGE_SIZE(Size);
    U32 Options = XAD_LAF_ADDRESS;
    PTR FreeAddress = ROUNDDOWN_TO_PAGE_SIZE(Address);

    if (RoundupSize)
    {
        Options |= XAD_LAF_SIZE;
    }

    MmXadAcquireLock(&MiPadTree);

    MMXAD *Xad = NULL;
    ESTATUS Status = MmXadLookupAddress(&MiPadTree, &Xad, FreeAddress, RoundupSize, 0, Options);

    if (!E_IS_SUCCESS(Status))
    {
        MmXadReleaseLock(&MiPadTree);
        return Status;
    }

    if (Xad->Address.Type == PadFree || Xad->Address.Type == PadInitialReserved)
    {
        // Already freed
        MmXadReleaseLock(&MiPadTree);
        return Status;
    }

    SIZE_T FreeSize = RoundupSize ? RoundupSize :
        Xad->Address.Range.End - Xad->Address.Range.Start;

    ADDRESS AddressReclaim =
    {
        .Range.Start = FreeAddress,
        .Range.End = FreeAddress + FreeSize,
        .Type = Xad->PrevType,
    };

    Status = MmXadReclaimAddress(&MiPadTree, Xad, NULL, &AddressReclaim);

    MmXadReleaseLock(&MiPadTree);

    return Status;
}

/**
 * @brief Allocates PHYSICAL_ADDRESSES structure for given MaximumCount.
 * 
 * @param [in] PoolType         Pool type.
 * @param [in] MaximumCount     Maximum count.
 * 
 * @return Pointer to PHYSICAL_ADDRESSES if succeeds, NULL otherwise.
 */
KEXPORT
PHYSICAL_ADDRESSES *
KERNELAPI
MmAllocatePhysicalAddressesStructure(
    IN POOL_TYPE PoolType,
    IN U32 MaximumCount)
{
    SIZE_T StructureSize = SIZEOF_PHYSICAL_ADDRESSES(MaximumCount);
    PHYSICAL_ADDRESSES *PhysicalAddresses = MmAllocatePool(PoolType, StructureSize, 0x10, 0);

    if (!PhysicalAddresses || !MaximumCount)
    {
        return NULL;
    }

    INITIALIZE_PHYSICAL_ADDRESSES(PhysicalAddresses, 0, MaximumCount);

    return PhysicalAddresses;
}

/**
 * @brief Copies PHYSICAL_ADDRESSES structure to destination.
 * 
 * @param [out] Destination     Destination structure.\n
 *                              Caller must specify AddressMaximumCount as valid.\n
 *                              Caller must specify AddressCount to zero.
 * @param [in] Source           Source structure.
 * 
 * @return ESTATUS code.
 */
KEXPORT
ESTATUS
KERNELAPI
MmCopyPhysicalAddressesStructure(
    OUT PHYSICAL_ADDRESSES *Destination,
    IN PHYSICAL_ADDRESSES *Source)
{
    if (!Source->AddressCount ||
        Source->AddressCount > Source->AddressMaximumCount ||
        // Must set Destination->AddressCount to zero
        Destination->AddressCount ||
        Destination->AddressCount > Destination->AddressMaximumCount ||
        // Must check whether the destination buffer is large enough
        Destination->AddressMaximumCount < Source->AddressCount)
    {
        return E_INVALID_PARAMETER;
    }

    // Destination->AddressMaximumCount will not be changed as it specifies destination buffer size
    Destination->AddressCount = Source->AddressCount;
    Destination->AllocatedSize = Source->AllocatedSize;
    Destination->Mapped = Source->Mapped;
    Destination->StartingVirtualAddress = Source->StartingVirtualAddress;

    for (U32 i = 0; i < Source->AddressCount; i++)
    {
        Destination->Ranges[i] = Source->Ranges[i];
    }

    return E_SUCCESS;
}

/**
 * @brief Allocates physical memory blocks for given size.
 *
 * @param [in,out] PhysicalAddresses    Pointer to caller-supplied variable which points
 *                                      physical address list to be stored. See PHYSICAL_ADDRESSES.
 * @param [in] Size                     Allocation size.
 * @param [in] Type                     New address type after allocation.
 *
 * @return ESTATUS code.
 */
KEXPORT
ESTATUS
KERNELAPI
MmAllocatePhysicalMemoryGather(
    IN OUT PHYSICAL_ADDRESSES *PhysicalAddresses,
    IN SIZE_T Size,
    IN PAD_TYPE Type)
{
    if (!PhysicalAddresses)
    {
        return E_INVALID_PARAMETER;
    }

    if (Type == PadFree)
    {
        return E_INVALID_PARAMETER;
    }

    ESTATUS Status = E_SUCCESS;

    MmXadAcquireLock(&MiPadTree);

    SIZE_T AllocatedSize = 0;
    BOOLEAN SplitLastBlock = FALSE;
    SIZE_T SplitLastBlockSize = 0;

    PhysicalAddresses->Mapped = FALSE;
    PhysicalAddresses->AddressCount = 0;
    PhysicalAddresses->StartingVirtualAddress = NULL;

    for (INT i = MMXAD_MAX_SIZE_LEVELS - 1; i >= 0; i--)
    {
        DLIST_ENTRY *Head = &MiPadTree.SizeLinks[i];
        DLIST_ENTRY *Current = Head->Next;

        while (Head != Current)
        {
            if (PhysicalAddresses->AddressMaximumCount <=
                PhysicalAddresses->AddressCount)
            {
                DASSERT(AllocatedSize < Size);
                Status = E_BUFFER_TOO_SMALL;
                goto ExitLoop;
            }

            MMXAD *Temp = CONTAINING_RECORD(Current, MMXAD, Links);
            U64 BlockSize = Temp->Address.Range.End - Temp->Address.Range.Start;

            if (Temp->Address.Type == PadFree)
            {
                PhysicalAddresses->Ranges
                    [PhysicalAddresses->AddressCount++].Internal = Temp;

                if (AllocatedSize + BlockSize >= Size)
                {
                    SplitLastBlockSize = Size - AllocatedSize;
                    SplitLastBlock = TRUE;
                    AllocatedSize = Size;

                    goto ExitLoop;
                }
                else
                {
                    AllocatedSize += BlockSize;
                }
            }

            Current = Current->Next;
        }
    }

ExitLoop:

    if (AllocatedSize != Size)
    {
        return E_NOT_ENOUGH_MEMORY;
    }

    if (!E_IS_SUCCESS(Status))
    {
        MmXadReleaseLock(&MiPadTree);
        return Status;
    }
    
    // 
    // Reclaim addresses by address hint.
    // 

    for (U32 i = 0; i < PhysicalAddresses->AddressCount; i++)
    {
        MMXAD *Xad = PhysicalAddresses->Ranges[i].Internal;

        ADDRESS_RANGE Range = Xad->Address.Range;

        if (i + 1 == PhysicalAddresses->AddressCount && SplitLastBlock)
        {
            Range.End = Range.Start + SplitLastBlockSize;
        }

        ADDRESS AddressReclaim =
        {
            .Range = Range,
            .Type = Type,
        };

        //printf("Range 0x%016llx - 0x%016llx => type 0x%02x\n", Range.Start, Range.End, Type);

        Status = MmXadReclaimAddress(&MiPadTree, Xad, NULL, &AddressReclaim);
        DASSERT(E_IS_SUCCESS(Status));

        PhysicalAddresses->Ranges[i].Range = Range;
    }

    PhysicalAddresses->AllocatedSize = AllocatedSize;

    MmXadReleaseLock(&MiPadTree);

    return E_SUCCESS;
}

/**
 * @brief Frees physical memory blocks.
 *
 * @param [in] PhysicalAddresses    Physical addresses to free. See PHYSICAL_ADDRESSES.
 *
 * @return ESTATUS code.
 */
KEXPORT
ESTATUS
KERNELAPI
MmFreePhysicalMemoryGather(
    IN PHYSICAL_ADDRESSES *PhysicalAddresses)
{
    if (PhysicalAddresses->Mapped)
    {
        // unmap first!
        return E_INVALID_PARAMETER;
    }

    if (PhysicalAddresses->AddressCount > PhysicalAddresses->AddressMaximumCount)
    {
        return E_INVALID_PARAMETER;
    }

    MmXadAcquireLock(&MiPadTree);

    for (U32 i = 0; i < PhysicalAddresses->AddressCount; i++)
    {
        U64 StartAddress = PhysicalAddresses->Ranges[i].Range.Start;

        MMXAD *Xad = NULL;
        ESTATUS Status = MmXadLookupAddress(&MiPadTree, &Xad, StartAddress, 0, 0, XAD_LAF_ADDRESS);

        if (!E_IS_SUCCESS(Status))
        {
            MmXadReleaseLock(&MiPadTree);
            return Status;
        }

        if (Xad->Address.Range.Start != StartAddress)
        {
            MmXadReleaseLock(&MiPadTree);
            return E_INVALID_PARAMETER;
        }
    }

    for (U32 i = 0; i < PhysicalAddresses->AddressCount; i++)
    {
        MMXAD *Xad = NULL;
        ESTATUS Status = MmXadLookupAddress(&MiPadTree, &Xad,
            PhysicalAddresses->Ranges[i].Range.Start, 0, 0, XAD_LAF_ADDRESS);

        DASSERT(E_IS_SUCCESS(Status));

        ADDRESS AddressReclaim = {
            .Range = PhysicalAddresses->Ranges[i].Range,
            .Type = PadFree,
        };

        Status = MmXadReclaimAddress(&MiPadTree, Xad, NULL, &AddressReclaim);
        DASSERT(E_IS_SUCCESS(Status));
    }

    MmXadReleaseLock(&MiPadTree);

    return E_SUCCESS;
}

/**
 * @brief Allocates and maps the physical memory.
 * 
 * @param [in,out] PhysicalAddresses    Pointer to caller-supplied variable which points
 *                                      physical address list to be stored. See PHYSICAL_ADDRESSES.
 * @param [in] Size                     Allocation size.
 * @param [in] PxeFlags                 PXE flags.
 * @param [in] PadType                  Physical address type after allocation.
 * @param [in] VadType                  Virtual address type after allocation.
 * 
 * @return ESTATUS code.
 */
KEXPORT
ESTATUS
KERNELAPI
MmAllocateAndMapPagesGather(
    IN OUT PHYSICAL_ADDRESSES *PhysicalAddresses,
    IN SIZE_T Size,
    IN U64 PxeFlags,
    IN PAD_TYPE PadType,
    IN VAD_TYPE VadType)
{
    ESTATUS Status = MmAllocatePhysicalMemoryGather(PhysicalAddresses, Size, PadType);
    if (!E_IS_SUCCESS(Status))
    {
        return Status;
    }

    PTR VirtualAddress = 0;
    Status = MmAllocateVirtualMemory(NULL, &VirtualAddress, Size, VadType);
    if (!E_IS_SUCCESS(Status))
    {
        MmFreePhysicalMemoryGather(PhysicalAddresses);
        return Status;
    }

    Status = MmMapPages(PhysicalAddresses, VirtualAddress, PxeFlags, FALSE, 0);
    if (!E_IS_SUCCESS(Status))
    {
        MmFreeVirtualMemory(VirtualAddress, Size);
        MmFreePhysicalMemoryGather(PhysicalAddresses);
        return Status;
    }

    return Status;
}

/**
 * @brief Frees and unmaps the pages.
 * 
 * @param [in] PhysicalAddresses    List of physical addresses.
 * 
 * @return ESTATUS code.
 * 
 * @todo Currently unmapping is not performed as MmUnmapPages() is not implemented.
 */
KEXPORT
ESTATUS
KERNELAPI
MmFreeAndUnmapPagesGather(
    IN PHYSICAL_ADDRESSES *PhysicalAddresses)
{
    if (!PhysicalAddresses->Mapped)
    {
        return E_INVALID_PARAMETER;
    }

    // todo: unmap virtual memory! (not implemented)
    ESTATUS Status = E_SUCCESS; /* MmUnmapPages(...); */
    /* if (!E_IS_SUCCESS(Status))
    {
        return Status;
    } */

    Status = MmFreeVirtualMemory(
        PhysicalAddresses->StartingVirtualAddress, PhysicalAddresses->AllocatedSize);

    if (!E_IS_SUCCESS(Status))
    {
        // Simply panic as operations cannot be undone
        FATAL("Invalid PHYSICAL_ADDRESSES structure (looks like virtual address is already freed)");
    }

    Status = MmFreePhysicalMemoryGather(PhysicalAddresses);
    if (!E_IS_SUCCESS(Status))
    {
        // Simply panic as operations cannot be undone
        FATAL("Invalid PHYSICAL_ADDRESSES structure (looks like physical addresses are already freed)");
    }

    return Status;
}

/**
 * @brief Maps the physical addresses to virtual addresses.
 * 
 * @param [in] ToplevelTable            Pointer to top-level paging structure.
 * @param [in] ToplevelTableReverse     Pointer to top-level reverse mapping (that is, physical-to-virtual mapping) structure.
 * @param [in] PhysicalAddresses        Pointer to PHYSICAL_ADDRESSES structure which contains non-contiguous physical addresses.
 * @param [in] VirtualAddress           Virtual address.
 * @param [in] Flags                    PXE flags. See PxeFlags in MiArchX64SetPageMapping.
 * @param [in] AllowNonDefaultPageSize  If FALSE, uses only 4K paging. Otherwise, uses 2M paging if possible.
 * @param [in] PxePool                  PXE pool.
 * 
 * @return ESTATUS code.
 */
ESTATUS
KERNELAPI
MiMapMemory(
    IN U64 *ToplevelTable,
    IN U64 *ToplevelTableReverse,
    IN PHYSICAL_ADDRESSES *PhysicalAddresses,
    IN VIRTUAL_ADDRESS VirtualAddress, 
    IN U64 Flags, 
    IN BOOLEAN AllowNonDefaultPageSize,
    IN OBJECT_POOL *PxePool)
{
    if (PhysicalAddresses->AddressCount > 
        PhysicalAddresses->AddressMaximumCount)
    {
        return E_INVALID_PARAMETER;
    }

    if (PhysicalAddresses->Mapped)
    {
        return E_INVALID_PARAMETER;
    }

    VIRTUAL_ADDRESS TargetVirtualAddress = ROUNDDOWN_TO_PAGE_SIZE(VirtualAddress);
    for (U32 i = 0; i < PhysicalAddresses->AddressCount; i++)
    {
        ADDRESS_RANGE Range = PhysicalAddresses->Ranges[i].Range;
        SIZE_T Size = Range.End - Range.Start;

        // @todo: Revert page mapping if fails
        ASSERT(MiArchX64SetPageMapping(ToplevelTable, ToplevelTableReverse, TargetVirtualAddress, 
            Range.Start, Size, Flags, FALSE, AllowNonDefaultPageSize, PxePool));

        TargetVirtualAddress += Size;
    }

    PhysicalAddresses->StartingVirtualAddress = ROUNDDOWN_TO_PAGE_SIZE(VirtualAddress);
    PhysicalAddresses->Mapped = TRUE;

    return E_SUCCESS;
}

/**
 * @brief Maps the physical addresses to virtual addresses.
 * 
 * @param [in] PhysicalAddresses        Pointer to PHYSICAL_ADDRESSES structure which contains non-contiguous physical addresses.
 * @param [in] VirtualAddress           Virtual address.
 * @param [in] PxeFlags                 PXE flags. See PxeFlags in MiArchX64SetPageMapping.
 * @param [in] AllowNonDefaultPageSize  If FALSE, uses only 4K paging. Otherwise, uses 2M paging if possible.
 * @param [in] Reserved                 Reserved for future use.
 * 
 * @return ESTATUS code.
 */
KEXPORT
ESTATUS
KERNELAPI
MmMapPages(
    IN PHYSICAL_ADDRESSES *PhysicalAddresses,
    IN VIRTUAL_ADDRESS VirtualAddress,
    IN U64 PxeFlags, 
    IN BOOLEAN AllowNonDefaultPageSize,
    IN U64 Reserved)
{
//    OBJECT_POOL *PxePool = NULL;
//    if (MapFlags & MAP_MEMORY_FLAG_USE_PRE_INIT_PXE)
//    {
//        PxePool = &MiPreInitPxePool;
//    }

    return MiMapMemory(MiPML4TBase, MiRPML4TBase, PhysicalAddresses, VirtualAddress, 
        PxeFlags, AllowNonDefaultPageSize, &MiPreInitPxePool/* PxePool */);
}

/**
 * @brief Maps the single physical page to virtual addresses.
 * 
 * @param [in] PhysicalAddress          4K-aligned physical page address.
 * @param [in] VirtualAddress           Virtual address.
 * @param [in] PxeFlags                 PXE flags. See PxeFlags in MiArchX64SetPageMapping.
 * 
 * @return ESTATUS code.
 */
KEXPORT
ESTATUS
KERNELAPI
MmMapSinglePage(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN VIRTUAL_ADDRESS VirtualAddress,
    IN U64 PxeFlags)
{
    if (PhysicalAddress & PAGE_MASK)
    {
        return E_INVALID_PARAMETER;
    }

    PHYSICAL_ADDRESSES PhysicalAddresses = {
        .Mapped = FALSE,
        .AddressCount = 1,
        .AddressMaximumCount = 1,
        .StartingVirtualAddress = 0,
        .Ranges[0].Range.Start = PhysicalAddress,
        .Ranges[0].Range.End = PhysicalAddress + PAGE_SIZE - 1,
    };

    return MiMapMemory(MiPML4TBase, MiRPML4TBase, &PhysicalAddresses, VirtualAddress, 
        PxeFlags, FALSE, &MiPreInitPxePool/* PxePool */);
}

