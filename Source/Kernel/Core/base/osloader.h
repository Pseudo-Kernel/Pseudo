#pragma once

//
// OS Loader Block.
//

#pragma pack(push, 8)

typedef struct _OS_VIDEO_MODE_RECORD {
	U32 Mode;
	U32 SizeOfInfo;

	// Copy of returned buffer from EFI_GRAPHICS_OUTPUT_PROTOCOL.QueryMode
	// UCHAR ModeInfo[SizeOfInfo];
} OS_VIDEO_MODE_RECORD, *POS_VIDEO_MODE_RECORD;

typedef struct _OS_LOADER_BLOCK {
	struct
	{
		//
		// NOTE : Only the SystemTable and RuntimeServices are valid in kernel.
		//

		EFI_SYSTEM_TABLE *SystemTable;
		EFI_HANDLE ImageHandle;

		EFI_BOOT_SERVICES *BootServices;
		EFI_RUNTIME_SERVICES *RuntimeServices;

		//
		// Handle of the boot volume root directory.
		//

		VOID *RootDirectory;
	} Base;

	struct
	{
		//
		// NOTE : ... OS software must use the DescriptorSize to find the start of
		//        each EFI_MEMORY_DESCRIPTOR in the MemoryMap array.
		//        [UEFI Specification, Version 2.7 Errata A]
		//

		EFI_MEMORY_DESCRIPTOR *Map;
		UPTR MapCount;
		UPTR MapKey;
		UPTR DescriptorSize;
        UPTR MapSize;
		U32 DescriptorVersion;
	} Memory;

	struct
	{
        U8 Random[128];
        EFI_VIRTUAL_ADDRESS OffsetToVirtualBase;

 		EFI_PHYSICAL_ADDRESS PreInitPoolBase;
		SIZE_T PreInitPoolSize;

		EFI_PHYSICAL_ADDRESS ShadowBase; // Low 1M Shadow
		SIZE_T ShadowSize;

		EFI_PHYSICAL_ADDRESS KernelPhysicalBase;
		SIZE_T MappedSize;

		EFI_PHYSICAL_ADDRESS KernelStackBase;
		SIZE_T StackSize;

		EFI_PHYSICAL_ADDRESS BootImageBase;
		SIZE_T BootImageSize;

        EFI_PHYSICAL_ADDRESS PxeInitPoolBase;
        SIZE_T PxeInitPoolSize;
        SIZE_T PxeInitPoolSizeUsed;

        EFI_PHYSICAL_ADDRESS PML4TBase;
        EFI_PHYSICAL_ADDRESS RPML4TBase;

		//
		// Video Modes.
		// VideoModes = [VideoModeRecord1] [VideoModeRecord2] ... [VideoModeRecordN]
		// VideoModeRecord = [ModeNumber] [SizeOfInfo] [VideoMode]
		// UINT32 ModeNumber;
		// UINT32 SizeOfInfo;
		// UCHAR ModeInfo[SizeOfMode]; // Returned from EFI_GRAPHICS_OUTPUT_PROTOCOL.QueryMode
		//

		EFI_PHYSICAL_ADDRESS VideoModeBase;
		EFI_PHYSICAL_ADDRESS VideoFramebufferBase;
		UPTR VideoFramebufferSize;
		UPTR VideoModeSize;
		U32 VideoMaxMode;
		U32 VideoModeSelected;
	} LoaderData;

	struct
	{
		EFI_PHYSICAL_ADDRESS AcpiTable;
	} Configuration;

	struct
	{
		BOOLEAN DebugPrint;
	} Debug;
} OS_LOADER_BLOCK, *POS_LOADER_BLOCK;

#pragma pack(pop)

