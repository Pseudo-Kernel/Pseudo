
/**
 * @file bintree.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements binary tree.
 * @version 0.1
 * @date 2021-09-10
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <base/base.h>

/**
 * @brief Prints the formatted string.
 * 
 * @param [in] Format       Printf-like formst string.
 * @param [in] ...          Arguments.
 *
 * @return None.
 */
VOID
VARCALL
RsBtTrace(
    IN PSZ Format,
    ...)
{
    va_list List;
    CHAR8 Buffer[512];
    SIZE_T Length;

	va_start(List, Format);
	Length = ClStrFormatU8V(Buffer, COUNTOF(Buffer), Format, List);
	va_end(List);

//    DbgTraceN(TraceLevelDebug, Buffer, Length);
}

/**
 * @brief Traverses and prints the binary tree.
 * 
 * @param [in] Tree         Tree structure.
 * @param [in] Next         Starting node.
 * @param [in] Traverse     Traverse function.
 *
 * @return None.
 */
VOID
KERNELAPI
RsBtTraverse(
    IN RS_BINARY_TREE *Tree,
    IN RS_BINARY_TREE_LINK *Next,
    IN PRS_BINARY_TREE_TRAVERSE Traverse)
{
    if (!Next)
    {
        if (Tree->Root)
            RsBtTraverse(Tree, Tree->Root, Traverse);
    }
    else
    {
        if (Next->LeftChild)
            RsBtTraverse(Tree, Next->LeftChild, Traverse);

        Traverse(Tree->CallerContext, Next);

        if (Next->RightChild)
            RsBtTraverse(Tree, Next->RightChild, Traverse);
    }
}

/**
 * @brief Initializes the binary tree.
 * 
 * @param [in] Tree             Tree structure.
 * @param [in] Operations       Pointer to RS_BINARY_TREE_OPERATIONS structure which contains operation callbacks.
 * @param [in] CallerContext    Caller-supplied context pointer which will be passed to operation callbacks.
 *
 * @return None.
 */
VOID
KERNELAPI
RsBtInitialize(
    IN RS_BINARY_TREE *Tree,
    IN RS_BINARY_TREE_OPERATIONS *Operations,
    IN PVOID CallerContext)
{
    memset(Tree, 0, sizeof(*Tree));

    Tree->CallerContext = CallerContext;
    Tree->Operations = *Operations;
    Tree->Root = NULL;
}

/**
 * @brief Gets the child node of given node.
 * 
 * @param [in] Node             Node.
 * @param [in] NodeIndex        0 for left child, 1 for right child.
 *
 * @return Child node.
 */
inline
RS_BINARY_TREE_LINK *
KERNELAPI
RsBtGetChildNode(
    IN RS_BINARY_TREE_LINK *Node,
    IN INT NodeIndex)
{
    if (NodeIndex == 0)
        return Node->LeftChild;
    else if (NodeIndex == 1)
        return Node->RightChild;

    DASSERT(FALSE);

    return NULL;
}

/**
 * @brief Sets the child node of given node.
 * 
 * @param [in] Node             Node.
 * @param [in] NewChild         New child.
 * @param [in] NodeIndex        0 for left child, 1 for right child.
 *
 * @return Previous child node.
 */
inline
RS_BINARY_TREE_LINK *
KERNELAPI
RsBtSetChildNode(
    IN RS_BINARY_TREE_LINK *Node,
    IN RS_BINARY_TREE_LINK *NewChild,
    IN INT NodeIndex)
{
    RS_BINARY_TREE_LINK *PreviousNode = NULL;

    if (NodeIndex == 0)
    {
        PreviousNode = Node->LeftChild;
        Node->LeftChild = NewChild;
    }
    else if (NodeIndex == 1)
    {
        PreviousNode = Node->RightChild;
        Node->RightChild = NewChild;
    }
    else
    {
        DASSERT(FALSE);
    }

    return PreviousNode;
}

/**
 * @brief Gets the node index from child node.
 * 
 * @param [in] Child            Child node.
 *
 * @return 0 if child is left child, 1 if right child.
 *         -1 Otherwise (Child has no parent).
 */
inline
INT
KERNELAPI
RsBtNodeIndexFromChild(
    IN RS_BINARY_TREE_LINK *Child)
{
    RS_BINARY_TREE_LINK *Parent = Child->Parent;

    if (Parent)
    {
        if (Parent->LeftChild == Child)
            return 0; // Left child
        else if (Parent->RightChild == Child)
            return 1; // Right child

        DASSERT(FALSE);
    }
    else
    {
        // Child is root
        // Do nothing
    }

    return -1;
}

/**
 * @brief Gets the inorder predecessor of given node.
 * 
 * @param [in] Node             Node.
 *
 * @return Inorder predecessor node.
 */
inline
RS_BINARY_TREE_LINK *
KERNELAPI
RsBtGetInorderPredecessor(
    IN RS_BINARY_TREE_LINK *Node)
{
    RS_BINARY_TREE_LINK *Current = Node->LeftChild;
    RS_BINARY_TREE_LINK *Predecessor = NULL;

    while (Current)
    {
        Predecessor = Current;
        Current = Current->RightChild;
    }

    return Predecessor;
}

/**
 * @brief Gets the inorder successor of given node.
 * 
 * @param [in] Node             Node.
 *
 * @return Inorder successor node.
 */
inline
RS_BINARY_TREE_LINK *
KERNELAPI
RsBtGetInorderSuccessor(
    IN RS_BINARY_TREE_LINK *Node)
{
    RS_BINARY_TREE_LINK *Current = Node->RightChild;
    RS_BINARY_TREE_LINK *Successor = NULL;

    while (Current)
    {
        Successor = Current;
        Current = Current->LeftChild;
    }

    return Successor;
}

/**
 * @brief Sets the root.
 * 
 * @param [in] Tree         Tree structure.
 * @param [in] Root         New root.
 *
 * @return None.
 */
inline
VOID
KERNELAPI
RsBtSetRoot(
    IN RS_BINARY_TREE *Tree,
    IN RS_BINARY_TREE_LINK *Root)
{
    Tree->Root = Root;

    if (Root)
        Root->Parent = NULL;
}

/**
 * @brief Checks whether the tree contains given node.
 * 
 * @param [in] Tree         Tree structure.
 * @param [in] Node         Node.
 *
 * @return TRUE if tree contains given node. FALSE otherwise,
 */
inline
BOOLEAN
KERNELAPI
RsBtTreeContainsNode(
    IN RS_BINARY_TREE *Tree,
    IN RS_BINARY_TREE_LINK *Node)
{
    RS_BINARY_TREE_LINK *Current = Node;
    RS_BINARY_TREE_LINK *Root = NULL;

    while (Current)
    {
        Root = Current;
        Current = Current->Parent;
    }

    if (Tree->Root != Root)
        return FALSE;

    return TRUE;
}

/**
 * @brief Links child node to parent node.
 * 
 * @param [in] Parent       Parent node.
 * @param [in] NewChild     New child node.
 * @param [in] NodeIndex    if 0, NewChild becomes left child.
 *                          Otherwise, NewChild becomes right child.
 *
 * @return None.
 */
VOID
KERNELAPI
RsBtLinkNode(
    IN RS_BINARY_TREE_LINK *Parent,
    IN RS_BINARY_TREE_LINK *NewChild,
    IN INT NodeIndex)
{
    if (Parent)
    {
        RsBtSetChildNode(Parent, NewChild, NodeIndex);
    }

    if (NewChild)
    {
        NewChild->Parent = Parent;
    }
}

/**
 * @brief Unlinks parent or child from given node.
 * 
 * @param [in] Node                 Node.
 * @param [in] UnlinkFlags          Unlink flags. Combination of RS_BT_UNLINK_XXX.\n
 *                                  RS_BT_UNLINK_PARENT - Unlinks parent.\n
 *                                  RS_BT_UNLINK_LEFT_CHILD - Unlinks left child.\n
 *                                  RS_BT_UNLINK_RIGHT_CHILD - Unlinks right child.\n
 * @param [out] PreviousParent      Pointer to caller-supplied variable to receive previous parent.
 * @param [out] PreviousLeftChild   Pointer to caller-supplied variable to receive previous left child.
 * @param [out] PreviousRightChild  Pointer to caller-supplied variable to receive previous right child.
 *
 * @return None.
 */
VOID
KERNELAPI
RsBtUnlinkNode(
    IN RS_BINARY_TREE_LINK *Node,
    IN UINT UnlinkFlags,
    OUT RS_BINARY_TREE_LINK **PreviousParent,
    OUT RS_BINARY_TREE_LINK **PreviousLeftChild,
    OUT RS_BINARY_TREE_LINK **PreviousRightChild)
{
    if (UnlinkFlags & RS_BT_UNLINK_PARENT)
    {
        RS_BINARY_TREE_LINK *Parent = Node->Parent;

        if (Parent)
        {
            if (Parent->LeftChild == Node)
                Parent->LeftChild = NULL;
            else if (Parent->RightChild == Node)
                Parent->RightChild = NULL;
            else
                DASSERT(FALSE);

            Node->Parent = NULL;
        }

        if (PreviousParent)
            *PreviousParent = Parent;
    }

    if (UnlinkFlags & RS_BT_UNLINK_LEFT_CHILD)
    {
        RS_BINARY_TREE_LINK *LeftChild = Node->LeftChild;
        if (LeftChild)
        {
            LeftChild->Parent = NULL;
            Node->LeftChild = NULL;
        }

        if (PreviousLeftChild)
            *PreviousLeftChild = LeftChild;
    }

    if (UnlinkFlags & RS_BT_UNLINK_RIGHT_CHILD)
    {
        RS_BINARY_TREE_LINK *RightChild = Node->RightChild;
        if (RightChild)
        {
            RightChild->Parent = NULL;
            Node->RightChild = NULL;
        }

        if (PreviousRightChild)
            *PreviousRightChild = RightChild;
    }
}

/**
 * @brief Checks whether the given node is a leaf node.
 * 
 * @param [in] Node                 Node.
 *
 * @return TRUE if node is a leaf node, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
RsBtIsLeaf(
    IN RS_BINARY_TREE_LINK *Node)
{
    if (Node->LeftChild || Node->RightChild)
        return FALSE;

    return TRUE;
}

/**
 * @brief Inserts leaf node to given tree.
 * 
 * @param [in] Tree                 Tree.
 * @param [in] LeafNode             Leaf node to insert.
 *
 * @return TRUE if insertion succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
RsBtInsertLeaf(
    IN RS_BINARY_TREE *Tree,
    IN RS_BINARY_TREE_LINK *LeafNode)
{
    if (!LeafNode)
        return FALSE;

    if (!RsBtIsLeaf(LeafNode))
        return FALSE;

    PVOID CapturedContext = Tree->CallerContext;
    PVOID InsertKey = Tree->Operations.GetKey(CapturedContext, LeafNode);

    CHAR KeyName[128];
    Tree->Operations.KeyToString(CapturedContext, InsertKey, KeyName, sizeof(KeyName));

    if (!Tree->Root)
    {
        Tree->Root = LeafNode;

        RsBtTrace("%s inserted as root.\n", KeyName);
    }
    else
    {
        for (RS_BINARY_TREE_LINK *Node = Tree->Root; Node;)
        {
            PVOID CompareKey = Tree->Operations.GetKey(CapturedContext, Node);

            // InsertKey < CompareKey : < 0
            // InsertKey overlaps CompareKey : = 0
            // InsertKey > CompareKey : > 0
            INT Result = Tree->Operations.CompareKey(CapturedContext, InsertKey, CompareKey);
            INT Index = -1;

            if (Result < 0) // InsertKey < CompareKey
                Index = 0;
            else if (Result > 0) // InsertKey > CompareKey
                Index = 1;
            else
                return FALSE; // collision detected

            RS_BINARY_TREE_LINK *ChildNode = RsBtGetChildNode(Node, Index);
            if (!ChildNode)
            {
                RsBtLinkNode(Node, LeafNode, Index);
                RsBtTrace("%s inserted.\n", KeyName);
                break;
            }

            Node = ChildNode;
        }
    }

    Tree->NodeCount++;

    return TRUE;
}

/**
 * @brief Removes node from tree.\n
 *        The node will not be deleted unless you call RsBtDeleteNode.
 * 
 * @param [in] Tree                 Tree.
 * @param [in] Node                 Node to remove.
 * @param [out] HeightUpdatePoint   Pointer to caller-supplied variable to receive node which height is changed.
 *
 * @return TRUE if removal succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
RsBtRemoveNode(
    IN RS_BINARY_TREE *Tree,
    IN RS_BINARY_TREE_LINK *Node,
    OUT RS_BINARY_TREE_LINK **HeightUpdatePoint)
{
    if (!Node)
        return FALSE;

    if (!Tree->Root)
    {
        // Tree is empty
        return FALSE;
    }

    if (!RsBtTreeContainsNode(Tree, Node))
    {
        // Node does not belong to tree
        return FALSE;
    }

    RS_BINARY_TREE_LINK *Parent = NULL;
    RS_BINARY_TREE_LINK *LeftChild = NULL;
    RS_BINARY_TREE_LINK *RightChild = NULL;
    RS_BINARY_TREE_LINK *Successor = RsBtGetInorderSuccessor(Node);

    RS_BINARY_TREE_LINK *UpdateNeeded = NULL;

    INT NodeIndex = RsBtNodeIndexFromChild(Node);

    // Unlink the target node.
    RsBtUnlinkNode(Node, RS_BT_UNLINK_ALL, &Parent, &LeftChild, &RightChild);

    if (!LeftChild && !RightChild)
    {
        // Case 1. Node does not have any child
        UpdateNeeded = Parent;

        if (!Parent)
        {
            RsBtSetRoot(Tree, NULL);
        }
    }
    else if ((LeftChild && !RightChild) || (!LeftChild && RightChild))
    {
        // Case 2. Node has one child
        RS_BINARY_TREE_LINK *Child = LeftChild;
        if (RightChild)
            Child = RightChild;

        DASSERT(Child);
        if (Parent)
        {
            RsBtLinkNode(Parent, Child, NodeIndex);
            UpdateNeeded = Parent;
        }
        else
        {
            RsBtSetRoot(Tree, Child);
        }
    }
    else
    {
        /*
          Case 3. Node has two children.
          i) s is not a child of S:
                 R
                 |
                 S   <--- S: Node to be deleted (Node)
               /   \
              T     U  
             / \   / \   
             ...  s  ...    <--- s: In-order successor (Successor).
                   \
                    c 
                   / \
                   ...
           
                 R
                 |
                 s   <--- S: Replaced by s.
               /   \
              T     U  
             / \   / \   
             ...  c  ...    <--- s: Replaced by c.
                 / \
                 ...
          
           ii) s is child of S:
                 R
                 |
                 S   <--- S: Node to be deleted
               /   \
              T     s  <--- s: In-order successor.
             / \     \   
             ...      c 
                     / \
                     ...
           
                 R
                 |
                 s   <--- S: Replaced by s.
               /   \
              T     c  <--- s: Replaced by c.
             / \   / \   
            ...  ...  ...
        */

        DASSERT(!Successor->LeftChild);

        if (Successor == RightChild)
        {
            // Special case: Successor is right child of deletion node.
            UpdateNeeded = Successor;
        }
        else
        {
            RS_BINARY_TREE_LINK *SuccessorParent = NULL;
            RS_BINARY_TREE_LINK *SuccessorChild = NULL;
            INT SuccessorNodeIndex = RsBtNodeIndexFromChild(Successor);

            // Unlink successor node.
            RsBtUnlinkNode(Successor, RS_BT_UNLINK_PARENT | RS_BT_UNLINK_RIGHT_CHILD,
                           &SuccessorParent, NULL, &SuccessorChild);

            DASSERT(SuccessorParent);

            if (SuccessorChild)
            {
                // Case 3-1. In-order successor has right child.
                RsBtLinkNode(SuccessorParent, SuccessorChild, SuccessorNodeIndex);
            }
            else
            {
                // Case 3-2. In-order successor has no child (leaf).
                // do nothing
            }

            // Take care of right child.
            RsBtLinkNode(Successor, RightChild, 1);

            UpdateNeeded = SuccessorParent;
        }

        RsBtLinkNode(Successor, LeftChild, 0);

        if (Parent)
        {
            RsBtLinkNode(Parent, Successor, NodeIndex);
        }
        else
        {
            RsBtSetRoot(Tree, Successor);
        }
    }

    Tree->NodeCount--;

    PVOID CapturedContext = Tree->CallerContext;
    PVOID Key = Tree->Operations.GetKey(CapturedContext, Node);

    char Buffer[128];
    Tree->Operations.KeyToString(CapturedContext, Key, Buffer, sizeof(Buffer) / sizeof(Buffer[0]));
    RsBtTrace("%s removed.\n", Buffer);

    if (HeightUpdatePoint)
        *HeightUpdatePoint = UpdateNeeded;

    return TRUE;
}

/**
 * @brief Deletes node from tree.
 * 
 * @param [in] Tree                 Tree.
 * @param [in] Node                 Node to delete.
 *
 * @return None.
 */
inline
VOID
KERNELAPI
RsBtDeleteNode(
    IN RS_BINARY_TREE *Tree,
    IN RS_BINARY_TREE_LINK *Node)
{
    DASSERT(!Node->LeftChild && !Node->RightChild && !Node->Parent);

    Tree->Operations.DeleteNode(Tree->CallerContext, Node);
}

/**
 * @brief Searches node by given key.
 * 
 * @param [in] Tree                 Tree.
 * @param [in] Key                  Node key.
 * @param [out] LookupNode          Pointer to caller-supplied variable to receive target node.
 *
 * @return TRUE if lookup succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
RsBtLookup(
    IN RS_BINARY_TREE *Tree,
    IN PVOID Key,
    OUT RS_BINARY_TREE_LINK **LookupNode)
{
    INT Index = 0;
    RS_BINARY_TREE_LINK *Node = Tree->Root;
    PVOID CapturedContext = Tree->CallerContext;

    while (Node)
    {
        INT CompareResult = Tree->Operations.CompareKey(
            CapturedContext, Key, Tree->Operations.GetKey(CapturedContext, Node));

        if (CompareResult == 0)
        {
            if (LookupNode)
                *LookupNode = Node;

            return TRUE;
        }
        else if (CompareResult < 0)
        {
            Index = 0;
        }
        else if (CompareResult > 0)
        {
            Index = 1;
        }

        // Move to next child
        Node = RsBtGetChildNode(Node, Index);
    }

    return FALSE;
}

/**
 * @brief Searches node by given key with option.
 * 
 * @param [in] Tree                 Tree.
 * @param [in] Key                  Node key.
 * @param [in] Lookup               Lookup option.
 * @param [out] LookupNode          Pointer to caller-supplied variable to receive target node.
 *
 * @return TRUE if lookup succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
RsBtLookup2(
    IN RS_BINARY_TREE *Tree,
    IN PVOID Key,
    IN U32 Lookup,
    OUT RS_BINARY_TREE_LINK **LookupNode)
{
    INT Index = 0;
    RS_BINARY_TREE_LINK *Node = Tree->Root;
    PVOID CapturedContext = Tree->CallerContext;

    U32 LookupMethod = Lookup & ~RS_BT_LOOKUP_FLAG_BITS;
    U32 LookupFlags = Lookup & RS_BT_LOOKUP_FLAG_BITS;

    RS_BINARY_TREE_LINK *NearestNode = Node;

    while (Node)
    {
        INT CompareResult = Tree->Operations.CompareKey(
            CapturedContext, Key, Tree->Operations.GetKey(CapturedContext, Node));

        if (CompareResult == 0)
        {
            if (LookupMethod == RS_BT_LOOKUP_EQUAL || 
                (LookupFlags & RS_BT_LOOKUP_FLAG_EQUAL))
            {
                NearestNode = Node;
                break;
            }
        }
        else if (CompareResult < 0)
        {
            if (LookupMethod == RS_BT_LOOKUP_NEAREST_BELOW)
                NearestNode = Node;

            Index = 0;
        }
        else if (CompareResult > 0)
        {
            if (LookupMethod == RS_BT_LOOKUP_NEAREST_ABOVE)
                NearestNode = Node;

            Index = 1;
        }

        // Move to next child
        Node = RsBtGetChildNode(Node, Index);
    }

    if (!NearestNode)
        return FALSE;

    if (LookupNode)
        *LookupNode = NearestNode;

    return TRUE;
}

/**
 * @brief Allocates and inserts node by given key.
 * 
 * @param [in] Tree                 Tree.
 * @param [in] Key                  Node key.
 *
 * @return TRUE if insertion succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
RsBtInsert(
    IN RS_BINARY_TREE *Tree,
    IN PVOID Key)
{
    if (RsBtLookup(Tree, Key, NULL))
    {
        // Cannot be inserted due to key collision
        return FALSE;
    }

    PVOID CapturedContext = Tree->CallerContext;
    RS_BINARY_TREE_LINK *NewNode = Tree->Operations.AllocateNode(CapturedContext);

    if (!NewNode)
    {
        // Cannot allocate new node
        return FALSE;
    }

    Tree->Operations.SetKey(Tree->CallerContext, NewNode, Key);

    if (!RsBtInsertLeaf(Tree, NewNode))
    {
        // Failed to insert node
        Tree->Operations.DeleteNode(CapturedContext, NewNode);
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Calculates balance factor of given node.
 * 
 * @param [in] Node                 Node.
 *
 * @return (Height of right subtree) - (Height of left subtree).
 */
inline
S8
KERNELAPI
RsAvlGetBalance(
    IN RS_AVL_NODE *Node)
{
    RS_AVL_NODE *LeftChild = BINARY_TREE_NODE_TO_AVL_NODE(Node->Links.LeftChild);
    RS_AVL_NODE *RightChild = BINARY_TREE_NODE_TO_AVL_NODE(Node->Links.RightChild);

    S8 LeftHeight = 0;
    S8 RightHeight = 0;

    if (LeftChild)
        LeftHeight = LeftChild->Height;

    if (RightChild)
        RightHeight = RightChild->Height;

    return RightHeight - LeftHeight;
}

/**
 * @brief Updates height of given node.
 * 
 * @param [in] Node                 Node.
 *
 * @return max((Height of right subtree), (Height of left subtree)) + 1.
 */
inline
S8
KERNELAPI
RsAvlUpdateHeight(
    IN RS_AVL_NODE *Node)
{
    RS_AVL_NODE *LeftChild = BINARY_TREE_NODE_TO_AVL_NODE(Node->Links.LeftChild);
    RS_AVL_NODE *RightChild = BINARY_TREE_NODE_TO_AVL_NODE(Node->Links.RightChild);

    S8 LeftHeight = 0;
    S8 RightHeight = 0;

    if (LeftChild)
        LeftHeight = LeftChild->Height;

    if (RightChild)
        RightHeight = RightChild->Height;

    // max(LeftHeight, RightHeight)
    S8 NewHeight = RightHeight > LeftHeight ? RightHeight : LeftHeight;
    Node->Height = NewHeight + 1;

    return NewHeight;
}

/**
 * @brief Updates ancestors height of given node.
 * 
 * @param [in] Node                     Node.
 * @param [in] StopIfUnbalancedFound    If TRUE, update will be stopped at first unbalanced node.
 * @param [out] FirstUnbalanced         Pointer to caller-supplied variable to receive\n
 *                                      first unbalanced node.
 *
 * @return None.
 */
VOID
KERNELAPI
RsAvlUpdateHeightAncestors(
    IN RS_AVL_NODE *Node,
    IN BOOLEAN StopIfUnbalancedFound,
    OUT RS_AVL_NODE **FirstUnbalanced)
{
    RS_AVL_NODE *Unbalanced = NULL;
    S8 Balance = 0;

    if (!Node)
        return;

    RS_AVL_NODE *Current = Node;

    while (Current)
    {
        RsAvlUpdateHeight(Current);
        Balance = RsAvlGetBalance(Current);
        if ((Balance > 1 || Balance < -1) && !Unbalanced)
        {
            Unbalanced = Current;
            if (StopIfUnbalancedFound)
                break;
        }

        Current = BINARY_TREE_NODE_TO_AVL_NODE(Current->Links.Parent);
    }

    if (FirstUnbalanced)
        *FirstUnbalanced = Unbalanced;
}

/**
 * @brief Rotates the tree.
 * 
 * @param [in] Tree             Tree.
 * @param [in] Target           Target node to rotate.
 * @param [in] RotateRight      If FALSE, left rotation will be performed.\n
 *                              Otherwise, right rotation will be performed.
 *
 * @return None.
 */
VOID
KERNELAPI
RsAvlRotate(
    IN RS_BINARY_TREE *Tree,
    IN RS_AVL_NODE *Target,
    IN BOOLEAN RotateRight)
{
    RS_AVL_NODE *Parent = NULL;
    RS_AVL_NODE *Child = NULL;
    RS_AVL_NODE *ChildChild = NULL;

    if (RotateRight)
    {
        /*
                 Rotate Right
                P             P
                |             |
                U             V
               / \           / \
              V   3   =>    1   U
             / \               / \
            1   2             2   3
        */

        INT NodeIndex = RsBtNodeIndexFromChild(&Target->Links);

        RsBtUnlinkNode(
            &Target->Links,
            RS_BT_UNLINK_PARENT | RS_BT_UNLINK_LEFT_CHILD,
            (RS_BINARY_TREE_LINK **)&Parent, // Parent = P
            (RS_BINARY_TREE_LINK **)&Child,  // Child = V
            NULL);

        if (Child)
        {
            RsBtUnlinkNode(
                &Child->Links,
                RS_BT_UNLINK_RIGHT_CHILD,
                NULL,
                NULL,
                (RS_BINARY_TREE_LINK **)&ChildChild); // ChildChild = 2
        }

        if (Parent)
        {
            RsBtLinkNode(&Parent->Links, &Child->Links, NodeIndex);
        }
        else
        {
            // V becomes new root.
            DASSERT(Child);
            Tree->Root = &Child->Links;
        }

        if (Child)
        {
            RsBtLinkNode(&Child->Links, &Target->Links, 1 /*Right*/);
        }

        RsBtLinkNode(&Target->Links, &ChildChild->Links, 0 /*Left*/);
    }
    else
    {
        /*
                 Rotate Left
              P              P
              |              |
              U              V
             / \            / \    
            1   V    =>    U   3
               / \        / \      
              2   3      1   2
        */

        INT NodeIndex = RsBtNodeIndexFromChild(&Target->Links);

        RsBtUnlinkNode(
            &Target->Links,
            RS_BT_UNLINK_PARENT | RS_BT_UNLINK_RIGHT_CHILD,
            (RS_BINARY_TREE_LINK **)&Parent, // Parent = P
            NULL,
            (RS_BINARY_TREE_LINK **)&Child); // Child = V

        if (Child)
        {
            RsBtUnlinkNode(
                &Child->Links,
                RS_BT_UNLINK_LEFT_CHILD,
                NULL,
                (RS_BINARY_TREE_LINK **)&ChildChild, // ChildChild = 2
                NULL);
        }

        if (Parent)
        {
            RsBtLinkNode(&Parent->Links, &Child->Links, NodeIndex);
        }
        else
        {
            // V becomes new root.
            DASSERT(Child);
            Tree->Root = &Child->Links;
        }

        if (Child)
        {
            RsBtLinkNode(&Child->Links, &Target->Links, 0 /*Left*/);
        }

        RsBtLinkNode(&Target->Links, &ChildChild->Links, 1 /*Right*/);
    }

    //
    // Update height for node U, V.
    // Update U first.
    //

    RsAvlUpdateHeight(Target);

    if (Child)
        RsAvlUpdateHeight(Child);
}

/**
 * @brief Rebalances the tree.
 * 
 * @param [in] Tree             Tree.
 * @param [in] Unbalanced       Unbalanced node.
 * @param [in] NextUnbalanced   Pointer to cAller-supplied variable to receive\n
 *                              unbalanced node if exists.
 *
 * @return TRUE if rebalancing performed, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
RsAvlRebalance(
    IN RS_BINARY_TREE *Tree,
    IN RS_AVL_NODE *Unbalanced,
    OUT RS_AVL_NODE **NextUnbalanced)
{
    /*
       There are 4 cases for rebalancing:
      
               U            U          U            U
              / \          / \        / \          / \                
             V   4        V   4      1   V        1   V
            / \          / \            / \          / \              
           W   3        1   W          W   4        2   W
          / \              / \        / \              / \             
         1   2            2   3      2   3            3   4
        a) LL case    b) LR case    c) RL case    d) RR case
      
               U               V
              / \            /   \           
             V   4    =>    W     U
            / \            / \   / \          
           W   3          1   2 3   4
          / \                            
         1   2
        a) LL case : Right rotation (U).
      
              U              U            W
             / \            / \         /   \     
            V   4   =>     W   4  =>   V     U
           / \            / \         / \   / \   
          1   W          V   3       1   2 3   4
             / \        / \          
            2   3      1   2
        b) LR case : Left rotation (V) -> Right rotation (U).
      
        c) RL case : Right rotation (V) -> Left rotation (U). [Symmetric case of LR]
        d) RR case : Left rotation (U). [Symmetric case of LL]
    */

    S8 Balance = RsAvlGetBalance(Unbalanced);
    RS_AVL_NODE *Child = NULL;
    S8 ChildBalance = 0;
    RS_AVL_NODE *UpdateHeightFrom = NULL;

    if (Balance == +2)
    {
        Child = BINARY_TREE_NODE_TO_AVL_NODE(RsBtGetChildNode(&Unbalanced->Links, 1));
        ChildBalance = RsAvlGetBalance(Child);

        if (ChildBalance == -1)
        {
            // RL case
            RsAvlRotate(Tree, Child, TRUE);
            RsAvlRotate(Tree, Unbalanced, FALSE);
            UpdateHeightFrom = Child;
        }
        else if (ChildBalance == 1 || ChildBalance == 0)
        {
            // RR case
            RsAvlRotate(Tree, Unbalanced, FALSE);
            UpdateHeightFrom = Child;
        }
    }
    else if (Balance == -2)
    {
        Child = BINARY_TREE_NODE_TO_AVL_NODE(RsBtGetChildNode(&Unbalanced->Links, 0));
        ChildBalance = RsAvlGetBalance(Child);

        if (ChildBalance == -1 || ChildBalance == 0)
        {
            // LL case
            RsAvlRotate(Tree, Unbalanced, TRUE);
            UpdateHeightFrom = Child;
        }
        else if (ChildBalance == 1)
        {
            // LR case
            RsAvlRotate(Tree, Child, FALSE);
            RsAvlRotate(Tree, Unbalanced, TRUE);
            UpdateHeightFrom = Child;
        }
    }

    // Rebalancing not performed.
    if (!UpdateHeightFrom)
        return FALSE;

    // Update height for ancestors.
    RS_AVL_NODE *FirstUnbalanced = NULL;
    RsAvlUpdateHeightAncestors(
        BINARY_TREE_NODE_TO_AVL_NODE(UpdateHeightFrom->Links.Parent),
        TRUE,
        &FirstUnbalanced);

    *NextUnbalanced = FirstUnbalanced;

    return TRUE;
}

/**
 * @brief Allocates and inserts node by given key.
 * 
 * @param [in] Tree         Tree.
 * @param [in] Key          Key to insert.
 * @param [out] Inserted    Pointer to caller-supplied variable
 *                          to receive Result node.
 *
 * @return TRUE if insertion succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
RsAvlInsert(
    IN RS_BINARY_TREE *Tree,
    IN PVOID Key,
    OUT RS_BINARY_TREE_LINK **Inserted)
{
    if (RsBtLookup(Tree, Key, NULL))
    {
        // Cannot be inserted due to key collision
        return FALSE;
    }

    PVOID CapturedContext = Tree->CallerContext;
    RS_BINARY_TREE_LINK *NewNode = Tree->Operations.AllocateNode(CapturedContext);

    if (!NewNode)
    {
        // Cannot allocate new node
        return FALSE;
    }

    Tree->Operations.SetKey(Tree->CallerContext, NewNode, Key);

    if (!RsBtInsertLeaf(Tree, NewNode))
    {
        // Failed to insert node
        Tree->Operations.DeleteNode(CapturedContext, NewNode);
        return FALSE;
    }

    // Update height for ancestors.
    RS_AVL_NODE *FirstUnbalanced = NULL;
    RsAvlUpdateHeightAncestors(
        BINARY_TREE_NODE_TO_AVL_NODE(NewNode->Parent),
        TRUE, /* Stop update if unbalanced node is found */
        &FirstUnbalanced);

    if (FirstUnbalanced)
    {
        RS_AVL_NODE *NextUnbalanced = NULL;
        RsAvlRebalance(Tree, FirstUnbalanced, &NextUnbalanced);
        DASSERT(!NextUnbalanced);
    }

    if (Inserted)
        *Inserted = NewNode;

    return TRUE;
}

/**
 * @brief Removes node from tree.\n
 *        The node will not be deleted unless you call RsAvlDelete.
 * 
 * @param [in] Tree                 Tree.
 * @param [in] Node                 Node to remove.
 *
 * @return TRUE if removal succeeds, FALSE otherwise.
 * 
 * @warning Do not call RsBtDeleteNode for AVL tree node
 *          as it handles deletion differently.
 *
 */
BOOLEAN
KERNELAPI
RsAvlRemove(
    IN RS_BINARY_TREE *Tree,
    IN RS_AVL_NODE *Node)
{
    RS_BINARY_TREE_LINK *UpdatePoint = NULL;
    if (!RsBtRemoveNode(Tree, &Node->Links, &UpdatePoint))
    {
        // Cannot be removed from tree
        return FALSE;
    }

    if (UpdatePoint)
    {
        RS_AVL_NODE *Unbalanced = NULL;
        RsAvlUpdateHeightAncestors(
            BINARY_TREE_NODE_TO_AVL_NODE(UpdatePoint), TRUE, &Unbalanced);

        while (Unbalanced)
        {
            // Removal may require tree rebalancing more than once...
            RS_AVL_NODE *NextUnbalanced = NULL;
            DASSERT(RsAvlRebalance(Tree, Unbalanced, &NextUnbalanced));
            Unbalanced = NextUnbalanced;
        }
    }

    return TRUE;
}

/**
 * @brief Deletes node from tree.
 * 
 * @param [in] Tree                 Tree.
 * @param [in] Node                 Node to delete.
 *
 * @return TRUE if deletion succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
RsAvlDelete(
    IN RS_BINARY_TREE *Tree,
    IN RS_AVL_NODE *Node)
{
    if (Node->Links.Parent ||
        Node->Links.LeftChild ||
        Node->Links.RightChild)
    {
        // Unlink first
        if (!RsAvlRemove(Tree, BINARY_TREE_NODE_TO_AVL_NODE(Node)))
        {
            return FALSE;
        }
    }

    RsBtDeleteNode(Tree, &Node->Links);

    return TRUE;
}

/**
 * @brief Perform search-remove-delete for node from tree.
 * 
 * @param [in] Tree                 Tree.
 * @param [in] Key                  Key to delete.
 *
 * @return TRUE if deletion succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
RsAvlDeleteByKey(
    IN RS_BINARY_TREE *Tree,
    IN PVOID Key)
{
    RS_BINARY_TREE_LINK *Node = NULL;
    if (!RsBtLookup(Tree, Key, &Node))
    {
        return FALSE;
    }

    if (!RsAvlRemove(Tree, BINARY_TREE_NODE_TO_AVL_NODE(Node)))
    {
        return FALSE;
    }

    RsBtDeleteNode(Tree, Node);

    return TRUE;
}
