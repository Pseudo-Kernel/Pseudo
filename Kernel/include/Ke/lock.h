#pragma once

#pragma pack(push, 4)

typedef struct _KSPIN_LOCK {
	U32 Lock;
} KSPIN_LOCK, *PKSPIN_LOCK;

#pragma pack(pop)

VOID
KERNELAPI
KeInitializeSpinLock(
	IN PKSPIN_LOCK Lock);

VOID
KERNELAPI
KeAcquireSpinLock(
	IN PKSPIN_LOCK Lock);

VOID
KERNELAPI
KeReleaseSpinLock(
	IN PKSPIN_LOCK Lock);

BOOLEAN
KERNELAPI
KeIsSpinLockAcquired(
	IN PKSPIN_LOCK Lock);

