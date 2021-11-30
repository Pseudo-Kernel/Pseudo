
#pragma once

#include <base/base.h>

#define BINARY_TREE_NODE_TO_AVL_NODE(_n)            ((RS_AVL_NODE *)(_n))

typedef union _RS_BINARY_TREE_LINK          RS_BINARY_TREE_LINK;
typedef struct _RS_BINARY_TREE              RS_BINARY_TREE;

typedef
BOOLEAN
(KERNELAPI *PRS_BINARY_TREE_TRAVERSE)(
    IN PVOID CallerContext,
    IN RS_BINARY_TREE_LINK *Node);


typedef
RS_BINARY_TREE_LINK *
(KERNELAPI *PRS_BINARY_TREE_ALLOCATE_NODE)(
    IN PVOID CallerContext);

typedef
VOID
(KERNELAPI *PRS_BINARY_TREE_DELETE_NODE)(
    IN PVOID CallerContext,
    IN RS_BINARY_TREE_LINK *Node);

typedef
INT
(KERNELAPI *PRS_BINARY_TREE_COMPARE_KEY)(
    IN PVOID CallerContext,
    IN PVOID Key1,
    IN PVOID Key2);

typedef
PVOID
(KERNELAPI *PRS_BINARY_TREE_GET_NODE_KEY)(
    IN PVOID CallerContext,
    IN RS_BINARY_TREE_LINK *Node);

typedef
VOID
(KERNELAPI *PRS_BINARY_TREE_SET_NODE_KEY)(
    IN PVOID CallerContext,
    IN RS_BINARY_TREE_LINK *Node,
    IN PVOID Key);

typedef
SIZE_T
(KERNELAPI *PRS_BINARY_TREE_KEY_TO_STRING)(
    IN PVOID CallerContext,
    IN PVOID Key, 
    OUT CHAR *Buffer, 
    IN SIZE_T BufferLength);



typedef union _RS_BINARY_TREE_LINK
{
    struct
    {
        union _RS_BINARY_TREE_LINK *Parent;
        union _RS_BINARY_TREE_LINK *LeftChild;
        union _RS_BINARY_TREE_LINK *RightChild;
    };

//  for test
//  struct
//  {
//      struct _RS_AVL_NODE *P;
//      struct _RS_AVL_NODE *L;
//      struct _RS_AVL_NODE *R;
//  };
} RS_BINARY_TREE_LINK;

typedef struct _RS_AVL_NODE
{
    RS_BINARY_TREE_LINK Links;
    S8 Height;
} RS_AVL_NODE;

typedef struct _RS_BINARY_TREE_OPERATIONS
{
    PRS_BINARY_TREE_ALLOCATE_NODE AllocateNode;
    PRS_BINARY_TREE_DELETE_NODE DeleteNode;
    PRS_BINARY_TREE_GET_NODE_KEY GetKey;
    PRS_BINARY_TREE_SET_NODE_KEY SetKey;
    PRS_BINARY_TREE_COMPARE_KEY CompareKey;
    PRS_BINARY_TREE_KEY_TO_STRING KeyToString;
} RS_BINARY_TREE_OPERATIONS;

typedef struct _RS_BINARY_TREE
{
    RS_BINARY_TREE_LINK *Root;
    RS_BINARY_TREE_OPERATIONS Operations;
    PVOID CallerContext;
    U32 NodeCount;
} RS_BINARY_TREE;


typedef struct _RS_BINARY_TREE          RS_AVL_TREE;


#define RS_BT_UNLINK_PARENT         0x00000001
#define RS_BT_UNLINK_LEFT_CHILD     0x00000002
#define RS_BT_UNLINK_RIGHT_CHILD    0x00000004

#define RS_BT_UNLINK_ALL            (RS_BT_UNLINK_PARENT | RS_BT_UNLINK_CHILDREN)
#define RS_BT_UNLINK_CHILDREN       (RS_BT_UNLINK_LEFT_CHILD | RS_BT_UNLINK_RIGHT_CHILD)

#define RS_BT_LOOKUP_EQUAL          0
#define RS_BT_LOOKUP_NEAREST_ABOVE  1
#define RS_BT_LOOKUP_NEAREST_BELOW  2

#define RS_BT_LOOKUP_FLAG_BITS      RS_BT_LOOKUP_FLAG_EQUAL
#define RS_BT_LOOKUP_FLAG_EQUAL     0x80000000



BOOLEAN
KERNELAPI
RsAvlRebalance(
    IN RS_BINARY_TREE *Tree,
    IN RS_AVL_NODE *Unbalanced,
    OUT RS_AVL_NODE **NextUnbalanced);

VOID
VARCALL
RsBtTrace(
    IN PSZ Format,
    ...);

VOID
KERNELAPI
RsBtTraverse(
    IN RS_BINARY_TREE *Tree,
    IN RS_BINARY_TREE_LINK *Next,
    IN PRS_BINARY_TREE_TRAVERSE Traverse);

VOID
KERNELAPI
RsBtInitialize(
    IN RS_BINARY_TREE *Tree,
    IN RS_BINARY_TREE_OPERATIONS *Operations,
    IN PVOID CallerContext);

RS_BINARY_TREE_LINK *
KERNELAPI
RsBtGetChildNode(
    IN RS_BINARY_TREE_LINK *Node,
    IN INT NodeIndex);

RS_BINARY_TREE_LINK *
KERNELAPI
RsBtSetChildNode(
    IN RS_BINARY_TREE_LINK *Node,
    IN RS_BINARY_TREE_LINK *NewChild,
    IN INT NodeIndex);

INT
KERNELAPI
RsBtNodeIndexFromChild(
    IN RS_BINARY_TREE_LINK *Child);

RS_BINARY_TREE_LINK *
KERNELAPI
RsBtGetInorderPredecessor(
    IN RS_BINARY_TREE_LINK *Node);

RS_BINARY_TREE_LINK *
KERNELAPI
RsBtGetInorderSuccessor(
    IN RS_BINARY_TREE_LINK *Node);

VOID
KERNELAPI
RsBtSetRoot(
    IN RS_BINARY_TREE *Tree,
    IN RS_BINARY_TREE_LINK *Root);

BOOLEAN
KERNELAPI
RsBtTreeContainsNode(
    IN RS_BINARY_TREE *Tree,
    IN RS_BINARY_TREE_LINK *Node);

VOID
KERNELAPI
RsBtLinkNode(
    IN RS_BINARY_TREE_LINK *Parent,
    IN RS_BINARY_TREE_LINK *NewChild,
    IN INT NodeIndex);

VOID
KERNELAPI
RsBtUnlinkNode(
    IN RS_BINARY_TREE_LINK *Node,
    IN UINT UnlinkFlags,
    OUT RS_BINARY_TREE_LINK **PreviousParent,
    OUT RS_BINARY_TREE_LINK **PreviousLeftChild,
    OUT RS_BINARY_TREE_LINK **PreviousRightChild);

BOOLEAN
KERNELAPI
RsBtInsertLeaf(
    IN RS_BINARY_TREE *Tree,
    IN RS_BINARY_TREE_LINK *LeafNode);

BOOLEAN
KERNELAPI
RsBtRemoveNode(
    IN RS_BINARY_TREE *Tree,
    IN RS_BINARY_TREE_LINK *Node,
    OUT RS_BINARY_TREE_LINK **HeightUpdatePoint);

VOID
KERNELAPI
RsBtDeleteNode(
    IN RS_BINARY_TREE *Tree,
    IN RS_BINARY_TREE_LINK *Node);

BOOLEAN
KERNELAPI
RsBtLookup(
    IN RS_BINARY_TREE *Tree,
    IN PVOID Key,
    OUT RS_BINARY_TREE_LINK **LookupNode);

BOOLEAN
KERNELAPI
RsBtLookup2(
    IN RS_BINARY_TREE *Tree,
    IN PVOID Key,
    IN U32 Lookup,
    OUT RS_BINARY_TREE_LINK **LookupNode);

BOOLEAN
KERNELAPI
RsBtInsert(
    IN RS_BINARY_TREE *Tree,
    IN PVOID Key);

S8
KERNELAPI
RsAvlGetBalance(
    IN RS_AVL_NODE *Node);

S8
KERNELAPI
RsAvlUpdateHeight(
    IN RS_AVL_NODE *Node);

VOID
KERNELAPI
RsAvlUpdateHeightAncestors(
    IN RS_AVL_NODE *Node,
    IN BOOLEAN StopIfUnbalancedFound,
    OUT RS_AVL_NODE **FirstUnbalanced);

VOID
KERNELAPI
RsAvlRotate(
    IN RS_BINARY_TREE *Tree,
    IN RS_AVL_NODE *Target,
    IN BOOLEAN RotateRight);

BOOLEAN
KERNELAPI
RsAvlRebalance(
    IN RS_BINARY_TREE *Tree,
    IN RS_AVL_NODE *Unbalanced,
    OUT RS_AVL_NODE **NextUnbalanced);

BOOLEAN
KERNELAPI
RsAvlInsert(
    IN RS_BINARY_TREE *Tree,
    IN PVOID Key,
    OUT RS_BINARY_TREE_LINK **Inserted);

BOOLEAN
KERNELAPI
RsAvlDelete(
    IN RS_BINARY_TREE *Tree,
    IN RS_AVL_NODE *Node);

BOOLEAN
KERNELAPI
RsAvlDeleteByKey(
    IN RS_BINARY_TREE *Tree,
    IN PVOID Key);


