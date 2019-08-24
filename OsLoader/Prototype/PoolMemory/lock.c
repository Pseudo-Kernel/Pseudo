
#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <Windows.h>
#include "Arch\Arch.h"

#include "lock.h"

VOID
KERNELAPI
KeInitializeSpinLock(
	IN PKSPIN_LOCK2 Lock)
{
	// ASSERT_ALIGN(Lock, 4)
	Lock->Lock = 0;
}

VOID
KERNELAPI
KeAcquireSpinLock(
	IN PKSPIN_LOCK2 Lock)
{
	// ASSERT_ALIGN(Lock, 4)
	while (_InterlockedExchange((volatile U32 *)&Lock->Lock, 1))
		_mm_pause();
}

VOID
KERNELAPI
KeReleaseSpinLock(
	IN PKSPIN_LOCK2 Lock)
{
	// ASSERT_ALIGN(Lock, 4)
	// ASSERT(
	_InterlockedExchange((volatile U32 *)&Lock->Lock, 0);
	// == 1 );
}

BOOLEAN
KERNELAPI
KeIsSpinLockAcquired(
	IN PKSPIN_LOCK2 Lock)
{
	return (BOOLEAN)(Lock->Lock != 0);
}
