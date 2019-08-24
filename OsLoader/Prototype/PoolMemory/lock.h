#pragma once

#pragma pack(push, 4)

typedef struct _KSPIN_LOCK2 {
	U32 Lock;
} KSPIN_LOCK2, *PKSPIN_LOCK2;

#pragma pack(pop)

VOID
KERNELAPI
KeInitializeSpinLock(
	IN PKSPIN_LOCK2 Lock);

VOID
KERNELAPI
KeAcquireSpinLock(
	IN PKSPIN_LOCK2 Lock);

VOID
KERNELAPI
KeReleaseSpinLock(
	IN PKSPIN_LOCK2 Lock);

BOOLEAN
KERNELAPI
KeIsSpinLockAcquired(
	IN PKSPIN_LOCK2 Lock);

