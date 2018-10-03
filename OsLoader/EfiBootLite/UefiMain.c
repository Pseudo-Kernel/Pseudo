
//
// Must be compiled with /c, not /Za !!
//

#include "OsLoader.h"

EFI_GUID gEfiFileInfoGuid = { 0x09576E92, 0x6D3F, 0x11D2,{ 0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } };
EFI_GUID gEfiSimpleFileSystemProtocolGuid = { 0x964E5B22, 0x6459, 0x11D2,{ 0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } };
EFI_GUID gEfiDevicePathProtocolGuid = { 0x09576E91, 0x6D3F, 0x11D2,{ 0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } };
EFI_GUID gEfiLoadedImageProtocolGuid = { 0x5B1B31A1, 0x9562, 0x11D2,{ 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } };
EFI_GUID gEfiDebugPortProtocolGuid = { 0xEBA4E8D2, 0x3858, 0x41EC,{ 0xA2, 0x81, 0x26, 0x47, 0xBA, 0x96, 0x60, 0xD0 } };
EFI_GUID gEfiDriverBindingProtocolGuid = { 0x18A031AB, 0xB443, 0x4D1A,{ 0xA5, 0xC0, 0x0C, 0x09, 0x26, 0x1E, 0x9F, 0x71 } };
EFI_GUID gEfiSimpleTextOutProtocolGuid = { 0x387477C2, 0x69C7, 0x11D2,{ 0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } };
EFI_GUID gEfiGraphicsOutputProtocolGuid = { 0x9042A9DE, 0x23DC, 0x4A38,{ 0x96, 0xFB, 0x7A, 0xDE, 0xD0, 0x80, 0x51, 0x6A } };
EFI_GUID gEfiHiiFontProtocolGuid = { 0xe9ca4775, 0x8657, 0x47fc,{ 0x97, 0xe7, 0x7e, 0xd6, 0x5a, 0x08, 0x43, 0x24 } };
EFI_GUID gEfiUgaDrawProtocolGuid = { 0x982C298B, 0xF4FA, 0x41CB,{ 0xB8, 0x38, 0x77, 0xAA, 0x68, 0x8F, 0xB8, 0x39 } };

EFI_GUID gEfiAcpiTableGuid = EFI_ACPI_TABLE_GUID;


OS_LOADER_BLOCK OslLoaderBlock;

EFI_HANDLE gImageHandle = NULL;
EFI_SYSTEM_TABLE *gST = NULL;
EFI_BOOT_SERVICES *gBS = NULL;
EFI_RUNTIME_SERVICES *gRS = NULL;
EFI_SIMPLE_TEXT_INPUT_PROTOCOL *gConIn = NULL;
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *gConOut = NULL;

VOID
Trace(
	IN CHAR16 *Format,
	...)
{
	CHAR16 Buffer[PRINT_MAX_BUFFER + 1];
	VA_LIST List;
	UINTN Length;

	VA_START(List, Format);
	Length = StrFormatV(Buffer, ARRAY_SIZE(Buffer) - 1, Format, List);
	VA_END(List);

	StrTerminate(Buffer, ARRAY_SIZE(Buffer), Length);
	gConOut->OutputString(gConOut, Buffer);
}



/**
	Initializes the loader block.

	@param[in] ImageHandle    The firmware allocated handle for the EFI image.
	@param[in] SystemTable    A pointer to the EFI System Table.

	@retval EFI_SUCCESS       The operation is completed successfully.
	@retval other             An error occurred during the operation.
**/
EFI_STATUS
EFIAPI
OslInitializeLoaderBlock(
	IN  EFI_HANDLE        ImageHandle,
	IN  EFI_SYSTEM_TABLE  *SystemTable)
{
	OS_LOADER_BLOCK *LoaderBlock = &OslLoaderBlock;
	EFI_PHYSICAL_ADDRESS TempBase = 0xffff0000;
	const UINTN TempSize = 0x100000;

	EFI_STATUS Status;

	TRACEF(L"LoaderBlock 0x%p\r\n", LoaderBlock);

	SystemTable->BootServices->SetMem((void *)LoaderBlock, sizeof(*LoaderBlock), 0);

	LoaderBlock->Base.ImageHandle = ImageHandle;
	LoaderBlock->Base.SystemTable = SystemTable;
	LoaderBlock->Base.BootServices = SystemTable->BootServices;
	LoaderBlock->Base.RuntimeServices = SystemTable->RuntimeServices;

	Status = gBS->AllocatePages(AllocateMaxAddress,
		EfiLoaderData, EFI_SIZE_TO_PAGES(TempSize), &TempBase);
	if (Status != EFI_SUCCESS)
		return Status;

	SystemTable->BootServices->SetMem((void *)TempBase, TempSize, 0);

	LoaderBlock->LoaderData.TempBase = TempBase;
	LoaderBlock->LoaderData.TempSize = TempSize;

	return EFI_SUCCESS;
}


/**
	Opens the Boot Partition.

	@param[in] LoaderBlock    The loader block pointer.

	@retval EFI_SUCCESS       The operation is completed successfully.
	@retval other             An error occurred during the operation.

**/
EFI_STATUS
EFIAPI
OslOpenBootPartition(
	IN OS_LOADER_BLOCK *LoaderBlock)
{
	EFI_LOADED_IMAGE_PROTOCOL *LoadedImageProtocol = NULL;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystemProtocol = NULL;
	EFI_FILE *RootDirectory = NULL;
	EFI_STATUS Status = EFI_SUCCESS;

	do
	{
		Status = gBS->HandleProtocol(gImageHandle, &gEfiLoadedImageProtocolGuid,
			(void **)&LoadedImageProtocol);

		if (Status != EFI_SUCCESS)
			break;

		Status = gBS->HandleProtocol(LoadedImageProtocol->DeviceHandle,
			&gEfiSimpleFileSystemProtocolGuid, (void **)&FileSystemProtocol);

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
}


/**
	Loads the Kernel Image to Memory.

	@param[in] LoaderBlock    The loader block pointer.

	@retval FALSE             An error occurred during the operation.
	@retval other             The operation is completed successfully.

**/
BOOLEAN
EFIAPI
OslLoadKernelImage(
	IN OS_LOADER_BLOCK *LoaderBlock)
{
	EFI_FILE *RootDirectory = LoaderBlock->Base.RootDirectory;
	EFI_FILE *KernelImage = NULL;
	EFI_STATUS Status = EFI_SUCCESS;

	UINTN FileBufferSize = 0;
	EFI_PHYSICAL_ADDRESS FileBuffer = 0;
	EFI_PHYSICAL_ADDRESS KernelPhysicalBase = 0;
	UINTN MappedSize = 0;

	EFI_PHYSICAL_ADDRESS FileInfo = 0;
	UINTN FileInfoSize = 0;

	do
	{
		Status = RootDirectory->Open(RootDirectory, &KernelImage,
			L"Efi\\Boot\\Core.sys", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);

		if (Status != EFI_SUCCESS)
		{
			TRACEF(L"Failed to Open the Kernel Image\r\n");
			break;
		}

		// This returns required FileInfoSize with error.
		KernelImage->GetInfo(KernelImage, &gEfiFileInfoGuid, &FileInfoSize, NULL);

		Status = gBS->AllocatePages(
			AllocateAnyPages, EfiLoaderData, EFI_SIZE_TO_PAGES(FileInfoSize), &FileInfo);

		if (Status != EFI_SUCCESS)
			break;

		Status = KernelImage->GetInfo(KernelImage, &gEfiFileInfoGuid, &FileInfoSize, (void *)FileInfo);

		if (Status != EFI_SUCCESS)
			break;

		DTRACEF(LoaderBlock, L"Kernel Name `%ls', Size 0x%lX\r\n",
			((EFI_FILE_INFO *)FileInfo)->FileName,
			((EFI_FILE_INFO *)FileInfo)->FileSize);

		FileBufferSize = ((EFI_FILE_INFO *)FileInfo)->FileSize;


		Status = gBS->AllocatePages(
			AllocateAnyPages, EfiLoaderData, EFI_SIZE_TO_PAGES(FileBufferSize), &FileBuffer);
		if (Status != EFI_SUCCESS)
		{
			TRACEF(L"Failed to Allocate the Memory (%d Pages)\r\n", EFI_SIZE_TO_PAGES(FileBufferSize));
			FileBuffer = 0;
			break;
		}

		Status = KernelImage->Read(KernelImage, &FileBufferSize, (void *)FileBuffer);
		if (Status != EFI_SUCCESS)
			break;

		if (FileBufferSize >= 0x100000000ULL)
			break;

		KernelPhysicalBase = OslPeLoadImage((void *)FileBuffer, FileBufferSize, &MappedSize);
	} while (FALSE);

	if (FileInfo)
		gBS->FreePages(FileInfo, EFI_SIZE_TO_PAGES(FileInfoSize));

	if (FileBuffer)
		gBS->FreePages(FileBuffer, EFI_SIZE_TO_PAGES(FileBufferSize));

	if (!KernelPhysicalBase)
	{
		return FALSE;
	}

	LoaderBlock->LoaderData.KernelPhysicalBase = KernelPhysicalBase;
	LoaderBlock->LoaderData.MappedSize = MappedSize;

	if (IS_DEBUG_MODE(LoaderBlock))
	{
		DTRACEF(LoaderBlock, L"DBG: Press Enter Key to Continue\r\n");
		OslWaitForKeyInput(LoaderBlock, 0, 0x0d, 0x00ffffffffffffff);
	}

	return TRUE;
}

/**
	Dump the Low 1M Area to Disk.

	@param[in] LoaderBlock    The loader block pointer.

	@retval FALSE             An error occurred during the operation.
	@retval other             The operation is completed successfully.

**/
BOOLEAN
EFIAPI
OslDbgDumpLowMemoryToDisk(
	IN OS_LOADER_BLOCK *LoaderBlock)
{
	EFI_FILE *RootDirectory = LoaderBlock->Base.RootDirectory;
	EFI_FILE *DumpImage = NULL;
	EFI_STATUS Status = EFI_SUCCESS;

	UINTN BufferSize = 0;

	do
	{
		Status = RootDirectory->Open(
			RootDirectory,
			&DumpImage,
			L"Efi\\Boot\\Dump1M.bin",
			EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
			0
		);

		if (Status != EFI_SUCCESS)
		{
			TRACEF(L"Cannot create the dump file\r\n");
			break;
		}

		//
		// Dump Area : 0x00000000 - 0x000fffff.
		//

		BufferSize = 0x100000;

		Status = DumpImage->Write(DumpImage, &BufferSize, (void *)0x00000000);
		if (Status != EFI_SUCCESS || !BufferSize)
		{
			TRACEF(L"Cannot write to the dump file\r\n");
			break;
		}

		if (BufferSize != 0x100000)
		{
			TRACEF(L"Dump requested 0x%08x but only written 0x%08x - 0x%08x\r\n",
				0x100000, 0, BufferSize - 1);
		}

		//
		// Flush and close.
		//

		DumpImage->Flush(DumpImage);
		DumpImage->Close(DumpImage);

		return TRUE;

	} while (FALSE);

	return FALSE;
}

/**
	Query the following configuration tables.
	- ACPI table
	- SMBIOS table

	@param[in] LoaderBlock    The loader block pointer.

	@retval FALSE             An error occurred during the operation.
	@retval other             The operation is completed successfully.

**/
BOOLEAN
EFIAPI
OslQueryConfigurationTables(
	IN OS_LOADER_BLOCK *LoaderBlock)
{
	//	EFI_GUID AcpiGuid = EFI_ACPI_TABLE_GUID;
	//	gEfiSmbios3TableGuid
	//	#error FIXME: Must implement query ACPI tables and SMBIOS tables!!!!

	return FALSE;
}


/**
	Wait for Single Keystroke.

	@param[in] LoaderBlock      The loader block pointer.
	@param[in] ScanCode         ScanCode to wait.
	@param[in] UnicodeCharacter Unicode character. Used if ScanCode = 0.
	@param[in] Timeout          Timeout in microseconds.

	@retval FALSE             An timeout or error occurred.
	@retval other             The operation is completed successfully.

**/
BOOLEAN
EFIAPI
OslWaitForKeyInput(
	IN OS_LOADER_BLOCK *LoaderBlock,
	IN UINT16 ScanCode,
	IN CHAR16 UnicodeCharacter,
	IN UINT64 Timeout)
{
	EFI_INPUT_KEY Key;
	EFI_EVENT TimerEvent;
	EFI_STATUS Status;
	EFI_EVENT WaitEvents[2];
	BOOLEAN BreakLoop;
	BOOLEAN Result;

	Status = gBS->CreateEvent(EVT_TIMER, 0, NULL, NULL, &TimerEvent);

	if (Status != EFI_SUCCESS)
	{
		return FALSE;
	}

	// SetTimer requires 100ns-unit timeout.
	Status = gBS->SetTimer(TimerEvent, TimerRelative, Timeout * 10);

	if (Status != EFI_SUCCESS)
	{
		gBS->CloseEvent(TimerEvent);
		return FALSE;
	}

	WaitEvents[0] = TimerEvent;
	WaitEvents[1] = gConIn->WaitForKey;
	BreakLoop = Result = FALSE;

	do
	{
		UINTN Index;

		Status = gBS->WaitForEvent(ARRAY_SIZE(WaitEvents), WaitEvents, &Index);
		if (Status != EFI_SUCCESS)
		{
			break;
		}

		switch (Index)
		{
		case 0:
			// Timeout!
			BreakLoop = TRUE;
			break;
		case 1:
			// Key was pressed!
			gConIn->ReadKeyStroke(gConIn, &Key);
			// TRACEF("Key.ScanCode 0x%02X, Key.UCh 0x%04X\r\n", Key.ScanCode, Key.UnicodeChar);

			if (Key.ScanCode == ScanCode)
			{
				if (Key.ScanCode != 0 ||
					(Key.ScanCode == 0 && Key.UnicodeChar == UnicodeCharacter))
				{
					BreakLoop = TRUE;
					Result = TRUE;
				}
			}
			break;
		}
	} while (!BreakLoop);

	gBS->CloseEvent(TimerEvent);

	return Result;
}

/**
	Entry point of the UEFI application.

	@param[in] ImageHandle    This EFI image handle.
	@param[in] SystemTable    A pointer to the EFI System Table.

	@retval EFI_SUCCESS       The entry point is executed successfully.
	@retval other             An error occured.

**/
EFI_STATUS
EFIAPI
UefiMain(
	IN EFI_HANDLE        ImageHandle,
	IN EFI_SYSTEM_TABLE  *SystemTable)
{
	EFI_STATUS Status;

	//
	// Initialize the global structures.
	//

	gImageHandle = ImageHandle;
	gST = SystemTable;
	gConIn = gST->ConIn;
	gConOut = gST->ConOut;
	gBS = SystemTable->BootServices;
	gRS = SystemTable->RuntimeServices;


	TRACEF(L"Hello in the UEFI-World!!\r\n");
	TRACEF(L"ImageHandle 0x%p, SystemTable 0x%p\r\n", ImageHandle, SystemTable);

	//
	// Disable watchdog timer.
	//

	gBS->SetWatchdogTimer(0, 0, 0, NULL);

	//
	// Initialize our Loader Block.
	//

	Status = OslInitializeLoaderBlock(ImageHandle, SystemTable);
	if (Status != EFI_SUCCESS)
	{
		TRACEF(L"Failed to Initialize\r\n");
		gBS->Stall(5 * 1000 * 1000);
		return Status;
	}

	TRACE(L"Press the F1 Key to DebugTrace...\r\n");
	if (OslWaitForKeyInput(&OslLoaderBlock, SCAN_F1, 0, 1 * 1000 * 1000))
	{
		TRACE(L"DBG: DebugTrace Enabled\r\n\r\n");
		OslLoaderBlock.Debug.DebugPrint = TRUE;
	}

	//
	// Open the Boot Partition so that we can load the kernel image.
	//

	TRACEF(L"Opening the Boot Partition...\r\n");

	Status = OslOpenBootPartition(&OslLoaderBlock);
	if (Status != EFI_SUCCESS)
	{
		TRACEF(L"Failed to open partition\r\n");
		gBS->Stall(5 * 1000 * 1000);
		return Status;
	}

	//
	// Dump low 1M area to file so that user may check the INT 10H capability manually.
	//

	TRACEF(L"Dumping Low 1M Area\r\n");

	OslDbgDumpLowMemoryToDisk(&OslLoaderBlock);

	TRACEF(L"Loading Kernel...\r\n");

	if (!OslLoadKernelImage(&OslLoaderBlock))
	{
		TRACEF(L"Failed to load kernel image\r\n");
		gBS->Stall(5 * 1000 * 1000);
		return EFI_LOAD_ERROR;
	}


	//
	// Query the configuration tables.
	// Currently we use ACPI table only.
	//

//	EfiGetSystemConfigurationTable(&AcpiTableGuid, (void **)&OslLoaderBlock.Configuration.AcpiTable);

	//
	// Print the System Memory Map.
	//

	TRACEF(L"Query Memory Map...\r\n");

	Status = OslMmQueryMemoryMap(&OslLoaderBlock);
	if (Status != EFI_SUCCESS)
	{
		TRACEF(L"Failed to query memory map");
		SystemTable->BootServices->Stall(5 * 1000 * 1000);
		return Status;
	}

	OslMmDumpMemoryMap(
		OslLoaderBlock.Memory.Map,
		OslLoaderBlock.Memory.MapCount * OslLoaderBlock.Memory.DescriptorSize,
		OslLoaderBlock.Memory.DescriptorSize);

	//
	// FIXME : Query the Graphics Mode.
	//
	// NOTE : DO NOT ASSUME that the VGA mode is available.
	//        It seems that some machines don't support the legacy VGA mode
	//        (And physical address 0xb8000 is not valid anymore)
	//


	gConOut->SetMode(gConOut, 0);
	gConOut->ClearScreen(gConOut);

	//	SystemTable->BootServices->OpenProtocol


	//
	// Query the memory map once again because memory map must be changed by other calls.
	//

	Status = OslMmQueryMemoryMap(&OslLoaderBlock);
	if (Status != EFI_SUCCESS)
	{
		TRACEF(L"Failed to query memory map");
		gBS->Stall(5 * 1000 * 1000);
		return Status;
	}

	//
	// Exit the Boot Services.
	// Any service calls must be prohibited because it can change the memory map.
	//

	Status = gBS->ExitBootServices(ImageHandle, OslLoaderBlock.Memory.MapKey);

	if (Status != EFI_SUCCESS)
	{
		TRACEF(L"ExitBootServices() failed (Status 0x%lX)\r\n", Status);

		TRACEF(L"Press Enter Key to Exit\r\n");
		OslWaitForKeyInput(&OslLoaderBlock, 0, 0x0d, 0x00ffffffffffffff);

		return Status;
	}

	if (!OslTransferToKernel(&OslLoaderBlock))
	{
		return EFI_NOT_STARTED;
	}

	return EFI_SUCCESS;
}

