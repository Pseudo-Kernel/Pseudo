
/**
 * @file objpool.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements object pool allocation/free.
 * @version 0.1
 * @date 2021-09-21
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <base/base.h>
#include <misc/misc.h>
#include <mm/pool.h>

/**
 * @brief Calculates maximum tree depth by object count.
 * 
 * @param [in] MaximumObjectCount       Maximum object count.
 * 
 * @return Maximum tree depth.
 */
U32
PoolGetMaximumDepth(
    IN U32 MaximumObjectCount)
{
    for (U32 i = 0; i < 32; i++)
    {
        if (MaximumObjectCount <= (1U << i))
            return i;
    }

    return 0;
}

/**
 * @brief Initializes pool bitmap.
 * 
 * @param [out] PoolBitmap          Pool bitmap structure to be initialized.
 * @param [in] MaximumObjectCount   Maximum object count.
 * @param [in] InitialBitmap        Initial bitmap. If this parameter is NULL, a new bitmap will be allocated.\n
 *                                  Otherwise, PoolBitmapInitialize does not allocate bitmap and uses InitialBitmap instead.
 * @param [in] InitialBitmapSize    Initial bitmap size in bytes. This parameter is ignored if InitialBitmap is NULL.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
PoolBitmapInitialize(
    OUT OBJECT_POOL_BITMAP *PoolBitmap,
    IN U32 MaximumObjectCount,
    OPTIONAL IN PVOID InitialBitmap,
    OPTIONAL IN U32 InitialBitmapSize)
{
    U32 MaximumDepth = PoolGetMaximumDepth(MaximumObjectCount);
    if (!MaximumDepth)
    {
        // size is too small
        return FALSE;
    }

    // (1 << (MaximumDepth + 1)) - 1 = 1 + 1..2 + 1..4 + ... + 1..(1 << MaximumDepth)
    U32 BitmapSize = POOL_BITMAP_SIZE_MINIMUM_BY_MAXIMUM_DEPTH(MaximumDepth);
    if (!BitmapSize)
    {
        // size is too small
        return FALSE;
    }

    U8 *Bitmap = NULL;
    BOOLEAN BitmapSpecified = FALSE;

    if (InitialBitmap)
    {
        if (BitmapSize > InitialBitmapSize)
        {
            // insufficient bitmap size
            return FALSE;
        }

        Bitmap = InitialBitmap;
        BitmapSpecified = TRUE;
    }
    else
    {
        Bitmap = (U8 *)MmAllocatePool(PoolTypeNonPaged, BitmapSize, 0x10, TAG4('o', 'b', 'j', 'p'));
        if (!Bitmap)
        {
            // failed to allocate bitmap
            return FALSE;
        }

        memset(Bitmap, 0, BitmapSize);
    }

    PoolBitmap->Bitmap = Bitmap;
    PoolBitmap->BitmapSize = BitmapSize;
    PoolBitmap->MaximumDepth = MaximumDepth;
    PoolBitmap->MaximumObjectCount = MaximumObjectCount;
    PoolBitmap->BitmapSpecified = BitmapSpecified;

    return TRUE;
}

/**
 * @brief Frees the pool bitmap.
 * 
 * @param [in] PoolBitmap       Pool bitmap structure.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
PoolBitmapFree(
    IN OBJECT_POOL_BITMAP *PoolBitmap)
{
    U8 *Bitmap = PoolBitmap->Bitmap;
    if (!Bitmap)
    {
        return FALSE;
    }

    if (!PoolBitmap->BitmapSpecified)
    {
        MmFreePool(Bitmap);
    }

    PoolBitmap->Bitmap = NULL;

    return TRUE;
}

/**
 * @brief Tests whether specified bit is set.
 * 
 * @param [in] PoolBitmap       Pool bitmap structure.
 * @param [in] BitIndex         Bit index to test.
 * 
 * @return TRUE if specified bit is set, FALSE otherwise.
 * 
 * @note This function does not check boundary.
 */
BOOLEAN
PoolBitmapGetBit(
    IN OBJECT_POOL_BITMAP *PoolBitmap,
    IN U32 BitIndex)
{
    return !!(PoolBitmap->Bitmap[BitIndex >> 3] & (1 << (BitIndex & 0x07)));
}

/**
 * @brief Sets/clears specified bit.
 * 
 * @param [in] PoolBitmap       Pool bitmap structure.
 * @param [in] BitIndex         Bit index to modify.
 * @param [in] Set              If non-zero, specified bit will be set.\n
 *                              Otherwise, specified bit will be cleared.
 * 
 * @return Returns previous bit.\n
 *         TRUE if previous bit was set, FALSE otherwise.
 * 
 * @note This function does not check boundary.
 */
BOOLEAN
PoolBitmapSetBit(
    IN OBJECT_POOL_BITMAP *PoolBitmap,
    IN U32 BitIndex,
    IN BOOLEAN Set)
{
    BOOLEAN PrevBit = PoolBitmapGetBit(PoolBitmap, BitIndex);

    if (Set)
        PoolBitmap->Bitmap[BitIndex >> 3] |= (1 << (BitIndex & 0x07));
    else
        PoolBitmap->Bitmap[BitIndex >> 3] &= ~(1 << (BitIndex & 0x07));

    return PrevBit;
}

/**
 * @brief Finds free index of bitmap.
 * 
 * @param [in] PoolBitmap       Pool bitmap structure.
 * @param [in] FreeBitIndex     Caller-supplied pointer to a variable that receives the freed bit index.
 * 
 * @return Object index that is currently freed.\n
 *         If there is no freed object to allocate, returns -1.
 */
INT
PoolBitmapFindFree(
    IN OBJECT_POOL_BITMAP *PoolBitmap,
    OUT INT *FreeBitIndex)
{
    U32 StartIndex = OBJPOOL_NODE_INDEX_ROOT;
    if (PoolBitmapGetBit(PoolBitmap, StartIndex))
    {
        // Cannot find freed
        return -1;
    }

    U32 Depth = 0;
    U32 IndexLimit = OBJPOOL_NODE_INDEX_START(PoolBitmap->MaximumDepth) + PoolBitmap->MaximumObjectCount - 1;

    while (Depth < PoolBitmap->MaximumDepth)
    {
        BOOLEAN NextDepthFound = FALSE;

        for (U32 i = 0; i < 2; i++)
        {
            U32 ChildIndex = OBJPOOL_NODE_CHILD(StartIndex, i);

            if (IndexLimit < ChildIndex)
                break;

            if (!PoolBitmapGetBit(PoolBitmap, ChildIndex))
            {
                StartIndex = ChildIndex;
                NextDepthFound = TRUE;
                break;
            }
        }

        if (!NextDepthFound)
        {
            return -1;
        }

        Depth++;
    }

    INT ObjectIndex = StartIndex - OBJPOOL_NODE_INDEX_START(Depth);

    if (FreeBitIndex)
        *FreeBitIndex = StartIndex;

    return ObjectIndex;
}

/**
 * @brief Sets the pool bitmap to allocated state.
 * 
 * @param [in] PoolBitmap       Pool bitmap structure.
 * @param [in] Index            Bit index.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
PoolBitmapSetAllocate(
    IN OBJECT_POOL_BITMAP *PoolBitmap,
    IN U32 Index)
{
    U32 Start = OBJPOOL_NODE_INDEX_START(PoolBitmap->MaximumDepth);
    U32 End = OBJPOOL_NODE_INDEX_END(PoolBitmap->MaximumDepth);

    if (Start > Index || Index > End)
        return FALSE;

    if (PoolBitmapSetBit(PoolBitmap, Index, TRUE))
    {
        // Already allocated
        return FALSE;
    }

    U32 CurrentIndex = Index;
    while (CurrentIndex != OBJPOOL_NODE_INDEX_ROOT)
    {
        U32 ParentIndex = OBJPOOL_NODE_PARENT(CurrentIndex);

        //
        // Set parent bit if both left and right child are set
        //

        /**
         *       0              0
         *     /   \          /   \
         *    0     0   =>   1     0
         *   / \   / \      / \   / \
         *  1   0 1   0    1   1 1   0
         *      ^              ^
         */

        BOOLEAN LeftBit = PoolBitmapGetBit(PoolBitmap, OBJPOOL_NODE_CHILD(ParentIndex, 0));
        BOOLEAN RightBit = PoolBitmapGetBit(PoolBitmap, OBJPOOL_NODE_CHILD(ParentIndex, 1));

        if (!LeftBit || !RightBit)
            break;

        PoolBitmapSetBit(PoolBitmap, ParentIndex, TRUE);

        CurrentIndex = ParentIndex;
    }

    return TRUE;
}

/**
 * @brief Sets the pool bitmap to freed state.
 * 
 * @param [in] PoolBitmap       Pool bitmap structure.
 * @param [in] Index            Bit index.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
PoolBitmapSetFree(
    IN OBJECT_POOL_BITMAP *PoolBitmap,
    IN U32 Index)
{
    U32 Start = OBJPOOL_NODE_INDEX_START(PoolBitmap->MaximumDepth);
    U32 End = OBJPOOL_NODE_INDEX_END(PoolBitmap->MaximumDepth);

    if (Start > Index || Index > End)
        return FALSE;

    if (!PoolBitmapSetBit(PoolBitmap, Index, FALSE))
    {
        // Already freed
        return FALSE;
    }

    U32 CurrentIndex = Index;
    while (CurrentIndex != OBJPOOL_NODE_INDEX_ROOT)
    {
        //
        // Clear parent bit.
        //

        /**
         *       1              0
         *     /   \          /   \
         *    1     1   =>   0     1
         *   / \   / \      / \   / \
         *  1   1 1   1    1   0 1   1
         *      ^              ^
         */

        U32 ParentIndex = OBJPOOL_NODE_PARENT(CurrentIndex);
        PoolBitmapSetBit(PoolBitmap, ParentIndex, FALSE);

        CurrentIndex = ParentIndex;
    }

    return TRUE;
}

/**
 * @brief Initializes the pool structure.
 * 
 * @param [out] Pool                    Pool structure to be initialized.
 * @param [in] MaximumObjectCount       Maximum object count of the pool.
 * @param [in] SizeOfObject             Size of each object.
 * @param [in] InitialBitmap            Initial bitmap. PoolInitialize does not allocate additional memory\n
 *                                      and uses this parameter if Non-NULL value is specified.
 * @param [in] InitialBitmapSize        Initial bitmap size. This parameter is only valid if InitialBitmap is not NULL.
 * @param [in] InitialPool              Initial pool address. PoolInitialize does not allocate additional memory\n
 *                                      and uses this parameter if Non-NULL value is specified.
 * @param [in] InitialPoolSize          Initial pool size. This parameter is only valid if InitialPool is not NULL.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
PoolInitialize(
    OUT OBJECT_POOL *Pool,
    IN U32 MaximumObjectCount,
    IN U32 SizeOfObject,
    OPTIONAL IN U8 *InitialBitmap,
    OPTIONAL IN U32 InitialBitmapSize,
    OPTIONAL IN PVOID InitialPool,
    OPTIONAL IN U64 InitialPoolSize)
{
    if (!SizeOfObject)
    {
        return FALSE;
    }

    if (!PoolBitmapInitialize(&Pool->AllocationBitmap, MaximumObjectCount, InitialBitmap, InitialBitmapSize))
    {
        return FALSE;
    }

    PVOID NewPool = NULL;
    BOOLEAN PoolSpecified = FALSE;
    U64 PoolSize = (U64)MaximumObjectCount * (U64)SizeOfObject;

    if (InitialPool)
    {
        if (PoolSize > InitialPoolSize)
        {
            // insufficient pool size
            return FALSE;
        }

        NewPool = InitialPool;
        PoolSpecified = TRUE;
    }
    else
    {
        NewPool = MmAllocatePool(PoolTypeNonPaged, PoolSize, 0x10, TAG4('o', 'b', 'j', 'p'));
        if (!NewPool)
        {
            // failed to allocate bitmap
            return FALSE;
        }

        memset(NewPool, 0, PoolSize);
    }

    Pool->Pool = NewPool;
    Pool->SizeOfObject = SizeOfObject;
    Pool->PoolSpecified = PoolSpecified;

    return TRUE;
}

/**
 * @brief Frees the pool structure.
 * 
 * @param [in] Pool     Pool structure.
 *  
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
PoolFree(
    IN OBJECT_POOL *Pool)
{
    if (!PoolBitmapFree(&Pool->AllocationBitmap))
    {
        return FALSE;
    }

    if (!Pool->PoolSpecified)
    {
        MmFreePool(Pool->Pool);
    }

    Pool->Pool = NULL;

    return TRUE;
}

/**
 * @brief Allocates the object from the pool.
 * 
 * @param [in] Pool     Pool structure.
 * 
 * @return Object pointer. Non-null if succeeds, NULL otherwise.
 */
PVOID
PoolAllocateObject(
    IN OBJECT_POOL *Pool)
{
    INT FreeBitIndex = -1;
    INT Index = PoolBitmapFindFree(&Pool->AllocationBitmap, &FreeBitIndex);

    if (Index < 0)
    {
        return NULL;
    }

    //ASSERT(FreeBitIndex >= 0);

    if (!PoolBitmapSetAllocate(&Pool->AllocationBitmap, FreeBitIndex))
    {
        // ASSERT(FALSE);
        return NULL;
    }

    PVOID Object = (PVOID)((U8 *)Pool->Pool + Pool->SizeOfObject * (U32)Index);

    return Object;
}

/**
 * @brief Frees the object from the pool.
 * 
 * @param [in] Pool         Pool structure.
 * @param [in] Object       Object to be freed.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
PoolFreeObject(
    IN OBJECT_POOL *Pool,
    IN PVOID Object)
{
    U32 Offset = (U32)((U8 *)Object - (U8 *)Pool->Pool);

    if (Offset % Pool->SizeOfObject)
    {
        // invalid object address
        return FALSE;
    }

    U32 Index = OBJPOOL_NODE_INDEX_START(Pool->AllocationBitmap.MaximumDepth) + 
        (Offset / Pool->SizeOfObject);

    if (!PoolBitmapSetFree(&Pool->AllocationBitmap, Index))
    {
        return FALSE;
    }

    return TRUE;
}

