
/**
 * @file mm.c
 * @brief Implements memory management functions.
 * @todo Implement MmMapMemory which maps physical address to virtual address.\n
 *       Implement memory manager initialization (Initialize pool, initialize XAD trees, etc.).\n
 */

#include <mm/mm.h>

MMXAD_TREE MiPadTree; //!< Physical address tree.
MMXAD_TREE MiVadTree; //!< Virtual address tree for shared kernel space.


ESTATUS
MmAllocateVirtualMemory2(
    IN PVOID ReservedZero,
	IN OUT PTR *Address,
	IN SIZE_T Size,
    IN VAD_TYPE SourceType,
	IN VAD_TYPE Type)
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
{
	if (!Address)
	{
    	return E_INVALID_PARAMETER;
	}

    if (SourceType != VadFree)
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
		CapturedAddressHint = ROUNTDOWN_TO_PAGE_SIZE(CapturedAddressHint);
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

    MmXadReleaseLock(&MiVadTree);

	*Address = AddressStart;

	return E_SUCCESS;
}

KEXPORT
ESTATUS
MmAllocateVirtualMemory(
	IN OUT PTR *Address,
	IN SIZE_T Size,
	IN VAD_TYPE Type)
/**
 * @brief Allocates virtual memory for given address.
 * 
 * @param [in,out] Address  Pointer to caller-supplied variable which contains
 *                          desired address to allocate. if *Address == zero,
 *                          address will be determined automatically.
 * @param [in] Size         Allocation size.
 * @param [in] Type         New address type after allocation.
 *
 * @return ESTATUS code.
 */
{
    return MmAllocateVirtualMemory2(NULL, Address, Size, VadFree, Type);
}

KEXPORT
ESTATUS
MmFreeVirtualMemory(
	IN PTR Address,
	IN SIZE_T Size)
/**
 * @brief Frees virtual memory.
 * 
 * @param [in] Address      Address to free.
 * @param [in] Size         Size to free.
 *
 * @return ESTATUS code.
 */
{
	SIZE_T RoundupSize = ROUNDUP_TO_PAGE_SIZE(Size);
	U32 Options = XAD_LAF_ADDRESS;
	PTR FreeAddress = ROUNTDOWN_TO_PAGE_SIZE(Address);

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



ESTATUS
MmAllocatePhysicalMemory2(
    IN OUT PTR *Address,
    IN SIZE_T Size,
    IN PAD_TYPE SourceType,
    IN PAD_TYPE Type)
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
{
    if (!Address)
    {
        return E_INVALID_PARAMETER;
    }

    if (SourceType != PadFree)
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
        CapturedAddressHint = ROUNTDOWN_TO_PAGE_SIZE(CapturedAddressHint);
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

    MmXadReleaseLock(&MiPadTree);

    *Address = AddressStart;

    return E_SUCCESS;
}

KEXPORT
ESTATUS
MmAllocatePhysicalMemory(
    IN OUT PTR *Address,
    IN SIZE_T Size,
    IN PAD_TYPE Type)
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
{
    return MmAllocatePhysicalMemory2(Address, Size, PadFree, Type);
}

KEXPORT
ESTATUS
MmFreePhysicalMemory(
    IN PTR Address,
    IN SIZE_T Size)
/**
 * @brief Frees physical memory.
 *
 * @param [in] Address      Address to free.
 * @param [in] Size         Size to free.
 *
 * @return ESTATUS code.
 */
{
    SIZE_T RoundupSize = ROUNDUP_TO_PAGE_SIZE(Size);
    U32 Options = XAD_LAF_ADDRESS;
    PTR FreeAddress = ROUNTDOWN_TO_PAGE_SIZE(Address);

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

    if (Xad->Address.Type == PadFree)
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

KEXPORT
ESTATUS
MmAllocatePhysicalMemoryGather(
    IN OUT PHYSICAL_ADDRESSES *PhysicalAddresses,
    IN SIZE_T Size,
    IN PAD_TYPE Type)
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
    PhysicalAddresses->PhysicalAddressCount = 0;
    PhysicalAddresses->StartingVirtualAddress = NULL;

    for (INT i = MMXAD_MAX_SIZE_LEVELS - 1; i >= 0; i--)
    {
        DLIST_ENTRY *Head = &MiPadTree.SizeLinks[i];
        DLIST_ENTRY *Current = Head->Next;

        while (Head != Current)
        {
            if (PhysicalAddresses->PhysicalAddressMaximumCount <=
                PhysicalAddresses->PhysicalAddressCount)
            {
                DASSERT(AllocatedSize < Size);
                Status = E_BUFFER_TOO_SMALL;
                goto ExitLoop;
            }

            MMXAD *Temp = CONTAINING_RECORD(Current, MMXAD, Links);
            U64 BlockSize = Temp->Address.Range.End - Temp->Address.Range.Start;

            if (Temp->Address.Type == PadFree)
            {
				PhysicalAddresses->PhysicalAddresses
					[PhysicalAddresses->PhysicalAddressCount++].Internal = Temp;

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

	for (U32 i = 0; i < PhysicalAddresses->PhysicalAddressCount; i++)
	{
		MMXAD *Xad = PhysicalAddresses->PhysicalAddresses[i].Internal;

		ADDRESS_RANGE Range = Xad->Address.Range;

		if (i + 1 == PhysicalAddresses->PhysicalAddressCount && SplitLastBlock)
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

		PhysicalAddresses->PhysicalAddresses[i].Range = Range;
	}

	MmXadReleaseLock(&MiPadTree);

	return E_SUCCESS;
}

KEXPORT
ESTATUS
MmFreePhysicalMemoryGather(
    IN PHYSICAL_ADDRESSES *PhysicalAddresses)
/**
 * @brief Frees physical memory blocks.
 *
 * @param [in] PhysicalAddresses    Physical addresses to free. See PHYSICAL_ADDRESSES.
 *
 * @return ESTATUS code.
 */
{
    if (PhysicalAddresses->Mapped)
    {
        // unmap first!
        return E_INVALID_PARAMETER;
    }

    if (PhysicalAddresses->PhysicalAddressCount > PhysicalAddresses->PhysicalAddressMaximumCount)
    {
        return E_INVALID_PARAMETER;
    }

    MmXadAcquireLock(&MiPadTree);

    for (U32 i = 0; i < PhysicalAddresses->PhysicalAddressCount; i++)
    {
        U64 StartAddress = PhysicalAddresses->PhysicalAddresses[i].Range.Start;

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

    for (U32 i = 0; i < PhysicalAddresses->PhysicalAddressCount; i++)
    {
        MMXAD *Xad = NULL;
        ESTATUS Status = MmXadLookupAddress(&MiPadTree, &Xad,
            PhysicalAddresses->PhysicalAddresses[i].Range.Start, 0, 0, XAD_LAF_ADDRESS);

        DASSERT(E_IS_SUCCESS(Status));

        ADDRESS AddressReclaim = {
            .Range = PhysicalAddresses->PhysicalAddresses[i].Range,
            .Type = PadFree,
        };

        Status = MmXadReclaimAddress(&MiPadTree, Xad, NULL, &AddressReclaim);
        DASSERT(E_IS_SUCCESS(Status));
    }


    MmXadReleaseLock(&MiPadTree);

	return E_SUCCESS;
}

