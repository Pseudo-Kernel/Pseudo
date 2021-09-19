
#pragma once

#include <base/base.h>
#include <mm/xadtree.h>

typedef struct _XAD_CONTEXT
{
    BOOLEAN UsePreInitPool;
} XAD_CONTEXT;

extern MMXAD_TREE MiPadTree; //!< Physical address tree.
extern MMXAD_TREE MiVadTree; //!< Virtual address tree for shared kernel space.

extern XAD_CONTEXT MiXadContext;

extern BOOLEAN MiXadInitialized;


typedef enum _LOADER_XAD_TYPE
{
    OsSpecificMemTypeStart = (int)0x80000000,
    OsTemporaryData = OsSpecificMemTypeStart,
    OsLoaderData,
    OsLowMemory1M,
    OsKernelImage,
    OsKernelStack,
    OsBootImage,
    OsPreInitPool,
    OsPagingPxePool,
    OsFramebufferCopy,

    OsSpecificMemTypeEnd,
} LOADER_XAD_TYPE;

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
    PadInitialReserved,

	// Inherits OS_MEMORY_TYPE(LOADER_XAD_TYPE) in Osloader.
} PAD_TYPE;

typedef enum _VAD_TYPE
{
    VadReserved,	    //!< Address is reserved.
    VadInitialReserved, //!< Address is reserved for initial use.
    VadFree,		    //!< Address is free to use.
    VadInUse,		    //!< Address is currently in use.

	// Inherits OS_MEMORY_TYPE(LOADER_XAD_TYPE) in Osloader.
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

ESTATUS
MmReallocateVirtualMemory(
    IN PVOID ReservedZero,
    IN OUT PTR *Address,
    IN SIZE_T Size,
    IN VAD_TYPE SourceType,
    IN VAD_TYPE Type);

ESTATUS
MmAllocateVirtualMemory2(
    IN PVOID ReservedZero,
    IN OUT PTR *Address,
    IN SIZE_T Size,
    IN VAD_TYPE SourceType,
    IN VAD_TYPE Type);

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

ESTATUS
MmReallocatePhysicalMemory(
    IN OUT PTR *Address,
    IN SIZE_T Size,
    IN PAD_TYPE SourceType,
    IN PAD_TYPE Type);

ESTATUS
MmAllocatePhysicalMemory2(
    IN OUT PTR *Address,
    IN SIZE_T Size,
    IN PAD_TYPE SourceType,
    IN PAD_TYPE Type);

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
