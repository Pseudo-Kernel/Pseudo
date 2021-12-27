
/**
 * @file lock.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements simple spinlock.
 * @version 0.1
 * @date 201?-??-??
 * 
 * @copyright Copyright (c) 2021
 * 
 * @todo Need to test all lock-related routines. (KeXxxSpinlockXxx)\n
 */

#include <base/base.h>
#include <ke/ke.h>
#include <ke/lock.h>
#include <init/bootgfx.h>

/**
 * @brief Initializes the spinlock.
 * 
 * @param [out] Lock    Spinlock.
 * 
 * @return None.
 */
VOID
KERNELAPI
KeInitializeSpinlock(
    OUT PKSPIN_LOCK Lock)
{
    _InterlockedExchange(&Lock->Lock, 0);
}

/**
 * @brief Tries to acquire the spinlock.
 * 
 * @param [in] Lock     Spinlock.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
KeTryAcquireSpinlock(
    IN PKSPIN_LOCK Lock)
{
    return !_InterlockedExchange(&Lock->Lock, 1);
}

/**
 * @brief Acquires the spinlock.
 * 
 * @param [in] Lock     Spinlock.
 * 
 * @return None.
 */
VOID
KERNELAPI
KeAcquireSpinlock(
    IN PKSPIN_LOCK Lock)
{
    while (!KeTryAcquireSpinlock(Lock))
        _mm_pause();
}

/**
 * @brief Releases the spinlock.
 * 
 * @param [in] Lock     Spinlock.
 * 
 * @return None.
 */
VOID
KERNELAPI
KeReleaseSpinlock(
    IN PKSPIN_LOCK Lock)
{
    long Prev = _InterlockedExchange(&Lock->Lock, 0);

    ASSERT(Prev);
}

/**
 * @brief Tests whether the spinlock is already acquired.
 * 
 * @param [in] Lock     Spinlock.
 * 
 * @return TRUE if spinlock is already acquired.
 */
BOOLEAN
KERNELAPI
KeIsSpinlockAcquired(
    IN PKSPIN_LOCK Lock)
{
    return !!_InterlockedCompareExchange(&Lock->Lock, 0, 0);
}

/**
 * @brief Tries to acquire the spinlocks sequentially (LockList[0..(Count-1)]).\n
 * 
 * @param [in] LockList     Array of spinlock.
 * @param [in] Count        Spinlock count. This parameter cannot be zero.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
KeTryAcquireSpinlockMultiple(
    IN PKSPIN_LOCK *LockList,
    IN ULONG Count)
{
    ULONG AcquiredCount = 0;

    if (!Count)
    {
        FATAL("Zero count cannot be accepted");
    }

    for (AcquiredCount = 0; AcquiredCount < Count; AcquiredCount++)
    {
        if (!KeTryAcquireSpinlock(LockList[AcquiredCount]))
            break;
    }

    if (AcquiredCount == Count)
    {
        return TRUE;
    }

    while (AcquiredCount)
    {
        KeReleaseSpinlock(LockList[AcquiredCount]);
        AcquiredCount--;
    }

    return FALSE;
}

/**
 * @brief Acquires the spinlocks sequentially (LockList[0..(Count-1)]).
 * 
 * @param [in] LockList     Array of spinlock.
 * @param [in] Count        Spinlock count. This parameter cannot be zero.
 * 
 * @return None.
 */
VOID
KERNELAPI
KeAcquireSpinlockMultiple(
    IN PKSPIN_LOCK *LockList,
    IN ULONG Count)
{
    while (!KeTryAcquireSpinlockMultiple(LockList, Count))
        _mm_pause();
}

/**
 * @brief Releases the spinlocks with reverse order (LockList[(Count-1)..0]).
 * 
 * @param [in] LockList     Array of spinlock.
 * @param [in] Count        Spinlock count. This parameter cannot be zero.
 * 
 * @return None.
 */
VOID
KERNELAPI
KeReleaseSpinlockMultiple(
    IN PKSPIN_LOCK *LockList,
    IN ULONG Count)
{
    if (!Count)
    {
        FATAL("Zero count cannot be accepted");
    }

    while (Count)
    {
        KeReleaseSpinlock(LockList[Count - 1]);
        Count--;
    }
}


//
// Spinlock with interrupt flag.
//

/**
 * @brief Disables the interrupt and tries to acquire the spinlock.\n
 *        If fails, interrupt enable state will not be changed.
 * 
 * @param [in] Lock         Spinlock.
 * @param [out] PrevState   Previous interrupt state.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
KeTryAcquireSpinlockDisableInterrupt(
    IN PKSPIN_LOCK Lock,
    OUT BOOLEAN *PrevState)
{
    BOOLEAN CurrentState = !!(__readeflags() & RFLAG_IF);
    _disable();

    long Prev = _InterlockedExchange(&Lock->Lock, 1);

    if (Prev)
    {
        // Failed to acquire the lock
        if (CurrentState)
        {
            _enable();
        }

        return FALSE;
    }

    *PrevState = CurrentState;
    return TRUE;
}

/**
 * @brief Disables the interrupt and acquires the spinlock.
 * 
 * @param [in] Lock         Spinlock.
 * @param [out] PrevState   Previous interrupt state.
 * 
 * @return None. 
 */
VOID
KERNELAPI
KeAcquireSpinlockDisableInterrupt(
    IN PKSPIN_LOCK Lock,
    OUT BOOLEAN *PrevState)
{
    while (!KeTryAcquireSpinlockDisableInterrupt(Lock, PrevState))
        _mm_pause();
}

/**
 * @brief Restores the interrupt enable state and releases the spinlock.
 * 
 * @param [in] Lock         Spinlock.
 * @param [in] PrevState    Previous interrupt state.
 * 
 * @return None. 
 */
VOID
KERNELAPI
KeReleaseSpinlockRestoreInterrupt(
    IN PKSPIN_LOCK Lock,
    IN BOOLEAN PrevState)
{
    long Prev = _InterlockedExchange(&Lock->Lock, 0);

    ASSERT(Prev);

    if (PrevState)
    {
        _enable();
    }
}

/**
 * @brief Disables the interrupt and tries to acquire the spinlocks sequentially.\n
 *        If fails, interrupt enable state will not be changed.
 * 
 * @param [in] LockList     Array of spinlock.
 * @param [in] Count        Spinlock count. This parameter cannot be zero.
 * @param [out] PrevState   Previous interrupt state.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
KeTryAcquireSpinlockMultipleDisableInterrupt(
    IN PKSPIN_LOCK *LockList,
    IN ULONG Count,
    OUT BOOLEAN *PrevState)
{
    ULONG AcquiredCount = 0;

    if (!Count)
    {
        FATAL("Zero count cannot be accepted");
    }

    BOOLEAN CurrentState = !!(__readeflags() & RFLAG_IF);
    _disable();

    for (AcquiredCount = 0; AcquiredCount < Count; AcquiredCount++)
    {
        if (!KeTryAcquireSpinlock(LockList[AcquiredCount]))
            break;
    }

    if (AcquiredCount == Count)
    {
        *PrevState = CurrentState;
        return TRUE;
    }

    while (AcquiredCount)
    {
        KeReleaseSpinlock(LockList[AcquiredCount]);
        AcquiredCount--;
    }

    if (CurrentState)
    {
        _enable();
    }

    return FALSE;
}

/**
 * @brief Disables the interrupt and acquires the spinlocks sequentially.
 * 
 * @param [in] LockList     Array of spinlock.
 * @param [in] Count        Spinlock count. This parameter cannot be zero.
 * @param [out] PrevState   Previous interrupt state.
 * 
 * @return None.
 */
VOID
KERNELAPI
KeAcquireSpinlockMultipleDisableInterrupt(
    IN PKSPIN_LOCK *LockList,
    IN ULONG Count,
    OUT BOOLEAN *PrevState)
{
    while (!KeTryAcquireSpinlockMultipleDisableInterrupt(LockList, Count, PrevState))
        _mm_pause();
}

/**
 * @brief Restores the interrupt enable state and releases the spinlocks with reverse order.
 * 
 * @param [in] LockList     Array of spinlock.
 * @param [in] Count        Spinlock count. This parameter cannot be zero.
 * @param [in] PrevState    Previous interrupt state.
 * 
 * @return None.
 */
VOID
KERNELAPI
KeReleaseSpinlockMultipleRestoreInterrupt(
    IN PKSPIN_LOCK *LockList,
    IN ULONG Count,
    IN BOOLEAN PrevState)
{
    if (!Count)
    {
        FATAL("Zero count cannot be accepted");
    }

    while (Count)
    {
        KeReleaseSpinlock(LockList[Count - 1]);
        Count--;
    }

    if (PrevState)
    {
        _enable();
    }
}


//
// Spinlock with IRQL.
//

/**
 * @brief Raises IRQL and tries to acquire the spinlock.\n
 *        If fails, IRQL will not be changed.
 * 
 * @param [in] Lock         Spinlock.
 * @param [in] Irql         IRQL to raise.
 * @param [out] PrevIrql    Previous IRQL.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
KeTryAcquireSpinlockRaiseIrql(
    IN PKSPIN_LOCK Lock,
    IN KIRQL Irql,
    OUT KIRQL *PrevIrql)
{
    KIRQL CurrentIrql = KeRaiseIrql(Irql);

    long Prev = _InterlockedExchange(&Lock->Lock, 1);

    if (Prev)
    {
        // Failed to acquire the lock
        KeLowerIrql(CurrentIrql);
        return FALSE;
    }

    *PrevIrql = CurrentIrql;
    return TRUE;
}

/**
 * @brief Raises IRQL and acquires the spinlock.
 * 
 * @param [in] Lock         Spinlock.
 * @param [in] Irql         IRQL to raise.
 * @param [out] PrevIrql    Previous IRQL.
 * 
 * @return None.
 */
VOID
KERNELAPI
KeAcquireSpinlockRaiseIrql(
    IN PKSPIN_LOCK Lock,
    IN KIRQL Irql,
    OUT KIRQL *PrevIrql)
{
    while (!KeTryAcquireSpinlockRaiseIrql(Lock, Irql, PrevIrql))
        _mm_pause();
}

/**
 * @brief Releases the spinlock and restores the IRQL.
 * 
 * @param [in] Lock         Spinlock.
 * @param [in] PrevIrql     Previous IRQL.
 * 
 * @return None.
 */
VOID
KERNELAPI
KeReleaseSpinlockLowerIrql(
    IN PKSPIN_LOCK Lock,
    IN KIRQL PrevIrql)
{
    long Prev = _InterlockedExchange(&Lock->Lock, 0);

    ASSERT(Prev);

    KeLowerIrql(PrevIrql);
}

/**
 * @brief Raises IRQL and tries to acquire the spinlocks sequentially.\n
 *        If fails, IRQL will not be changed.
 * 
 * @param [in] LockList     Array of spinlock.
 * @param [in] Count        Spinlock count. This parameter cannot be zero.
 * @param [in] Irql         IRQL to raise.
 * @param [out] PrevIrql    Previous IRQL.
 *  
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
KeTryAcquireSpinlockMultipleRaiseIrql(
    IN PKSPIN_LOCK *LockList,
    IN ULONG Count,
    IN KIRQL Irql,
    OUT KIRQL *PrevIrql)
{
    ULONG AcquiredCount = 0;

    if (!Count)
    {
        FATAL("Zero count cannot be accepted");
    }

    KIRQL CurrentIrql = KeRaiseIrql(Irql);

    for (AcquiredCount = 0; AcquiredCount < Count; AcquiredCount++)
    {
        if (!KeTryAcquireSpinlock(LockList[AcquiredCount]))
            break;
    }

    if (AcquiredCount == Count)
    {
        *PrevIrql = CurrentIrql;
        return TRUE;
    }

    while (AcquiredCount)
    {
        KeReleaseSpinlock(LockList[AcquiredCount]);
        AcquiredCount--;
    }

    // Failed to acquire the lock
    KeLowerIrql(CurrentIrql);
    return FALSE;
}

/**
 * @brief Disables the interrupt and acquires the spinlocks sequentially.
 * 
 * @param [in] LockList     Array of spinlock.
 * @param [in] Count        Spinlock count. This parameter cannot be zero.
 * @param [in] Irql         IRQL to raise.
 * @param [out] PrevIrql    Previous IRQL.
 * 
 * @return None.
 */
VOID
KERNELAPI
KeAcquireSpinlockMultipleRaiseIrql(
    IN PKSPIN_LOCK *LockList,
    IN ULONG Count,
    IN KIRQL Irql,
    OUT KIRQL *PrevIrql)
{
    while (!KeTryAcquireSpinlockMultipleRaiseIrql(LockList, Count, Irql, PrevIrql))
        _mm_pause();
}

/**
 * @brief Releases the spinlocks with reverse order and restores the IRQL.
 * 
 * @param [in] LockList     Array of spinlock.
 * @param [in] Count        Spinlock count. This parameter cannot be zero.
 * @param [in] PrevIrql     Previous IRQL.
 * 
 * @return None.
 */
VOID
KERNELAPI
KeReleaseSpinlockMultipleLowerIrql(
    IN PKSPIN_LOCK *LockList,
    IN ULONG Count,
    IN KIRQL PrevIrql)
{
    if (!Count)
    {
        FATAL("Zero count cannot be accepted");
    }

    while (Count)
    {
        KeReleaseSpinlock(LockList[Count - 1]);
        Count--;
    }

    KeLowerIrql(PrevIrql);
}

