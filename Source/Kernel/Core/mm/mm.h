
#pragma once

#include <base/base.h>
#include <mm/xadtree.h>


typedef enum _PAD_TYPE
{
	// Inherits EFI_MEMORY_TYPE.
	PadFirmwareReservedMemoryType = 0,
	PadFirmwareLoaderCode,
	PadFirmwareLoaderData,
	PadFirmwareBootServicesCode,
	PadFirmwareBootServicesData,
	PadFirmwareRuntimeServicesCode,
	PadFirmwareRuntimeServicesData,
	PadFirmwareConventionalMemory,				//!< Address is free to use.
	PadFree = PadFirmwareConventionalMemory,
	PadFirmwareUnusableMemory,
	PadFirmwareACPIReclaimMemory,
	PadFirmwareACPIMemoryNVS,
	PadFirmwareMemoryMappedIO,
	PadFirmwareMemoryMappedIOPortSpace,
	PadFirmwarePalCode,
	PadFirmwarePersistentMemory,

	PadInUse,									//!< Address is currently in use.

    PadMaximum,
} PAD_TYPE;

typedef enum _VAD_TYPE
{
    VadReserved,	//!< Address is reserved.
    VadFree,		//!< Address is free to use.
    VadInUse,		//!< Address is currently in use.
} VAD_TYPE;

typedef struct _PHYSICAL_ADDRESSES
{
    U32 PhysicalAddressCount;
    U32 PhysicalAddressMaximumCount;
    BOOLEAN Mapped;
    PTR StartingVirtualAddress;
	union
	{
		PVOID Internal;			//!< Reserved for internal use.
		ADDRESS_RANGE Range;	//!< Physical address range.
	} PhysicalAddresses[1];
} PHYSICAL_ADDRESSES;

//ESTATUS
//MmInitialize(
//	VOID);

KEXPORT
ESTATUS
MmAllocateVirtualMemory(
	IN OUT PTR *Address,
	IN SIZE_T Size,
	IN VAD_TYPE Type);

KEXPORT
ESTATUS
MmFreeVirtualMemory(
	IN PTR Address,
	IN SIZE_T Size);

KEXPORT
ESTATUS
MmAllocatePhysicalMemory(
	IN OUT PTR *Address,
	IN SIZE_T Size,
	IN PAD_TYPE Type);

KEXPORT
ESTATUS
MmFreePhysicalMemory(
	IN PTR Address,
	IN SIZE_T Size);

KEXPORT
ESTATUS
MmAllocatePhysicalMemoryGather(
	IN OUT PHYSICAL_ADDRESSES *PhysicalAddresses,
	IN SIZE_T Size,
	IN PAD_TYPE Type);
