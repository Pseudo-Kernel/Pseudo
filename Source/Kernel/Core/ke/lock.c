
#include <Base.h>
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
	while (__PseudoIntrin_InterlockedExchange32((volatile __int32 *)&Lock->Lock, 1))
		__PseudoIntrin_Pause();
}

VOID
KERNELAPI
KeReleaseSpinLock(
	IN PKSPIN_LOCK Lock)
{
	// ASSERT_ALIGN(Lock, 4)
	// ASSERT(
	__PseudoIntrin_InterlockedExchange32((volatile __int32 *)&Lock->Lock, 0);
	// == 1 );
}

BOOLEAN
KERNELAPI
KeIsSpinLockAcquired(
	IN PKSPIN_LOCK Lock)
{
	return (BOOLEAN)(Lock->Lock != 0);
}
