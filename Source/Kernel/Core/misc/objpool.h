
#pragma once


#define	OBJPOOL_NODE_INDEX_ROOT             0
#define	OBJPOOL_NODE_PARENT(_index)         ( ((_index) - 1) >> 1 )
#define	OBJPOOL_NODE_CHILD(_index, _child)  ( ((_index) << 1) + 1 + (_child) )
#define	OBJPOOL_NODE_INDEX_START(_level)    ( (1 << (_level)) - 1 )
#define	OBJPOOL_NODE_INDEX_END(_level)      ( ((1 << (_level)) - 1) << 1 )

typedef struct _OBJECT_POOL_BITMAP
{
    U32 MaximumDepth;       // int(ceil(log2(MaximumObjectCount)))
    U32 MaximumObjectCount;
    U8 *Bitmap;             // size = int(ceil(MaximumObjectCount / 8)))
    U32 BitmapSize;
    BOOLEAN BitmapSpecified;
} OBJECT_POOL_BITMAP;

typedef struct _OBJECT_POOL
{
    OBJECT_POOL_BITMAP AllocationBitmap;
    PVOID Pool;
    U32 SizeOfObject;
    BOOLEAN PoolSpecified;
} OBJECT_POOL;




BOOLEAN
PoolBitmapInitialize(
    IN OBJECT_POOL_BITMAP *PoolBitmap,
    IN U32 MaximumObjectCount,
    OPTIONAL IN PVOID InitialBitmap,
    OPTIONAL IN U32 InitialBitmapSize);

BOOLEAN
PoolBitmapFree(
    IN OBJECT_POOL_BITMAP *PoolBitmap);

INT
PoolBitmapFindFree(
    IN OBJECT_POOL_BITMAP *PoolBitmap,
    OUT INT *FreeBitIndex);

BOOLEAN
PoolBitmapSetAllocate(
    IN OBJECT_POOL_BITMAP *PoolBitmap,
    IN U32 Index);

BOOLEAN
PoolBitmapSetFree(
    IN OBJECT_POOL_BITMAP *PoolBitmap,
    IN U32 Index);

BOOLEAN
PoolInitialize(
    OUT OBJECT_POOL *Pool,
    IN U32 MaximumObjectCount,
    IN U32 SizeOfObject,
    OPTIONAL IN U8 *InitialBitmap,
    OPTIONAL IN U32 InitialBitmapSize,
    OPTIONAL IN PVOID InitialPool,
    OPTIONAL IN U64 InitialPoolSize);

BOOLEAN
PoolFree(
    IN OBJECT_POOL *Pool);

PVOID
PoolAllocateObject(
    IN OBJECT_POOL *Pool);

BOOLEAN
PoolFreeObject(
    IN OBJECT_POOL *Pool,
    IN PVOID Object);



