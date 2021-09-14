
#pragma once

#include <base/base.h>
#include <base/gerror.h>
#include <misc/misc.h>


//
// Macros for page.
//

#define	PAGE_SHIFT						12
#define PAGE_SIZE						(1 << PAGE_SHIFT)
#define	PAGE_MASK						(PAGE_SIZE - 1)
#define	ROUNDUP_TO_PAGE_SIZE(_x)		( ((_x) + PAGE_MASK) & ~PAGE_MASK )
#define	ROUNDUP_TO_PAGES(_x)			( ((_x) + PAGE_MASK) >> PAGE_SHIFT )
#define	ROUNTDOWN_TO_PAGE_SIZE(_x)		( (_x) & ~PAGE_MASK )


//
// Physical/Virtual address descriptor.
//

typedef struct _ADDRESS_RANGE
{
    // Range = [Start, End)
	U64 Start;		// Starting address.
	U64 End;		// Ending address.
					// Size = End - start
} ADDRESS_RANGE;

typedef struct _ADDRESS
{
	ADDRESS_RANGE Range;	// Address range.
	U32 Type;				// Address type.
} ADDRESS;

typedef struct _MMXAD
{
	RS_AVL_NODE AvlNode;	// AVL node header.
	DLIST_ENTRY Links;		// Node links.
	U32 ReferenceCount;		// Reference count.

	ADDRESS Address;		// Address.
    U32 PrevType;           //!< Previous address type.
} MMXAD;

#define	MMXAD_MAX_SIZE_LEVELS					(64 - PAGE_SHIFT + 1) // (64 - PAGE_SHIFT) = 52 levels
C_ASSERT(MMXAD_MAX_SIZE_LEVELS > 1);

typedef struct _MMXAD_TREE
{
	RS_AVL_TREE Tree;		// AVL tree header.
	PVOID Lock;				// Tree lock.

	DLIST_ENTRY SizeLinks[MMXAD_MAX_SIZE_LEVELS]; // Size ordered links.
} MMXAD_TREE;



BOOLEAN
MmXadDebugTraverse(
    IN PVOID CallerContext,
    IN MMXAD *Xad);


BOOLEAN
MmXadIsAddressRangeValid(
	IN ADDRESS_RANGE *AddressRange);

BOOLEAN
MmXadIsInAddressRange(
	IN ADDRESS_RANGE *SourceAddressRange,
	IN ADDRESS_RANGE *TestAddressRange);

VOID
MmXadAcquireLock(
    IN MMXAD_TREE *XadTree);

VOID
MmXadReleaseLock(
    IN MMXAD_TREE *XadTree);

BOOLEAN
MmXadInitializeTree(
	OUT MMXAD_TREE *XadTree,
	IN PVOID CallerContext);

ESTATUS
MmXadInsertAddress(
	IN MMXAD_TREE *XadTree,
	OUT MMXAD **Xad,
	IN ADDRESS *Address);

ESTATUS
MmXadDeleteAddress(
	IN MMXAD_TREE *XadTree,
	IN ADDRESS *Address);

ESTATUS
MmXadReclaimAddress(
	IN MMXAD_TREE *XadTree,
	IN MMXAD *Xad,
	IN ADDRESS *AddressHint,
	IN ADDRESS *AddressReclaim);



#define	XAD_LAF_ADDRESS				0x01 //!< Uses AddressHint.
#define	XAD_LAF_SIZE				0x02 //!< Uses SizeHint.
#define	XAD_LAF_TYPE				0x04 //!< Uses TypeHint.

ESTATUS
MmXadLookupAddress(
	IN MMXAD_TREE *XadTree,
	OUT MMXAD **Xad,
	IN U64 AddressHint,
	IN U64 SizeHint,
	IN U32 TypeHint,
	IN U32 Options);

