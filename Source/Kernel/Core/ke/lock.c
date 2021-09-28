
/**
 * @file lock.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements simple spinlock.
 * @version 0.1
 * @date 201?-??-??
 * 
 * @copyright Copyright (c) 2021
 * 
 * @todo Implement interrupt disable when acquire, enable when release.\n
 *       Current implementation does not disable the interrupt.\n
 *       On x64, this can be done by setting LAPIC.TPR to higher value or clearing RFLAGS.IF.\n
 */

#include <base/base.h>
#include <ke/lock.h>

VOID
KERNELAPI
KeInitializeSpinLock(
	IN PKSPIN_LOCK Lock)
{
	// ASSERT_ALIGN(Lock, 4)
	Lock->Lock = 0;
}

VOID
KERNELAPI
KeAcquireSpinLock(
	IN PKSPIN_LOCK Lock)
{
	// ASSERT_ALIGN(Lock, 4)
	while (_InterlockedExchange((volatile long *)&Lock->Lock, 1))
		_mm_pause();
}

VOID
KERNELAPI
KeReleaseSpinLock(
	IN PKSPIN_LOCK Lock)
{
	// ASSERT_ALIGN(Lock, 4)
	// ASSERT(
	_InterlockedExchange((volatile long *)&Lock->Lock, 0);
	// == 1 );
}

BOOLEAN
KERNELAPI
KeIsSpinLockAcquired(
	IN PKSPIN_LOCK Lock)
{
	return (BOOLEAN)(Lock->Lock != 0);
}
