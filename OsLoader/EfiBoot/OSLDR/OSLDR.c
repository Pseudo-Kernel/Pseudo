/** @file
  Pseudo OS Loader Application for UEFI Environment.
**/

#include "OSLDR.h"

/**
 * TODO List:
 * 
 * [18-08-16]
 * 0. Set Architecture to x64 (Currently x86) 
 * -> Fallback to x86 because no emulation is supported [18-08-06]
 * -> Changed to x64, Using QEMU for test [18-08-12]
 * 1. Read the Kernel Image and Load it to the Page.
 * -> Reading Kernel Image Done, PE Relocation is in progress [18-08-15]
 *
 * [18-08-17]
 * 2. Initialize the 64-Bit Paging Structures.
 * [18-08-18]
 * 3. Relocate the Kernel Image to KernelVirtualBase.
 * 4. Get Firmware-Related Structures. (ACPI Table, SMBIOS Table, ...)
 * [18-08-19]
 * 5. Allocate the Kernel Stack.
 * 6. Get Memory Map and call ExitBootServices.
 * 7. Loader Block is completely initialized. Jump to the Kernel with Loader Block.
**/

// 
// Pseudo OS Virtual Memory Map:
//
// Start                 End                  Size                  Available?    Description
// 0x00000000`00000000 - 0x00000000`000fffff  0x00000000`00100000   N/A           Lower 1MB Area
// 0x00000000`00100000 - 0x000000ff`ffffffff  0x000000ff`fff00000   Yes(Kernel)   Shadow Lower 1TB
// 0x00001000`00000000 - 0x00001fff`ffffffff  0x00001000`00000000   Yes(User)     User Address Space
// 0xffff8000`00000000 - 0xfffffffe`ffffffff  0x7fff0000`00000000   Yes(Kernel)   Kernel Address Space
// + 0xffff8400`00000000 - 0xffff87ff`ffffffff  0x00000400`00000000   Yes(Kernel)   Kernel NonPaged Pool Area
// + 0xffff8800`00000000 - 0xffff8fff`ffffffff  0x00000400`00000000   Yes(Kernel)   Kernel Paged Pool Area
// + 0xffffe000`00000000 - 0xffffefff`ffffffff  0x00001000`00000000   Yes(Both)     User-Allocatable Shared Area
// + 0xffffffff`00000000 - 0xffffffff`ffffffff  0x00000001`00000000   N/A           Reserved for Future Use
// 

/**
  Allocates the loader block.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  @param[out] LoaderBlock   The pointer to which the address of loader block will be stored.
  
  @retval EFI_SUCCESS       The operation is completed successfully.
  @retval other             An error occurred during the operation.

**/
EFI_STATUS
EFIAPI
OslAllocateLoaderBlock (
	IN  EFI_HANDLE        ImageHandle,
	IN  EFI_SYSTEM_TABLE  *SystemTable,
	OUT OS_LOADER_BLOCK   **LoaderBlock
)
{
	OS_LOADER_BLOCK *LoaderBlockNew = NULL;
	EFI_STATUS Status = SystemTable->BootServices->AllocatePool(
		EfiLoaderData, sizeof(*LoaderBlock), (void **)&LoaderBlockNew);

	if (Status != EFI_SUCCESS)
	{
		TRACEF("Failed to allocate loader block, Status 0x%08x\n", Status);
		return Status;
	}

	SystemTable->BootServices->SetMem((void *)LoaderBlockNew, sizeof(*LoaderBlockNew), 0);

	LoaderBlockNew->Base.ImageHandle = ImageHandle;
	LoaderBlockNew->Base.SystemTable = SystemTable;
	LoaderBlockNew->Base.BootServices = SystemTable->BootServices;
	LoaderBlockNew->Base.RuntimeServices = SystemTable->RuntimeServices;

	TRACEF("Allocated LoaderBlock 0x%p\n", LoaderBlockNew);

	*LoaderBlock = LoaderBlockNew;

	return Status;
}


/**
  Opens the Boot Partition.

  @param[in] LoaderBlock    The loader block pointer.  
  
  @retval EFI_SUCCESS       The operation is completed successfully.
  @retval other             An error occurred during the operation.

**/
EFI_STATUS
EFIAPI
OslOpenBootPartition (
	IN OS_LOADER_BLOCK *LoaderBlock
)
{
	EFI_LOADED_IMAGE_PROTOCOL *LoadedImageProtocol = NULL;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystemProtocol = NULL;
	EFI_FILE *RootDirectory = NULL;
	EFI_STATUS Status = EFI_SUCCESS;

	do
	{
		Status = LoaderBlock->Base.BootServices->HandleProtocol(
			LoaderBlock->Base.ImageHandle,
			&gEfiLoadedImageProtocolGuid,
			(void **)&LoadedImageProtocol
		);

		if (Status != EFI_SUCCESS)
			break;

		Status = LoaderBlock->Base.BootServices->HandleProtocol(
			LoadedImageProtocol->DeviceHandle,
			&gEfiSimpleFileSystemProtocolGuid,
			(void **)&FileSystemProtocol
		);

		if (Status != EFI_SUCCESS)
			break;

		Status = FileSystemProtocol->OpenVolume(FileSystemProtocol, &RootDirectory);
	} while (FALSE);

	if (Status != EFI_SUCCESS)
	{
		// FIXME : Clean up if needed
		return Status;
	}

	LoaderBlock->Base.RootDirectory = RootDirectory;

	return EFI_SUCCESS;



#if 0
  UINTN HandleCount = 0;
  EFI_HANDLE *Handles = NULL;
  EFI_STATUS Status = EFI_SUCCESS;
  UINTN i = 0;
  
  Status = LoaderBlock->Base.BootServices->LocateHandleBuffer(
    ByProtocol, 
    &gEfiSimpleFileSystemProtocolGuid, 
    NULL, 
    &HandleCount, 
    &Handles
  );

  if (Status != EFI_SUCCESS)
    return NULL;


  // https://github.com/rpjohnst/kernel/blob/5e95b48d6e12b4cb03aa3c770160652a221ff085/src/boot.c#L27-L33

  for (i = 0; i < HandleCount; i++)
  {
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystemProtocol = NULL;
    EFI_DEVICE_PATH_PROTOCOL *DevicePathProtocol;
    EFI_HANDLE DeviceHandle;

    Status = LoaderBlock->Base.BootServices->HandleProtocol(
      Handles[i], 
      &gEfiSimpleFileSystemProtocolGuid, 
      (void **)&FileSystemProtocol
    );

    TRACE("SimpleFileProtocolHandle[%d] = 0x%x\n", i,  Handles[i]);

    LoaderBlock->Base.BootServices->LocateDevicePath(
      &gEfiDevicePathProtocolGuid, 
      &DevicePathProtocol, 
      &DeviceHandle
    );

//    DevicePathProtocol->Length
  }

  LoaderBlock->Base.BootServices->FreePool((void **)Handles);

//  LoaderBlock->Base.SystemTable->BootServices->OpenProtocol(
 //   LoaderBlock->Base.ImageHandle, 
    
//  )

  return NULL;

#endif
}

/**
  Loads the Kernel Image to Memory.

  @param[in] LoaderBlock    The loader block pointer.  
  
  @retval FALSE             An error occurred during the operation.
  @retval other             The operation is completed successfully.

**/
BOOLEAN
EFIAPI
OslLoadKernelImage (
	IN OS_LOADER_BLOCK *LoaderBlock
)
{
	EFI_FILE *RootDirectory = LoaderBlock->Base.RootDirectory;
	EFI_FILE *KernelImage = NULL;
	EFI_STATUS Status = EFI_SUCCESS;

	UINTN BufferSize = 0;
	void *FileBuffer = NULL;
	void *KernelPhysicalBase = NULL;

	do
	{
		Status = RootDirectory->Open(
			RootDirectory,
			&KernelImage,
			L"Efi\\Boot\\Core.sys",
			EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY
		);

		if (Status != EFI_SUCCESS)
		{
			TRACEF("Cannot open the Kernel Image\n");
			break;
		}

		Status = KernelImage->GetInfo(KernelImage, &gEfiFileInfoGuid, &BufferSize, NULL);
		if (Status != EFI_SUCCESS)
			break;

		Status = LoaderBlock->Base.BootServices->AllocatePool(EfiLoaderData, BufferSize, &FileBuffer);
		if (Status != EFI_SUCCESS)
		{
			FileBuffer = NULL;
			break;
		}

		Status = KernelImage->Read(KernelImage, &BufferSize, FileBuffer);
		if (Status != EFI_SUCCESS)
			break;

		if (BufferSize >= 0x100000000ULL)
			break;

		KernelPhysicalBase = (void *)OslPeLoadImage(LoaderBlock->Base.BootServices, FileBuffer, BufferSize);
	} while (FALSE);

	if (KernelPhysicalBase)
	{
		LoaderBlock->LoaderData.KernelPhysicalBase = KernelPhysicalBase;
	}

	return TRUE;
}


/**
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain(
	IN EFI_HANDLE        ImageHandle,
	IN EFI_SYSTEM_TABLE  *SystemTable
)
{
	OS_LOADER_BLOCK *LoaderBlock;
	EFI_STATUS Status;

	TRACEF("Hello in the UEFI-World!\n");

	Status = OslAllocateLoaderBlock(ImageHandle, SystemTable, &LoaderBlock);
	if (Status != EFI_SUCCESS)
	{
		TRACEF("Failed to allocate loader block");
		SystemTable->BootServices->Stall(5 * 1000 * 1000);
		return Status;
	}

	Status = OslOpenBootPartition(LoaderBlock);
	if (Status != EFI_SUCCESS)
	{
		TRACEF("Failed to open partition");
		SystemTable->BootServices->Stall(5 * 1000 * 1000);
		return Status;
	}

	if (!OslLoadKernelImage(LoaderBlock))
	{
		TRACEF("Failed to load kernel image");
		SystemTable->BootServices->Stall(5 * 1000 * 1000);
		return EFI_LOAD_ERROR;
	}

	Status = OslMmQueryMemoryMap(LoaderBlock);
	if (Status != EFI_SUCCESS)
	{
		TRACEF("Failed to query memory map");
		SystemTable->BootServices->Stall(5 * 1000 * 1000);
		return Status;
	}


	TRACEF("Wait for 3 Seconds...\n");
	SystemTable->BootServices->Stall(3 * 1000 * 1000);

	TRACEF("Now Exit...\n");
	SystemTable->BootServices->Stall(3 * 1000 * 1000);

	SystemTable->BootServices->ExitBootServices(ImageHandle, LoaderBlock->Memory.MapKey);

	return EFI_SUCCESS;
}
