
#pragma once

#define POOL_HEADER_ALIGNMENT_SHIFT     3
#define POOL_HEADER_ALIGNMENT           (1 << POOL_HEADER_ALIGNMENT_SHIFT)

#define POOL_FLAG_PAGED                 0x00000001
#define POOL_FLAG_DEBUG_BOUNDTEST       0x00000002

typedef enum _POOL_TYPE {
    PoolTypeNonPaged,
    PoolTypePaged,
//    PoolTypeNonPagedNx,
//    PoolTypePagedNx,
    PoolTypeNonPagedPreInit,
    PoolTypeMaximum,
} POOL_TYPE;

typedef struct _POOL_HEADER {
    U32 Tag;
    U16 Checksum;
    U8 AlignmemtShift;
    U8 Reserved1;

    // Pool Area Layout:
    // <PoolBlock1> <PoolBlock2> ... <PoolBlockN>
    //
    // Pool Block Layout:
    // [POOL_HEADER] [Size=BlockSize] [Size=BlockSizeReserved] [Size=BlockSizeUnused]

    DLIST_ENTRY BlockList;
    UPTR BlockSize;
    UPTR BlockSizeUnused;
    U32 BlockSizeReserved;
    U32 Reserved2;
} POOL_HEADER, *PPOOL_HEADER;

C_ASSERT(!(sizeof(POOL_HEADER) & 0x07));

#define POOL_BLOCK_END(_hdr)    (       \
        (UPTR)(_hdr)                    \
        + sizeof(POOL_HEADER)           \
        + (_hdr)->BlockSize             \
        + (_hdr)->BlockSizeReserved     \
        + (_hdr)->BlockSizeUnused       \
    )


typedef struct _POOL_BLOCK_LIST {
    KSPIN_LOCK Lock;
    U32 Flags;

    UPTR AreaStart;
    UPTR AreaSize;

    DLIST_ENTRY BlockListHead;
} POOL_BLOCK_LIST, *PPOOL_BLOCK_LIST;

extern POOL_BLOCK_LIST MiPoolList[PoolTypeMaximum];



BOOLEAN
KERNELAPI
MiInitializePoolBlockList(
    IN PPOOL_BLOCK_LIST PoolObject,
    IN UPTR AreaStart,
    IN UPTR AreaSize,
    IN U32 Flags);

VOID *
KERNELAPI
MmAllocatePool(
    IN POOL_TYPE Type,
    IN SIZE_T Size,
    IN U16 Alignment,
    IN U32 Tag);

VOID
KERNELAPI
MmFreePool(
    IN VOID *Address);

