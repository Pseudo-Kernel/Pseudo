
/**
 * @file xadtree.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements binary tree structure for physical/virtual address descriptor.
 * @version 0.1
 * @date 2021-09-14
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <base/base.h>
#include <base/gerror.h>
#include <init/bootgfx.h>

#include <mm/xadtree.h>
#include <ke/lock.h>
#include <mm/pool.h>
#include <mm/mm.h>


/**
 * @brief Initializes XAD structure.
 * 
 * @param [in] Xad          XAD structure to initialize.
 * @param [in] Start        Start address.
 * @param [in] End          End address.
 *
 * @return None.
 */
VOID
KERNELAPI
MiXadInitialize(
    OUT MMXAD *Xad,
    IN U64 Start,
    IN U64 End)
{
    memset(Xad, 0, sizeof(*Xad));

    Xad->Address.Range.Start = Start;
    Xad->Address.Range.End = End;
    Xad->Address.Type = 0;
    Xad->ReferenceCount = 0;
    Xad->AvlNode.Height = 0;
    Xad->AvlNode.Links.LeftChild = NULL;
    Xad->AvlNode.Links.RightChild = NULL;
    Xad->AvlNode.Links.Parent = NULL;
    Xad->PrevType = 0;

    DListInitializeHead(&Xad->Links);
}

/**
 * @brief Allocates and initializes XAD structure.
 * 
 * @param [in] CallerContext    Caller context.
 *
 * @return XAD pointer.
 */
MMXAD *
KERNELAPI
MiXadAllocate(
    IN XAD_CONTEXT *CallerContext)
{
    U32 Type = PoolTypeNonPagedPreInit;
    if (!CallerContext->UsePreInitPool)
    {
        Type = PoolTypeNonPaged;
    }

    MMXAD *Xad = MmAllocatePool(Type, sizeof(MMXAD), 8, 0);

    MiXadInitialize(Xad, 0, 0);

    return Xad;
}

/**
 * @brief Deletes the XAD structure.
 * 
 * @param [in] CallerContext    Currently zero.
 * @param [in] Xad              XAD pointer.
 *
 * @return None.
 */
VOID
KERNELAPI
MiXadDelete(
    IN PVOID CallerContext,
    IN MMXAD *Xad)
{
    DListRemoveEntry(&Xad->Links);

    MmFreePool(Xad);
}

/**
 * @brief Gets key structure of the XAD structure.
 * 
 * @param [in] CallerContext    Currently zero.
 * @param [in] Xad              XAD pointer.
 *
 * @return Key structure of XAD.
 */
ADDRESS *
KERNELAPI
MiXadGetAddress(
    IN PVOID CallerContext,
    IN MMXAD *Xad)
{
    return &Xad->Address;
}

/**
 * @brief Sets key structure of the XAD structure.
 * 
 * @param [in] CallerContext    Currently zero.
 * @param [in] Xad              XAD pointer.
 * @param [in] Address          Key structure.
 *
 * @return None.
 */
VOID
KERNELAPI
MiXadSetAddress(
    IN PVOID CallerContext,
    IN MMXAD *Xad,
    IN ADDRESS *Address)
{
    Xad->Address = *Address;
}

/**
 * @brief Compares key structures.
 * 
 * @param [in] CallerContext    Currently zero.
 * @param [in] Address1         Key structure 1.
 * @param [in] Address2         Key structure 2.
 *
 * @return 0 if Address1 overlaps Address2,
 *         -1 if Address1 < Address2,
 *         1 if Address1 > Address2.
 */
INT
KERNELAPI
MiXadCompareAddress(
    IN PVOID CallerContext,
    IN ADDRESS *Address1,
    IN ADDRESS *Address2)
{
    // Validate address range
    DASSERT(
        Address1->Range.Start < Address1->Range.End &&
        Address2->Range.Start < Address2->Range.End);

    if (MmXadIsInAddressRange(&Address1->Range, &Address2->Range) ||
        MmXadIsInAddressRange(&Address2->Range, &Address1->Range))
    {
        // treat overlapping case as equal
        return 0;
    }
    else if (Address1->Range.Start < Address2->Range.Start)
    {
        return -1;
    }
    else if (Address1->Range.Start > Address2->Range.Start)
    {
        return 1;
    }
    else if (Address1->Range.Start == Address2->Range.Start)
    {
        return 0;
    }
    else
    {
        DASSERT(FALSE);
        return 0;
    }
}

/**
 * @brief Converts key structure to string.
 * 
 * @param [in] CallerContext    Currently zero.
 * @param [in] Address          Key structure.
 * @param [out] Buffer          Caller-supplied string buffer.
 * @param [in] BufferLength     Size of string buffer.
 *
 * @return Returns written string length.
 */
SIZE_T
KERNELAPI
MiXadConvertAddressRangeToString(
    IN PVOID CallerContext,
    IN ADDRESS *Address,
    OUT CHAR *Buffer,
    IN SIZE_T BufferLength)
{
    SIZE_T WrittenCount = ClStrFormatU8(Buffer, BufferLength, 
        "{ 0x%016llX - 0x%016llX | type 0x%02X }", 
        Address->Range.Start, Address->Range.End, Address->Type);

    WrittenCount += ClStrTerminateU8(Buffer, BufferLength, WrittenCount);

    //return sprintf(Buffer, "{ 0x%016llX - 0x%016llX | type 0x%02X }", 
    //  Address->Range.Start, Address->Range.End, Address->Type);
    return WrittenCount;
}

#if 0
inline
VOID
MiXadReference(
    IN MMXAD *Xad)
{
    Xad->ReferenceCount++;
}

inline
VOID
MiXadDereference(
    IN MMXAD *Xad)
{
    Xad->ReferenceCount--;

    if (!Xad->ReferenceCount)
    {
        MiXadDelete(NULL, Xad);
    }
}
#endif

/**
 * @brief Converts size to size level.
 *
 * @param [in] Size             Size to convert.
 *
 * @return Returns size level. (=floor(log2(Size))).
 */
INT
KERNELAPI
MiXadSizeToSizeLevel(
    IN U64 Size)
{
    U64 PageCount = ROUNDUP_TO_PAGES(Size);
    U64 LevelPageCount = 1 << 0;

    // 
    // Level 0: Size < (1 << 0) Page
    // Level 1: (1 << 0) Page <= Size < (1 << 1) Page
    // Level 2: (1 << 1) Page <= Size < (1 << 2) Page
    // ...
    // 

    for (INT Level = 0; Level < MMXAD_MAX_SIZE_LEVELS; Level++)
    {
        if (PageCount < LevelPageCount)
            return Level;

        LevelPageCount <<= 1;
    }

    return MMXAD_MAX_SIZE_LEVELS - 1;
}

/**
 * @brief Updates size links of XAD.
 *
 * @param [in] XadTree          XAD tree.
 * @param [in] Xad              XAD to update size links.
 *
 * @return ESTATUS code.
 */
ESTATUS
KERNELAPI
MiXadUpdateSizeLinks(
    IN MMXAD_TREE *XadTree,
    IN MMXAD *Xad)
{
    U64 Size = Xad->Address.Range.End - Xad->Address.Range.Start;
    INT SizeLevel = MiXadSizeToSizeLevel(Size);

    if (SizeLevel < 0)
    {
        return E_INVALID_PARAMETER;
    }

    // Unlink and relink size link of XAD.
    DListRemoveEntry(&Xad->Links);
    DListInsertAfter(&XadTree->SizeLinks[SizeLevel], &Xad->Links);

    return E_SUCCESS;
}

/**
 * @brief Traverse function for debugging.
 * 
 * @param [in] CallerContext        Caller context.
 * @param [in] Xad                  XAD.
 * 
 * @return TRUE always.
 */
BOOLEAN
KERNELAPI
MmXadDebugTraverse(
    IN PVOID CallerContext,
    IN MMXAD *Xad)
{
    XAD_CONTEXT *Context = (XAD_CONTEXT *)CallerContext;

    if (Context->DebugPrintScreen)
    {
        BootGfxPrintTextFormat(
            "Xad 0x%016llx | 0x%016llx - 0x%016llx | 0x%08x\n", 
            Xad, Xad->Address.Range.Start, Xad->Address.Range.End - 1, 
            Xad->Address.Type);
    }

    if (Context->DebugPrintPort)
    {
        DbgTraceF(TraceLevelDebug,
            "Xad 0x%016llx | 0x%016llx - 0x%016llx | 0x%08x\n", 
            Xad, Xad->Address.Range.Start, Xad->Address.Range.End - 1, 
            Xad->Address.Type);
    }

    return TRUE;
}

/**
 * @brief Initializes XAD tree.
 *
 * @param [out] XadTree         XAD tree.
 * @param [in] CallerContext    Caller context.
 *
 * @return TRUE always.
 */
BOOLEAN
KERNELAPI
MmXadInitializeTree(
    OUT MMXAD_TREE *XadTree,
    IN PVOID CallerContext)
{
    RS_BINARY_TREE_OPERATIONS Operations =
    {
        .AllocateNode = (PRS_BINARY_TREE_ALLOCATE_NODE)&MiXadAllocate,
        .DeleteNode = (PRS_BINARY_TREE_DELETE_NODE)&MiXadDelete,
        .GetKey = (PRS_BINARY_TREE_GET_NODE_KEY)&MiXadGetAddress,
        .SetKey = (PRS_BINARY_TREE_SET_NODE_KEY)&MiXadSetAddress,
        .CompareKey = (PRS_BINARY_TREE_COMPARE_KEY)&MiXadCompareAddress,
        .KeyToString = (PRS_BINARY_TREE_KEY_TO_STRING)&MiXadConvertAddressRangeToString,
    };

    memset(XadTree, 0, sizeof(*XadTree));

    RsBtInitialize(&XadTree->Tree, &Operations, CallerContext);

    for (INT i = 0; i < COUNTOF(XadTree->SizeLinks); i++)
    {
        DListInitializeHead(&XadTree->SizeLinks[i]);
    }

    return TRUE;
}

/**
 * @brief Locks XAD tree.
 *
 * @param [in] XadTree          XAD tree.
 *
 * @return None.
 */
VOID
KERNELAPI
MmXadAcquireLock(
    IN MMXAD_TREE *XadTree)
{
    // Not implemented
}

/**
 * @brief Unlocks XAD tree.
 *
 * @param [in] XadTree          XAD tree.
 *
 * @return None.
 */
VOID
KERNELAPI
MmXadReleaseLock(
    IN MMXAD_TREE *XadTree)
{
    // Not implemented
}

/**
 * @brief Searches the XAD by given hint.
 *
 * @param [in] XadTree          XAD tree.
 * @param [out] Xad             Pointer to caller-supplied variable. The result XAD will be passed to the caller.
 * @param [in] AddressHint      Address hint.
 * @param [in] SizeHint         Size hint.
 * @param [in] TypeHint         Type hint.
 * @param [in] Options          Lookup options. Combinations of XAD_LAF_XXX flag.
 *
 * @return ESTATUS code.
 */
ESTATUS
KERNELAPI
MmXadLookupAddress(
    IN MMXAD_TREE *XadTree,
    OUT MMXAD **Xad,
    IN U64 AddressHint,
    IN U64 SizeHint,
    IN U32 TypeHint,
    IN U32 Options)
{
    MMXAD *LookupXad = NULL;

    if (!Xad)
    {
        return E_INVALID_PARAMETER;
    }

    if (Options & XAD_LAF_ADDRESS)
    {
        ADDRESS Address = { 0 };

        Address.Range.Start = AddressHint;
        Address.Range.End = AddressHint + 1;

        if (!RsBtLookup(&XadTree->Tree, &Address, (RS_BINARY_TREE_LINK **)&LookupXad))
        {
            return E_LOOKUP_FAILED;
        }

        if (Options & XAD_LAF_SIZE)
        {
            ADDRESS_RANGE Range =
            {
                .Start = AddressHint,
                .End = AddressHint + SizeHint,
            };

            if (!MmXadIsInAddressRange(&LookupXad->Address.Range, &Range))
            {
                return E_LOOKUP_FAILED;
            }
        }

        if (Options & XAD_LAF_TYPE)
        {
            if (LookupXad->Address.Type != TypeHint)
            {
                return E_LOOKUP_FAILED;
            }
        }
    }
    else
    {
        if (Options & XAD_LAF_SIZE)
        {
            INT SizeLevel = MiXadSizeToSizeLevel(SizeHint);

            if (SizeLevel < 0)
            {
                return E_INVALID_PARAMETER;
            }

            for (INT i = SizeLevel; i < MMXAD_MAX_SIZE_LEVELS; i++)
            {
                DLIST_ENTRY *Head = &XadTree->SizeLinks[i];
                DLIST_ENTRY *Current = Head->Next;

                while (Head != Current)
                {
                    MMXAD *Temp = CONTAINING_RECORD(Current, MMXAD, Links);
                    U64 Size = Temp->Address.Range.End - Temp->Address.Range.Start;

                    if (Size >= SizeHint)
                    {
                        if (!(Options & XAD_LAF_TYPE) || // doesn't care type
                            ((Options & XAD_LAF_TYPE) && Temp->Address.Type == TypeHint)) // cares type match
                        {
                            LookupXad = Temp;
                            break;
                        }
                    }

                    Current = Current->Next;
                }

                if (LookupXad)
                {
                    break;
                }
            }

            if (!LookupXad)
            {
                return E_LOOKUP_FAILED;
            }
        }
        else
        {
            return E_INVALID_PARAMETER;
        }
    }

    *Xad = LookupXad;

    return E_SUCCESS;
}

/**
 * @brief Checks whether the given ADDRESS_RANGE is valid.
 *
 * @param [in] AddressRange     Address range.
 *
 * @return TRUE if valid, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
MmXadIsAddressRangeValid(
    IN ADDRESS_RANGE *AddressRange)
{
    return AddressRange->Start < AddressRange->End;
}

/**
 * @brief Tests whether TestAddressRange is in SourceAddressRange.
 *
 * @param [in] SourceAddressRange   Source address range.
 * @param [in] TestAddressRange     Test address range.
 *
 * @return TRUE if SourceAddressRange includes TestAddressRange, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
MmXadIsInAddressRange(
    IN ADDRESS_RANGE *SourceAddressRange,
    IN ADDRESS_RANGE *TestAddressRange)
{
    if (!MmXadIsAddressRangeValid(SourceAddressRange) ||
        !MmXadIsAddressRangeValid(TestAddressRange))
        return FALSE;

    if (SourceAddressRange->Start <= TestAddressRange->Start &&
        TestAddressRange->End <= SourceAddressRange->End)
        return TRUE;

    return FALSE;
}

/**
 * @brief Inserts the address to XAD tree.
 *
 * @param [in] XadTree          XAD tree.
 * @param [out] Xad             Pointer to caller-supplied variable. The result XAD will be passed to the caller.
 * @param [in] Address          Address to insert.
 *
 * @return ESTATUS code.
 */
ESTATUS
KERNELAPI
MmXadInsertAddress(
    IN MMXAD_TREE *XadTree,
    OUT MMXAD **Xad,
    IN ADDRESS *Address)
{
    MMXAD *Inserted = NULL;

    if (!MmXadIsAddressRangeValid(&Address->Range))
    {
        // invalid range
        return E_INVALID_PARAMETER;
    }

    if (!RsAvlInsert(&XadTree->Tree, Address, (RS_BINARY_TREE_LINK **)&Inserted))
    {
        return E_FAILED;
    }

    ESTATUS Status = MiXadUpdateSizeLinks(XadTree, Inserted);
    if (!E_IS_SUCCESS(Status))
    {
        DASSERT(RsAvlDelete(&XadTree->Tree, &Inserted->AvlNode));
        return Status;
    }

    if (Xad)
    {
        *Xad = Inserted;
    }

    return E_SUCCESS;
}

/**
 * @brief Deletes the address to XAD tree.
 *
 * @param [in] XadTree          XAD tree.
 * @param [in] Address          Address to delete.
 *
 * @return ESTATUS code.
 */
ESTATUS
KERNELAPI
MmXadDeleteAddress(
    IN MMXAD_TREE *XadTree,
    IN ADDRESS *Address)
{
    if (!MmXadIsAddressRangeValid(&Address->Range))
    {
        // invalid range
        return E_INVALID_PARAMETER;
    }

    if (!RsAvlDeleteByKey(&XadTree->Tree, Address))
    {
        return E_FAILED;
    }

    return E_SUCCESS;
}

/**
 * @brief Merges the adjacent addresses if they have same type.
 *
 * @param [in] XadTree          XAD tree.
 * @param [in] AddressStartHint Address hint.
 *
 * @return ESTATUS code.
 */
ESTATUS
KERNELAPI
MiXadMergeAdjacentAddresses(
    IN MMXAD_TREE *XadTree,
    IN PTR AddressStartHint)
{
    MMXAD *Xad = NULL;

    ESTATUS Status = MmXadLookupAddress(XadTree, &Xad, AddressStartHint, 0, 0, XAD_LAF_ADDRESS);

    if (!E_IS_SUCCESS(Status))
    {
        return Status;
    }

    ADDRESS_RANGE Range = Xad->Address.Range;

    MMXAD *XadBefore = NULL;
    MMXAD *XadAfter = NULL;

    MMXAD *XadMergeList[3]; // Xad list to merge, ascending order
    INT MergeCount = 0;

    if (Range.Start > 0)
    {
        Status = MmXadLookupAddress(XadTree, &XadBefore, Range.Start - 1, 0, 0, XAD_LAF_ADDRESS);
        if (E_IS_SUCCESS(Status))
        {
            XadMergeList[MergeCount++] = XadBefore;
        }
    }

    XadMergeList[MergeCount++] = Xad;

    if (Range.End < 0xffffffffffffffffull)
    {
        Status = MmXadLookupAddress(XadTree, &XadAfter, Range.End + 1, 0, 0, XAD_LAF_ADDRESS);
        if (E_IS_SUCCESS(Status))
        {
            XadMergeList[MergeCount++] = XadAfter;
        }
    }


    //
    // Merge adjacent blocks.
    //

    MMXAD *XadMerge = XadMergeList[0];

    for (INT i = 1; i < MergeCount; i++)
    {
        if (XadMerge->Address.Range.End == XadMergeList[i]->Address.Range.Start && 
            XadMerge->Address.Type == XadMergeList[i]->Address.Type)
        {
            U64 NewEnd = XadMergeList[i]->Address.Range.End;

            // Delete node first before merge
            DASSERT(RsAvlDelete(&XadTree->Tree, &XadMergeList[i]->AvlNode));

            // Merge!
            XadMerge->Address.Range.End = NewEnd;

            // Relink the XAD because size is changed
            Status = MiXadUpdateSizeLinks(XadTree, XadMerge);
            DASSERT(E_IS_SUCCESS(Status));

            // No longer valid
            XadMergeList[i] = NULL;
        }
        else
        {
            XadMerge = XadMergeList[i];
        }
    }

    return E_SUCCESS;
}

/**
 * @brief Reclaims address with desired type.
 *
 * @param [in] XadTree          XAD tree.
 * @param [in] Xad              Target XAD.
 * @param [in] AddressHint      Target address hint.
 * @param [in] AddressReclaim   Desired address to reclaim.
 *
 * @return ESTATUS code.
 */
ESTATUS
KERNELAPI
MmXadReclaimAddress(
    IN MMXAD_TREE *XadTree,
    IN MMXAD *Xad,
    IN ADDRESS *AddressHint,
    IN ADDRESS *AddressReclaim)
{
    if ((Xad && AddressHint) || (!Xad && !AddressHint))
    {
        return E_INVALID_PARAMETER; // invalid parameter mix
    }

    ADDRESS CapturedReclaimAddress = *AddressReclaim;

    if (!MmXadIsAddressRangeValid(&CapturedReclaimAddress.Range))
    {
        return E_INVALID_PARAMETER;
    }

    MMXAD *TargetXad = Xad;

    if (!TargetXad)
    {
        if (!RsBtLookup(&XadTree->Tree, AddressHint, (RS_BINARY_TREE_LINK **)&TargetXad))
        {
            return E_LOOKUP_FAILED;
        }

        if (TargetXad->Address.Type != AddressHint->Type)
        {
            return E_LOOKUP_FAILED;
        }
    }
    else
    {
        // TODO: Should we check MMXAD?
    }


    ADDRESS TargetAddress = TargetXad->Address;

    U64 s1 = TargetAddress.Range.Start;
    U64 e1 = TargetAddress.Range.End;
    U32 t1 = TargetAddress.Type;
    U64 s2 = CapturedReclaimAddress.Range.Start;
    U64 e2 = CapturedReclaimAddress.Range.End;
    U32 t2 = CapturedReclaimAddress.Type;

    ADDRESS AddressSplit[3];
    BOOLEAN AddressIgnore[3];
    BOOLEAN SavePrevType[3];
    INT SplitCount = 0;

    #define XAD_SPLIT_ADD_ADDRESS(_start, _end, _type, _save_prev_type) \
        if ((_start) != (_end))                         \
        {                                               \
            AddressSplit[SplitCount] = (ADDRESS)        \
            {                                           \
                .Range.Start = (_start),                \
                .Range.End = (_end),                    \
                .Type = (_type),                        \
            };                                          \
            AddressIgnore[SplitCount] = FALSE;          \
            SavePrevType[SplitCount] = (_save_prev_type); \
            SplitCount++;                               \
        }


    if ((s1 <= s2 && s2 <= e1) && (s1 <= e2 && e2 <= e1))
    {
        if (s1 == s2)
        {
            // [s1, e2), [e2, e1)*
            XAD_SPLIT_ADD_ADDRESS(s1, e2, t2, TRUE);
            XAD_SPLIT_ADD_ADDRESS(e2, e1, t1, FALSE);
        }
        else if (e1 == e2)
        {
            // [s1, s2)*, [s2, e1)
            XAD_SPLIT_ADD_ADDRESS(s1, s2, t1, FALSE);
            XAD_SPLIT_ADD_ADDRESS(s2, e1, t2, TRUE);
        }
        else // s1 < s2
        {
            // [s1, s2)*, [s2, e2), [e2, e1)*
            XAD_SPLIT_ADD_ADDRESS(s1, s2, t1, FALSE);
            XAD_SPLIT_ADD_ADDRESS(s2, e2, t2, TRUE);
            XAD_SPLIT_ADD_ADDRESS(e2, e1, t1, FALSE);
        }

        ESTATUS Status = E_FAILED;

        for (INT i = 0; i < SplitCount; i++)
        {
            DASSERT(AddressSplit[i].Range.Start < AddressSplit[i].Range.End);

            if (AddressSplit[i].Range.Start == s1)
            {
                TargetXad->Address = AddressSplit[i];

                if (SavePrevType[i])
                {
                    // Save previous type.
                    TargetXad->PrevType = t1;
                }

                Status = MiXadUpdateSizeLinks(XadTree, TargetXad);
                DASSERT(E_IS_SUCCESS(Status));

                AddressIgnore[i] = TRUE;
                break;
            }
        }


        for (INT i = 0; i < SplitCount; i++)
        {
            if (AddressIgnore[i])
                continue;

            MMXAD *NewXad = NULL;
            DASSERT(RsAvlInsert(&XadTree->Tree, &AddressSplit[i], (RS_BINARY_TREE_LINK **)&NewXad));

            if (SavePrevType[i])
            {
                // Save previous type.
                NewXad->PrevType = t1;
            }

            Status = MiXadUpdateSizeLinks(XadTree, NewXad);
            DASSERT(E_IS_SUCCESS(Status));
        }

        if (s1 == s2 || e1 == e2)
        {
            // MiXadMergeAdjacentAddresses() may fail
            // Status = MiXadMergeAdjacentAddresses(XadTree, s1);
            // DASSERT(E_IS_SUCCESS(Status));
            // 
            // Status = MiXadMergeAdjacentAddresses(XadTree, e1);
            // DASSERT(E_IS_SUCCESS(Status));

            MiXadMergeAdjacentAddresses(XadTree, s1);
            MiXadMergeAdjacentAddresses(XadTree, e1);
        }

        return E_SUCCESS;
    }

    #undef XAD_SPLIT_ADD_ADDRESS

    return E_FAILED;
}


