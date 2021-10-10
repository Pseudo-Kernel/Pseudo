
/**
 * @file interrupt.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements interrupt allocation, free, and dispatch.
 * @version 0.1
 * @date 2021-10-06
 * 
 * @copyright Copyright (c) 2021
 * 
 * @todo Test!
 */


#include <base/base.h>
#include <ke/lock.h>
#include <ke/irql.h>
#include <ke/interrupt.h>
#include <init/bootgfx.h>

KIRQ_GROUP KiIrqGroup[IRQ_GROUPS_MAX];

#define VECTOR_TO_IRQ_GROUP_POINTER(_vector)                (&KiIrqGroup[VECTOR_TO_IRQL(_vector)])
#define VECTOR_TO_IRQ_POINTER(_vector)          \
    (&VECTOR_TO_IRQ_GROUP_POINTER(Vector)->Irq[VECTOR_TO_GROUP_IRQ_INDEX(Vector)])


/**
 * @brief Acquires IRQ group lock.
 * 
 * @param [in] IrqGroup     IRQ Group.
 * 
 * @return None.
 */
VOID
KERNELAPI
KiAcquireIrqGroupLock(
    IN KIRQ_GROUP *IrqGroup)
{
    KIRQL PrevIrql = 0;

    KeAcquireSpinlockRaiseIrql(&IrqGroup->GroupLock, IrqGroup->GroupIrql, &PrevIrql);
    IrqGroup->PreviousIrql = PrevIrql;
}

/**
 * @brief Releases IRQ group lock.
 * 
 * @param [in] IrqGroup     IRQ Group.
 * 
 * @return None.
 */
VOID
KERNELAPI
KiReleaseIrqGroupLock(
    IN KIRQ_GROUP *IrqGroup)
{
    KIRQL PrevIrql = IrqGroup->PreviousIrql;

    IrqGroup->PreviousIrql = 0;
    KeReleaseSpinlockLowerIrql(&IrqGroup->GroupLock, PrevIrql);
}

/**
 * @brief Allocates the IRQ vector by given IRQL and count.
 * 
 * @param [in] GroupIrql        Vector IRQL.
 * @param [in] Count            Allocate count.
 * @param [in] VectorHint       Hint of starting vector. KeAllocateIrqVector tries to allocate IRQ vector\n
 *                              starting with VectorHint. If zero is specified, this parameter is ignored.
 * @param [in] Flags            Combination of bit flags. See INTERRUPT_IRQ_XXX.
 * @param [out] StartingVector  Result vector.
 * @param [in] LockGroupIrq     Specifies whether the IRQ group locking is performed.\n
 *                              IRQ group locking will not be performed if FALSE is specified.
 * 
 * @return ESTATUS status code.
 */
ESTATUS
KERNELAPI
KeAllocateIrqVector(
    IN KIRQL GroupIrql,
    IN ULONG Count,
    OPTIONAL IN ULONG VectorHint, // Ignored if 0
    IN ULONG Flags,
    OUT ULONG *StartingVector,
    IN BOOLEAN LockGroupIrq)
{
    if (!IRQL_VALID(GroupIrql) || GroupIrql < IRQL_NORMAL)
    {
        // Invalid IRQL.
        return E_INVALID_PARAMETER;
    }

    if (Count > IRQS_PER_IRQ_GROUP || !Count)
    {
        // Invalid count.
        return E_INVALID_PARAMETER;
    }

    if (VectorHint)
    {
        if (!IRQ_VECTOR_VALID(VectorHint))
        {
            // Vector is invalid.
            return E_INVALID_PARAMETER;
        }

        if (VECTOR_TO_IRQL(VectorHint) != GroupIrql)
        {
            // Vector does not belongs to specified IRQL.
            return E_INVALID_PARAMETER;
        }
    }

    ULONG IndexStart = 0;
    ULONG IndexLimit = 0;

    if (VectorHint)
    {
        IndexStart = VECTOR_TO_GROUP_IRQ_INDEX(VectorHint);
        IndexLimit = IndexStart + Count - 1;

        if (VECTOR_TO_IRQL(IndexLimit) != VECTOR_TO_IRQL(IndexStart))
        {
            // Cannot allocate vectors for multiple groups at once
            return E_INVALID_PARAMETER;
        }
    }
    else
    {
        IndexStart = 0;
        IndexLimit = Count - 1;
    }

    KIRQ_GROUP *IrqGroup = &KiIrqGroup[GroupIrql];
    ULONG AllocationBitmap = 0;
    ULONG TargetMask = ((1 << Count) - 1);

    if (LockGroupIrq)
    {
        // Lock IRQ group.
        KiAcquireIrqGroupLock(IrqGroup);
    }
    else
    {
    }

    // 
    // Get allocation bitmap.
    // 

    for (ULONG i = IndexStart; i <= IndexLimit; i++)
    {
        BOOLEAN Allocate = FALSE;
        KIRQ *Irq = &IrqGroup->Irq[i];

        if (Flags & INTERRUPT_IRQ_SHARED)
        {
            // Allow irq allocation multiple times for sharing
            if ((Irq->Allocated && Irq->Shared) ||
                !Irq->Allocated)
            {
                Allocate = TRUE;
            }
        }
        else
        {
            if (!Irq->Allocated)
            {
                Allocate = TRUE;
            }
        }

        if (Allocate)
        {
            AllocationBitmap |= (1 << i);
        }
    }

    //
    // Find free vectors.
    //

    ULONG TargetIndex = 0;
    BOOLEAN Found = FALSE;

    for (ULONG i = IndexStart; i <= IndexLimit; i++)
    {
        if ((AllocationBitmap & (TargetMask << i)) == (TargetMask << i))
        {
            if (Flags & INTERRUPT_IRQ_HINT_EXACT_MATCH)
            {
                // Exact match failed
                if (i != IndexStart)
                    break;
            }

            TargetIndex = i;
            Found = TRUE;
            break;
        }
    }

    if (Found)
    {
        for (ULONG i = 0; i < Count; i++)
        {
            KIRQ *Irq = &IrqGroup->Irq[i + TargetIndex];

            if (Flags & INTERRUPT_IRQ_SHARED)
            {
                // Shared
                if (!Irq->Allocated)
                {
                    Irq->Allocated = TRUE;
                    Irq->Shared = TRUE;
                }

                Irq->SharedCount++;
            }
            else
            {
                // Exclusive
                ASSERT(!Irq->Allocated);

                Irq->Allocated = TRUE;
                Irq->Shared = FALSE;
            }
        }

        *StartingVector = IRQL_TO_VECTOR_START(GroupIrql) + TargetIndex;
    }

    if (LockGroupIrq)
    {
        KiReleaseIrqGroupLock(IrqGroup);
    }
    else
    {
    }

    if (!Found)
    {
        return E_NOT_ENOUGH_RESOURCE;
    }

    return E_SUCCESS;
}

/**
 * @brief Tests whether the given IRQ vector is allocated.
 * 
 * @param [in] Vector           IRQ vector.
 * @param [in] Flags            Combination of bit flags. See INTERRUPT_IRQ_XXX.
 * @param [out] Allocated       Allocation state. If given vector is allocated, TRUE will be passed to the caller.
 * @param [in] LockGroupIrq     Specifies whether the IRQ group locking is performed.\n
 *                              IRQ group locking will not be performed if FALSE is specified.
 * 
 * @return ESTATUS status code.
 * 
 * @warning Caller must not trust *Allocated if function fails.
 */
ESTATUS
KERNELAPI
KeIsIrqVectorAllocated(
    IN ULONG Vector,
    IN ULONG Flags,
    OUT BOOLEAN *Allocated,
    IN BOOLEAN LockGroupIrq)
{
    KIRQL GroupIrql = VECTOR_TO_IRQL(Vector);
    if (!IRQ_VECTOR_VALID(Vector) || !IRQL_VALID(GroupIrql))
    {
        return E_INVALID_PARAMETER;
    }

    KIRQ_GROUP *IrqGroup = &KiIrqGroup[GroupIrql];

    if (LockGroupIrq)
    {
        // Lock IRQ group.
        KiAcquireIrqGroupLock(IrqGroup);
    }
    else
    {
    }

    KIRQ *Irq = &IrqGroup->Irq[VECTOR_TO_GROUP_IRQ_INDEX(Vector)];
    BOOLEAN State = FALSE;

    if (Flags & INTERRUPT_IRQ_SHARED)
    {
        // Allow irq allocation multiple times for sharing
        if (Irq->Allocated && Irq->Shared)
        {
            State = TRUE;
        }
    }
    else
    {
        if (!Irq->Allocated)
        {
            State = TRUE;
        }
    }

    *Allocated = State;

    if (LockGroupIrq)
    {
        KiReleaseIrqGroupLock(IrqGroup);
    }
    else
    {
    }

    return E_SUCCESS;
}

/**
 * @brief Frees the IRQ vector.
 * 
 * @param [in] StartingVector   Starting vector to be freed.
 * @param [in] Count            Vector count.
 * @param [in] LockGroupIrq     Specifies whether the IRQ group locking is performed.\n
 *                              IRQ group locking will not be performed if FALSE is specified.
 * 
 * @return ESTATUS status code.
 * 
 */
ESTATUS
KERNELAPI
KeFreeIrqVector(
    IN ULONG StartingVector,
    IN ULONG Count,
    IN BOOLEAN LockGroupIrq)
{
    if (Count > IRQS_PER_IRQ_GROUP || !Count)
    {
        // Invalid count.
        return E_INVALID_PARAMETER;
    }

    if (!IRQ_VECTOR_VALID(StartingVector))
    {
        // Vector is invalid.
        return E_INVALID_PARAMETER;
    }

    if (VECTOR_TO_IRQL(StartingVector) != VECTOR_TO_IRQL(StartingVector + Count - 1))
    {
        // Cannot free vectors for multiple groups
        return E_INVALID_PARAMETER;
    }

    KIRQL GroupIrql = VECTOR_TO_IRQL(StartingVector);
    if (!IRQL_VALID(GroupIrql))
    {
        return E_INVALID_PARAMETER;
    }

    KIRQ_GROUP *IrqGroup = &KiIrqGroup[GroupIrql];

    if (LockGroupIrq)
    {
        // Lock IRQ group.
        KiAcquireIrqGroupLock(IrqGroup);
    }
    else
    {

    }

    // 
    // Free!
    // 

    ULONG IndexStart = VECTOR_TO_GROUP_IRQ_INDEX(StartingVector);
    ULONG IndexLimit = VECTOR_TO_GROUP_IRQ_INDEX(StartingVector + Count - 1);
    ESTATUS Status = E_SUCCESS;

    for (ULONG i = IndexStart; i <= IndexLimit; i++)
    {
        KIRQ *Irq = &IrqGroup->Irq[i];

        if (!Irq->Allocated)
        {
            Status = E_INVALID_PARAMETER;
            break;
        }
    }

    if (E_IS_SUCCESS(Status))
    {
        for (ULONG i = IndexStart; i <= IndexLimit; i++)
        {
            KIRQ *Irq = &IrqGroup->Irq[i];

            if (!Irq->Allocated)
            {
                ASSERT(FALSE);
                break;
            }

            ASSERT(DListIsEmpty(&Irq->InterruptListHead));

            if (Irq->Shared)
            {
                Irq->SharedCount--;
                if (!Irq->SharedCount)
                {
                    Irq->Shared = FALSE;
                    Irq->Allocated = FALSE;
                }
            }
            else
            {
                Irq->Allocated = FALSE;
            }
        }
    }

    if (LockGroupIrq)
    {
        KiReleaseIrqGroupLock(IrqGroup);
    }
    else
    {

    }

    return Status;
}

/**
 * @brief Connects the interrupt object to interrupt chain.
 * 
 * @param [in] Interrupt    Interrupt object.
 * @param [in] Vector       Desired vector. The caller must call KeAllocateIrqVector() to allocate vector.
 * @param [in] Flags        Combination of bit flags. See INTERRUPT_IRQ_XXX.
 * 
 * @return ESTATUS status code.
 * 
 */
ESTATUS
KERNELAPI
KeConnectInterrupt(
    IN PKINTERRUPT Interrupt,
    IN ULONG Vector,
    IN ULONG Flags)
{
    KIRQL Irql = VECTOR_TO_IRQL(Vector);
    if (!IRQ_VECTOR_VALID(Vector) || !IRQL_VALID(Irql))
    {
        return E_INVALID_PARAMETER;
    }

    if (Interrupt->Connected)
    {
        return E_INVALID_PARAMETER;
    }

    KIRQ_GROUP *IrqGroup = &KiIrqGroup[Irql];

    KiAcquireIrqGroupLock(IrqGroup);

    BOOLEAN Allocated = FALSE;
    ESTATUS Status = KeIsIrqVectorAllocated(Vector, Flags, &Allocated, FALSE);

    if (E_IS_SUCCESS(Status))
    {
        ULONG Index = VECTOR_TO_GROUP_IRQ_INDEX(Vector);

        Interrupt->AutoEoi = !!(Flags & INTERRUPT_AUTO_EOI);

        _InterlockedExchange8((volatile char *)&Interrupt->InterruptVector, Vector);
        DListInsertAfter(&IrqGroup->Irq[Index].InterruptListHead, &Interrupt->InterruptList);
        Interrupt->Connected = TRUE;
    }

    KiReleaseIrqGroupLock(IrqGroup);

    return Status;
}

/**
 * @brief Disconnects the interrupt object from interrupt chain.
 * 
 * @param [in] Interrupt    Interrupt object.
 * 
 * @return ESTATUS status code.
 * 
 * @warning Do not call KeConnectInterrupt() and KeDisconnectInterrupt() concurrently for same KINTERRUPT\n
 *          as it may cause race condition.
 */
ESTATUS
KERNELAPI
KeDisconnectInterrupt(
    IN PKINTERRUPT Interrupt)
{
    if (!Interrupt->Connected)
    {
        return E_INVALID_PARAMETER;
    }

    ULONG Vector = _InterlockedCompareExchange8((volatile char *)&Interrupt->InterruptVector, 0, 0);
    KIRQL Irql = VECTOR_TO_IRQL(Vector);

    if (!Vector || !IRQ_VECTOR_VALID(Vector) || !IRQL_VALID(Irql))
    {
        return E_INVALID_PARAMETER;
    }

    KIRQ_GROUP *IrqGroup = &KiIrqGroup[Irql];

    KiAcquireIrqGroupLock(IrqGroup);

    ULONG ExpectedVector = _InterlockedCompareExchange8((volatile char *)&Interrupt->InterruptVector, 0, 0);
    KIRQL ExpectedIrql = VECTOR_TO_IRQL(ExpectedVector);

    if (!ExpectedVector || 
        !IRQ_VECTOR_VALID(ExpectedVector) ||
        !IRQL_VALID(ExpectedIrql))
    {
        KiReleaseIrqGroupLock(IrqGroup);
        return E_INVALID_PARAMETER;
    }

    if (ExpectedIrql != Irql)
    {
        KiReleaseIrqGroupLock(IrqGroup);
        return E_RACE_CONDITION;
    }

    DListRemoveEntry(&Interrupt->InterruptList);
    _InterlockedExchange8((volatile char *)&Interrupt->InterruptVector, 0);
    Interrupt->Connected = FALSE;

    KiReleaseIrqGroupLock(IrqGroup);

    return E_SUCCESS;
}

/**
 * @brief Initializes the interrupt object.
 * 
 * @param [out] Interrupt           Interrupt object.
 * @param [in] InterruptRoutine     Interrupt service routine.
 * @param [in] InterruptContext     Interrupt context.
 * @param [in] Reserved             Reserved for future use.
 * 
 * @return ESTATUS status code.
 */
ESTATUS
KERNELAPI
KeInitializeInterrupt(
    OUT PKINTERRUPT Interrupt,
    IN PKINTERRUPT_ROUTINE InterruptRoutine,
    IN PVOID InterruptContext,
    IN U64 Reserved /*InterruptAffinity*/)
{
    memset(Interrupt, 0, sizeof(*Interrupt));

    Interrupt->Connected = FALSE;
    Interrupt->AutoEoi = FALSE;
    Interrupt->InterruptContext = InterruptContext;
    Interrupt->InterruptRoutine = InterruptRoutine;
    Interrupt->InterruptVector = 0;
    DListInitializeHead(&Interrupt->InterruptList);

    return E_SUCCESS;
}

/**
 * @brief Calls the interrupt chain by given vector.
 * 
 * @param [in] Vector   IDT vector number.
 * 
 * @return None.
 */
VOID
KERNELAPI
KiCallInterruptChain(
    IN U8 Vector)
{
    ULONG Index = VECTOR_TO_GROUP_IRQ_INDEX(Vector);
    KIRQ_GROUP *IrqGroup = VECTOR_TO_IRQ_GROUP_POINTER(Vector);

    KiAcquireIrqGroupLock(IrqGroup);

    DASSERT(IrqGroup->Irq[Index].Allocated);

    PDLIST_ENTRY ListHead = &IrqGroup->Irq[Index].InterruptListHead;
    PDLIST_ENTRY Next = ListHead->Next;
    BOOLEAN Dispatched = FALSE;

    while (ListHead != Next)
    {
        PKINTERRUPT Interrupt = CONTAINING_RECORD(Next, KINTERRUPT, InterruptList);
        DASSERT(Interrupt->Connected && Interrupt->InterruptVector == Vector);

        KINTERRUPT_RESULT Result = Interrupt->InterruptRoutine(Interrupt, Interrupt->InterruptContext);

        if (Result == InterruptError || Result == InterruptAccepted)
        {
            if (Interrupt->AutoEoi)
            {
                //
                // @todo : Handle auto EOI.
                //         Write zero to LAPIC EOI register.
                //
            }

            Dispatched = TRUE;
            break;
        }
        else if (Result == InterruptCallNext)
        {
            // Call next interrupt
            continue;
        }
        else
        {
            DASSERT(FALSE);
        }
    }

    DASSERT(Dispatched);
    
    KiReleaseIrqGroupLock(IrqGroup);
}
