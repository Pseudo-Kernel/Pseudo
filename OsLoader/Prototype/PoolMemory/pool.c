
#include <stdio.h>
#include <Windows.h>
#include "Arch\Arch.h"
#include "lock.h"
#include "list.h"

#define	POOL_HEADER_ALIGNMENT_SHIFT		3
#define	POOL_HEADER_ALIGNMENT			(1 << POOL_HEADER_ALIGNMENT_SHIFT)

#define	POOL_FLAG_PAGED					0x00000001
#define	POOL_FLAG_DEBUG_BOUNDTEST		0x00000002

typedef enum _POOL_TYPE {
	PoolTypeNonPaged,
	PoolTypePaged,
	PoolTypeNonPagedNx,
	PoolTypePagedNx,
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

#define	POOL_BLOCK_END(_hdr)	(		\
		(UPTR)(_hdr)					\
		+ sizeof(POOL_HEADER)			\
		+ (_hdr)->BlockSize				\
		+ (_hdr)->BlockSizeReserved		\
		+ (_hdr)->BlockSizeUnused		\
	)


typedef struct _POOL_BLOCK_LIST {
	KSPIN_LOCK2 Lock;
	U32 Flags;

	UPTR AreaStart;
	UPTR AreaSize;

	DLIST_ENTRY BlockListHead;
} POOL_BLOCK_LIST, *PPOOL_BLOCK_LIST;

POOL_BLOCK_LIST MiPoolList[PoolTypeMaximum];

typedef struct _POOL_INITIAL_PARAMETERS {
	POOL_TYPE Type;
	UPTR AreaStart;
	UPTR AreaSize;
	U32 Flags;
} POOL_INITIAL_PARAMETERS;

POOL_INITIAL_PARAMETERS MiInitialPoolParameters[] = {
	{ PoolTypeNonPaged,        0xffff800000000000, 0x0000000000000000, 0, },
	{ PoolTypePaged,           0xffff800000000000, 0x0000000000000000, 0, },
	{ PoolTypeNonPagedNx,      0xffff800000000000, 0x0000000000000000, 0, },
	{ PoolTypePagedNx,	       0xffff800000000000, 0x0000000000000000, 0, },
	{ PoolTypeNonPagedPreInit, 0xffff800000000000, 0x0000000000000000, 0, },
};

#define	POOL_ASSERT(_cond)	\
	if (!(_cond)) {			\
		__debugbreak();		\
	}


U16
KERNELAPI
MiComputePoolChecksum(
	IN PPOOL_HEADER Header)
{
	U16 Checksum = 0;
	U32 i = 0;

	for (i = 0; i < sizeof(*Header); i++)
	{
		Checksum += *((U8 *)Header + i);
	}

	Checksum -= (U8)(Header->Checksum >> 0x08) + (U8)Header->Checksum;

	return Checksum;
}

BOOLEAN
KERNELAPI
MiPoolChecksumMismatch(
	IN PPOOL_HEADER Header)
{
	if (MiComputePoolChecksum(Header) == Header->Checksum)
		return FALSE;

	return TRUE;
}


PPOOL_BLOCK_LIST
KERNELAPI
MiLookupPoolByAddress(
	IN UPTR Address)
{
	U32 i;
	for (i = 0; i < ARRAYSIZE(MiPoolList); i++)
	{
		UPTR Start = MiPoolList[i].AreaStart;
		UPTR End = MiPoolList[i].AreaStart + MiPoolList[i].AreaSize;

		if (Start <= Address && Address < End)
			return MiPoolList + i;
	}

	return NULL;
}

PPOOL_BLOCK_LIST
KERNELAPI
MiLookupPool(
	IN POOL_TYPE Type)
{
	if (0 <= Type && Type < PoolTypeMaximum)
		return MiPoolList + Type;

	return NULL;
}

VOID
KERNELAPI
MiInitializePoolHeader(
	IN OUT PPOOL_HEADER Header,
	IN U32 Tag, 
	IN UPTR BlockSize,
	IN UPTR BlockSizeReserved,
	IN UPTR BlockSizeUnused,
	IN U8 AlignmentShift)
{
	POOL_ASSERT(BlockSizeReserved < 0x1000);

	Header->AlignmemtShift = AlignmentShift;
	Header->BlockSize = BlockSize;

	Header->BlockSizeReserved = (U32)BlockSizeReserved;
	Header->BlockSizeUnused = BlockSizeUnused;
	Header->Checksum = 0;
	Header->Tag = Tag;
	Header->Reserved1 = 0;
	Header->Reserved2 = 0;

	Header->BlockList.Prev = NULL;
	Header->BlockList.Next = NULL;
}

VOID
KERNELAPI
MiUpdatePoolHeaderChecksum(
	IN PPOOL_HEADER Header)
{
	Header->Checksum = MiComputePoolChecksum(Header);
}

BOOLEAN
KERNELAPI
MiLinkPoolBlock(
	IN PPOOL_HEADER BlockHeader)
{
	PPOOL_BLOCK_LIST BlockList;

	BlockList = MiLookupPoolByAddress((UPTR)BlockHeader);
	if (!BlockList)
		return FALSE;

	POOL_ASSERT( KeIsSpinLockAcquired(&BlockList->Lock) );
	InsertDListBefore(&BlockList->BlockListHead, &BlockHeader->BlockList);

	return TRUE;
}

BOOLEAN
KERNELAPI
MiInitializePoolBlockList(
	IN PPOOL_BLOCK_LIST PoolObject,
	IN UPTR AreaStart,
	IN UPTR AreaSize, 
	IN U32 Flags)
{
	PPOOL_HEADER FirstHeader;

	if (!AreaSize || AreaSize < sizeof(POOL_HEADER))
		return FALSE;

	KeInitializeSpinLock(&PoolObject->Lock);

	PoolObject->AreaStart = AreaStart;
	PoolObject->AreaSize = AreaSize;
	PoolObject->Flags = Flags;

	InitializeDListHead(&PoolObject->BlockListHead);

	// Initialize the first free.
	FirstHeader = (PPOOL_HEADER)AreaStart;
	MiInitializePoolHeader(FirstHeader, 'TINI', 0, 0, AreaSize - sizeof(POOL_HEADER), 0);
	InitializeDListHead(&FirstHeader->BlockList);
	InsertDListBefore(&PoolObject->BlockListHead, &FirstHeader->BlockList);
	MiUpdatePoolHeaderChecksum(FirstHeader);

	return TRUE;
}

VOID *
KERNELAPI
MmAllocatePool(
	IN POOL_TYPE Type,
	IN SIZE_T Size, 
	IN U16 Alignment, 
	IN U32 Tag)
{
	PPOOL_BLOCK_LIST BlockList;
	PDLIST_ENTRY ListHead;
	PDLIST_ENTRY ListCurrent;
	SIZE_T AlignedSize;
	U8 AlignmentShift;

	PPOOL_HEADER BlockHeader;
	UPTR BlockAddress = 0;

	U32 BitsCount = 0;
	U32 i;

	AlignedSize = (Size + POOL_HEADER_ALIGNMENT - 1) & ~(POOL_HEADER_ALIGNMENT - 1);

	BlockList = MiLookupPool(Type);
	if (!BlockList)
		return FALSE;

	for (i = 0; i < sizeof(Alignment) * 8; i++)
	{
		if (Alignment & (1 << i))
		{
			BitsCount++;
			AlignmentShift = (U8)i;
		}
	}

	if (BitsCount >= 2)
		return FALSE;

	if (Alignment < POOL_HEADER_ALIGNMENT)
	{
		Alignment = POOL_HEADER_ALIGNMENT;
		AlignmentShift = POOL_HEADER_ALIGNMENT_SHIFT;
	}


	KeAcquireSpinLock(&BlockList->Lock);

	ListHead = &BlockList->BlockListHead;
	ListCurrent = ListHead->Next;

	while (ListCurrent != ListHead)
	{
		PPOOL_HEADER BlockHeader2 = NULL;
		PPOOL_HEADER BlockHeader3 = NULL;
		UPTR BlockHeaderNextTemp;
		UPTR BlockHeaderNext;
		UPTR BlockEnd;

		UPTR BlockStartUnaligned;
		UPTR BlockStartAligned;
		UPTR BlockSizeUnused;

		BlockHeader = CONTAINING_RECORD(ListCurrent, POOL_HEADER, BlockList);
		ListCurrent = ListCurrent->Next;

		BlockEnd = POOL_BLOCK_END(BlockHeader);

		BlockHeaderNextTemp = BlockEnd - BlockHeader->BlockSizeUnused;
		POOL_ASSERT( !(BlockHeaderNextTemp & (POOL_HEADER_ALIGNMENT - 1)) );

		BlockStartUnaligned = (BlockHeaderNextTemp + sizeof(POOL_HEADER) + POOL_HEADER_ALIGNMENT - 1) & ~(POOL_HEADER_ALIGNMENT - 1);
		BlockStartAligned = (BlockStartUnaligned + Alignment - 1) & ~(Alignment - 1);

		if (BlockStartAligned + AlignedSize > BlockEnd)
			continue;

		BlockHeaderNext = BlockStartAligned - sizeof(POOL_HEADER);
		BlockSizeUnused = BlockEnd - (BlockStartAligned + AlignedSize);

		// Create the new header.
		// HHHH|XXXXRUUUUUUUUUUUUUUUUU -> HHHH|XXXXRUUHHHH|XXXXXXXXRUU
		BlockHeader2 = (PPOOL_HEADER)BlockHeaderNext;
		if (BlockHeader->BlockList.Next != ListHead)
			BlockHeader3 = CONTAINING_RECORD(BlockHeader->BlockList.Next, POOL_HEADER, BlockList);

		MiInitializePoolHeader(BlockHeader2, Tag, Size, (U32)(AlignedSize - Size), BlockSizeUnused, AlignmentShift);
		InitializeDListHead(&BlockHeader2->BlockList);
		InsertDListAfter(&BlockHeader->BlockList, &BlockHeader2->BlockList);

		// Set previous header.
		BlockHeader->BlockSizeUnused = BlockHeaderNext
			- ((UPTR)BlockHeader + sizeof(POOL_HEADER) + BlockHeader->BlockSize + BlockHeader->BlockSizeReserved);

		// Update the checksum.
		MiUpdatePoolHeaderChecksum(BlockHeader);
		MiUpdatePoolHeaderChecksum(BlockHeader2);

		if (BlockHeader3)
			MiUpdatePoolHeaderChecksum(BlockHeader3);

		if (1)
		{
			PDLIST_ENTRY head = &BlockList->BlockListHead;
			PDLIST_ENTRY curr = head->Next;
			while (curr != head)
			{
				PPOOL_HEADER hdr = CONTAINING_RECORD(curr, POOL_HEADER, BlockList);
				if (MiPoolChecksumMismatch(hdr))
				{
					printf("CHECKSUM MISMATCH AT BLOCK HEADER 0x%llx\n", (U64)hdr);
					POOL_ASSERT(FALSE);
				}
				curr = curr->Next;
			}
		}


		// Set return block address.
		BlockAddress = BlockStartAligned;

		break;
	}

	KeReleaseSpinLock(&BlockList->Lock);

	return (VOID *)BlockAddress;
}

VOID
KERNELAPI
MmFreePool(
	IN UPTR Address)
{
	PPOOL_BLOCK_LIST BlockList;
	PDLIST_ENTRY ListHead;
	PDLIST_ENTRY ListCurrent;
	PPOOL_HEADER BlockHeaderCurrent;

	PPOOL_HEADER BlockHeaderPrev = NULL;
	PPOOL_HEADER BlockHeaderNext = NULL;
	PPOOL_HEADER BlockHeaderUpdate = NULL;

	PPOOL_HEADER BlockHeaderTemp;

	UPTR BlockSizeUnusedIncrement;

	BlockList = MiLookupPoolByAddress(Address);
	POOL_ASSERT(BlockList != NULL);

	// Pool header must be in the same pool area
	BlockHeaderCurrent = (PPOOL_HEADER)((UPTR)Address - sizeof(POOL_HEADER));
	POOL_ASSERT(MiLookupPoolByAddress((UPTR)BlockHeaderCurrent) == BlockList);

	ListHead = &BlockList->BlockListHead;

	KeAcquireSpinLock(&BlockList->Lock);

	//
	// Verify the header checksum.
	//

	POOL_ASSERT( !MiPoolChecksumMismatch(BlockHeaderCurrent) );

	//
	// Merge the block.
	//
	// 1. Find first non-freed block from current block by searching forward/backward.
	//    we'll call forward block = FB, backward block = BB.
	// 2. If FB not found, first block of pool area becomes FB.
	// 3. BB = BB.Prev. (If BB exists)
	//    BB = current block (If BB not exists)
	// 4. Unlink the blocks FB and BB, including all blocks between them.
	// 5. FB.UnusedSize += BLOCK_END(BB) - BLOCK_END(FB)
	//
	// <- forward                                             backward ->
	// HHH|XXXXXRU HHH|RUUUUU HHH|XXXXUUU HHH|UU HHH|RRUUUUU HHH|XXXXUUUU
	// [ target1 ]            [ current ]        [ target2 ]
	//
	// HHH|XXXXXRU UUU UUUUUU UUU UUUUUUU UUU UU UUU UUUUUUU HHH|XXXXUUUU
	// [ target1                                           ]
	//

	ListCurrent = BlockHeaderCurrent->BlockList.Prev;
	while (ListCurrent != ListHead)
	{
		BlockHeaderPrev = CONTAINING_RECORD(ListCurrent, POOL_HEADER, BlockList);
		ListCurrent = ListCurrent->Prev;

		if (BlockHeaderPrev->BlockSize)
			break;
	}

	ListCurrent = BlockHeaderCurrent->BlockList.Next;
	while (ListCurrent != ListHead)
	{
		BlockHeaderNext = CONTAINING_RECORD(ListCurrent, POOL_HEADER, BlockList);
		ListCurrent = ListCurrent->Next;

		if (BlockHeaderNext->BlockSize)
			break;
	}


	if (!BlockHeaderPrev)
	{
		POOL_ASSERT(ListHead != ListHead->Next);
		BlockHeaderPrev = CONTAINING_RECORD(ListHead->Next, POOL_HEADER, BlockList);
	}

	if (!BlockHeaderNext)
	{
		BlockHeaderNext = BlockHeaderCurrent;
	}
	else
	{
		BlockHeaderUpdate = BlockHeaderNext;
		BlockHeaderNext = CONTAINING_RECORD(BlockHeaderNext->BlockList.Prev, POOL_HEADER, BlockList);
	}

	// Calculate the freed size before unlinking.
	BlockSizeUnusedIncrement = POOL_BLOCK_END(BlockHeaderNext) - POOL_BLOCK_END(BlockHeaderPrev);

	// Unlink the blocks.
	ListCurrent = BlockHeaderPrev->BlockList.Next;
	do
	{
		SIZE_T BlockRangeTemp;

		BlockHeaderTemp = CONTAINING_RECORD(ListCurrent, POOL_HEADER, BlockList);
		BlockRangeTemp = POOL_BLOCK_END(BlockHeaderTemp) - (UPTR)BlockHeaderTemp;
		ListCurrent = ListCurrent->Next;

		RemoveDListEntry(&BlockHeaderTemp->BlockList);

		// fill the magic number.
		memset((VOID *)BlockHeaderTemp, 0xee, BlockRangeTemp);
	} while (BlockHeaderTemp != BlockHeaderNext);

	// Add the freed block size.
	BlockHeaderPrev->BlockSizeUnused += BlockSizeUnusedIncrement;

	//
	// Update the checksum of FB and BB.
	//

	MiUpdatePoolHeaderChecksum(BlockHeaderPrev);

	if (BlockHeaderUpdate)
		MiUpdatePoolHeaderChecksum(BlockHeaderUpdate);

	KeReleaseSpinLock(&BlockList->Lock);
}

VOID
KERNELAPI
MmEnumeratePoolBlockList(
	IN PPOOL_BLOCK_LIST BlockList)
{
	PDLIST_ENTRY ListHead;
	PDLIST_ENTRY ListCurrent;

	KeAcquireSpinLock(&BlockList->Lock);

	ListHead = &BlockList->BlockListHead;
	ListCurrent = ListHead->Next;

	while (ListCurrent != ListHead)
	{
		PPOOL_HEADER BlockHeader = CONTAINING_RECORD(ListCurrent, POOL_HEADER, BlockList);
		UPTR BlockStart = (UPTR)(BlockHeader + 1);

		if (MiPoolChecksumMismatch(BlockHeader))
		{
			printf("CHECKSUM MISMATCH AT BLOCK HEADER 0x%llx\n", (U64)BlockHeader);
		}

		ListCurrent = ListCurrent->Next;

		printf("Block 0x%llx, BlockSize 0x%llx, BlockSizeReserved 0x%x, BlockSizeFree 0x%llx\n",
			BlockStart, BlockHeader->BlockSize, BlockHeader->BlockSizeReserved, BlockHeader->BlockSizeUnused);
	}

	KeReleaseSpinLock(&BlockList->Lock);
}

VOID
KERNELAPI
PoolBlockTest(
	VOID)
{
	UPTR BlockSize = 0x1000000;
	UPTR BlockAddress = (UPTR)VirtualAlloc(NULL, BlockSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	UPTR Blocks[256];
	U32 Count;
	U32 i;

	if (!BlockAddress)
		return;

	MiInitializePoolBlockList(&MiPoolList[PoolTypeNonPagedPreInit], BlockAddress, BlockSize, 0);

	printf("\nBlock Initialization Test:\n");
	MmEnumeratePoolBlockList(&MiPoolList[PoolTypeNonPagedPreInit]);


	printf("\nBlock Allocation Test:\n");
	for (i = 0; i < ARRAYSIZE(Blocks); i++)
	{
		SIZE_T AllocationSize = rand() * ((rand() >> 12) + 1) + 0x10;
		Blocks[i] = (UPTR)MmAllocatePool(PoolTypeNonPagedPreInit, AllocationSize, 0x1000, 0x12345678);

		printf("Blocks[%2d] = ", i);

		if (Blocks[i])
			printf("0x%llx\n", Blocks[i]);
		else
			printf("Failed to allocate block, size 0x%llx\n", AllocationSize);
	}

	MmEnumeratePoolBlockList(&MiPoolList[PoolTypeNonPagedPreInit]);

#if 1
	printf("\nBlock Free Test (Non-Sequential):\n");
	for (;;)
	{
		U32 IndexStart = rand() % ARRAYSIZE(Blocks);
		BOOLEAN Found = FALSE;

		for (i = 0; i < ARRAYSIZE(Blocks); i++)
		{
			U32 Index = (i + IndexStart) % ARRAYSIZE(Blocks);
			if (Blocks[Index])
			{
				printf("Freeing Blocks[%2d] 0x%llx\n", Index, Blocks[Index]);
				MmFreePool(Blocks[Index]);
				Blocks[Index] = 0;
				Found = TRUE;
				break;
			}
		}

		if (!Found)
			break;
	}
#else
	printf("\nBlock Free Test (Sequential):\n");
	for (i = 0; i < ARRAYSIZE(Blocks); i++)
	{
		if (Blocks[i])
		{
			printf("Freeing Blocks[%2d] 0x%llx\n", i, Blocks[i]);
			MmFreePool(Blocks[i]);
		}
	}
#endif
	MmEnumeratePoolBlockList(&MiPoolList[PoolTypeNonPagedPreInit]);

	printf("\nBlock Allocation-Free Test:\n");
	memset(Blocks, 0, sizeof(Blocks));
	for (Count = 0; ; )
	{
		U32 Index = rand() % ARRAYSIZE(Blocks);
		BOOLEAN FreeMode = !!((__rdtsc() + rand()) & 1);

		if (GetAsyncKeyState(VK_RETURN) < 0)
		{
			printf(" *** Canceled by user\n");
			break;
		}

		if (!!Blocks[Index] != FreeMode)
		{
			if ((FreeMode && Count == 0) ||
				(!FreeMode && Count == ARRAYSIZE(Blocks)))
			{
				FreeMode = !FreeMode;
			}

			continue;
		}

		if (!!Blocks[Index] == FreeMode)
		{
			if (FreeMode)
			{
				printf("Freeing Blocks[%2d] 0x%llx\n", Index, Blocks[Index]);

				MmFreePool(Blocks[Index]);
				Blocks[Index] = 0;
				Count--;
			}
			else
			{
				printf("Blocks[%2d] = ", Index);
				Blocks[Index] = (UPTR)MmAllocatePool(PoolTypeNonPagedPreInit, rand() + 1, 1 << (rand() & 0xf), 'TSET');

				if (Blocks[Index])
				{
					printf("0x%llx\n", Blocks[Index]);
					Count++;
				}
				else
					printf("Failed to allocate block\n");
			}
		}
	}

	MmEnumeratePoolBlockList(&MiPoolList[PoolTypeNonPagedPreInit]);
	printf("\n");

	printf("\nBlock Free Test:\n");
	for (i = 0; i < ARRAYSIZE(Blocks); i++)
	{
		if (Blocks[i])
		{
			printf("Freeing Blocks[%2d] 0x%llx\n", i, Blocks[i]);
			MmFreePool(Blocks[i]);
		}
	}

	MmEnumeratePoolBlockList(&MiPoolList[PoolTypeNonPagedPreInit]);

	printf("\nEnd of Test\n");
}

VOID
WINAPI
PoolAllocFreeThreadProc(
	IN PVOID Context)
{
	UPTR Blocks[64];
	U32 Count;

	memset(Blocks, 0, sizeof(Blocks));
	for (Count = 0; ; )
	{
		U32 Index = rand() % ARRAYSIZE(Blocks);
		BOOLEAN FreeMode = !!((__rdtsc() + rand()) & 1);

		if (GetAsyncKeyState(VK_RETURN) < 0)
		{
//			printf(" *** Canceled by user\n");
			break;
		}

		if (!!Blocks[Index] != FreeMode)
		{
			if ((FreeMode && Count == 0) ||
				(!FreeMode && Count == ARRAYSIZE(Blocks)))
			{
				FreeMode = !FreeMode;
			}

			continue;
		}

		if (!!Blocks[Index] == FreeMode)
		{
			if (FreeMode)
			{
//				printf("Freeing Blocks[%2d] 0x%llx\n", Index, Blocks[Index]);

				MmFreePool(Blocks[Index]);
				Blocks[Index] = 0;
				Count--;
			}
			else
			{
//				printf("Blocks[%2d] = ", Index);
				Blocks[Index] = (UPTR)MmAllocatePool(PoolTypeNonPagedPreInit, rand() + 1, 1 << (rand() & 0xf), 'TSET');

				if (Blocks[Index])
				{
//					printf("0x%llx\n", Blocks[Index]);
					Count++;
				}
//				else
//					printf("Failed to allocate block\n");
			}
		}
	}
}

VOID
KERNELAPI
PoolMPTest(
	VOID)
{
	UPTR BlockSize = 0x1000000;
	UPTR BlockAddress = (UPTR)VirtualAlloc(NULL, BlockSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	U32 i;

	HANDLE ThreadHandle[4];

	if (!BlockAddress)
		return;

	MiInitializePoolBlockList(&MiPoolList[PoolTypeNonPagedPreInit], BlockAddress, BlockSize, 0);

	printf("\nBlock Initialization Test:\n");
	MmEnumeratePoolBlockList(&MiPoolList[PoolTypeNonPagedPreInit]);


	printf("\nBlock Allocation-Free Test (%d Threads):\n", ARRAYSIZE(ThreadHandle));
	for (i = 0; i < ARRAYSIZE(ThreadHandle); i++)
		ThreadHandle[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PoolAllocFreeThreadProc, NULL, 0, NULL);

	WaitForMultipleObjects(ARRAYSIZE(ThreadHandle), ThreadHandle, TRUE, INFINITE);
	MmEnumeratePoolBlockList(&MiPoolList[PoolTypeNonPagedPreInit]);
}

int main()
{
//	PoolBlockTest();
	PoolMPTest();

	return 0;
}

