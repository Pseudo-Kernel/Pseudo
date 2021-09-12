
/**
 * @file uefimain.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements UEFI Osloader main routine.
 * @version 0.1
 * @date 2021-09-10
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "OsLoader.h"
#include "osmisc.h"
#include "osdebug.h"
#include "osmemory.h"
#include "ospeimage.h"
#include "oskernel.h"


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
EFI_GUID gEfiRngProtocolGuid = { 0x3152bca5, 0xeade, 0x433d, {0x86, 0x2e, 0xc0, 0x1c, 0xdc, 0x29, 0x1f, 0x44} };


EFI_GUID gEfiAcpi20TableGuid = EFI_ACPI_20_TABLE_GUID;


OS_LOADER_BLOCK OslLoaderBlock;

EFI_HANDLE gImageHandle = NULL;
EFI_SYSTEM_TABLE *gST = NULL;
EFI_BOOT_SERVICES *gBS = NULL;
EFI_RUNTIME_SERVICES *gRS = NULL;
EFI_SIMPLE_TEXT_INPUT_PROTOCOL *gConIn = NULL;
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *gConOut = NULL;



/**
 * @brief Initializes the loader block.
 * 
 * @param [out] LoaderBlock Pointer to loader block to be initialized.
 * @param [in] ImageHandle  The firmware allocated handle for EFI image.
 * @param [in] SystemTable  Pointer to the EFI system table.
 * 
 * @return EFI_SUCCESS      The operation is completed successfully.
 * @return else             An error occurred during the operation.
 */
EFI_STATUS
EFIAPI
OslInitializeLoaderBlock(
    IN  OS_LOADER_BLOCK   *LoaderBlock,
    IN  EFI_HANDLE        ImageHandle,
    IN  EFI_SYSTEM_TABLE  *SystemTable)
{
    EFI_PHYSICAL_ADDRESS PreInitPoolBase = 0;
    EFI_PHYSICAL_ADDRESS ShadowBase = 0;
    EFI_PHYSICAL_ADDRESS KernelStackBase = 0;
    EFI_PHYSICAL_ADDRESS PxeInitPoolBase = 0;

    UINTN PreInitPoolSize = OSL_LOADER_PREINIT_POOL_SIZE;
    UINTN ShadowSize = OSL_LOADER_LOW_1M_SHADOW_SIZE;
    UINTN KernelStackSize = OSL_LOADER_KERNEL_STACK_SIZE;
    UINTN PxeInitPoolSize = OSL_LOADER_PXE_INIT_POOL_SIZE;

    EFI_STATUS Status = EFI_SUCCESS;

    TRACE(L"LoaderBlock 0x%p\r\n", LoaderBlock);

    LoaderBlock->Base.ImageHandle = ImageHandle;
    LoaderBlock->Base.SystemTable = SystemTable;
    LoaderBlock->Base.BootServices = SystemTable->BootServices;
    LoaderBlock->Base.RuntimeServices = SystemTable->RuntimeServices;

    Status = OslAllocatePages(PreInitPoolSize, &PreInitPoolBase, FALSE, OsPreInitPool);
    if (Status != EFI_SUCCESS)
        return Status;

    Status = OslAllocatePages(ShadowSize, &ShadowBase, FALSE, OsLowMemory1M);
    if (Status != EFI_SUCCESS)
        return Status;

    // Copy the shadow 1M (0x00000 to 0xfffff)
    for (UINTN Offset = 0; Offset < ShadowSize; Offset++)
        *((CHAR8 *)ShadowBase + Offset) = *((CHAR8 *)0 + Offset);

    Status = OslAllocatePages(KernelStackSize, &KernelStackBase, FALSE, OsKernelStack);
    if (Status != EFI_SUCCESS)
        return Status;

    Status = OslAllocatePages(PxeInitPoolSize, &PxeInitPoolBase, FALSE, OsPagingPxePool);
    if (Status != EFI_SUCCESS)
        return Status;

    // Get random number from RNG.
    Status = OslGetRandom(LoaderBlock->LoaderData.Random, sizeof(LoaderBlock->LoaderData.Random));
    if (Status != EFI_SUCCESS)
    {
        TRACE(L"WARNING: Cannot get random\r\n");
    }

    LoaderBlock->LoaderData.PreInitPoolBase = PreInitPoolBase;
    LoaderBlock->LoaderData.PreInitPoolSize = PreInitPoolSize;
    LoaderBlock->LoaderData.ShadowBase = ShadowBase;
    LoaderBlock->LoaderData.ShadowSize = ShadowSize;
    LoaderBlock->LoaderData.KernelStackBase = KernelStackBase;
    LoaderBlock->LoaderData.StackSize = KernelStackSize;
    LoaderBlock->LoaderData.PxeInitPoolBase = PxeInitPoolBase;
    LoaderBlock->LoaderData.PxeInitPoolSize = PxeInitPoolSize;
    LoaderBlock->LoaderData.PxeInitPoolSizeUsed = 0;
    LoaderBlock->LoaderData.PML4TBase = 0;
    LoaderBlock->LoaderData.RPML4TBase = 0;

    return EFI_SUCCESS;
}

/**
 * @brief Opens the boot partition.
 * 
 * @param [in] LoaderBlock  Loader block.
 * 
 * @return EFI_SUCCESS      The operation is completed successfully.
 * @return else             An error occurred during the operation.
 */
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
 * @brief Loads the file to memory.
 * 
 * @param [in] LoaderBlock      Loader block.
 * @param [in] FilePath         File path.
 * @param [in] BufferMemoryType Specifies memory type when the file buffer is allocated.\n
 *                              If zero is specified, EfiLoaderData will be used.
 * @param [out] Buffer          Caller-supplied pointer to receive file buffer.
 * @param [out] Size            Caller-supplied pointer to receive file size.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
EFIAPI
OslLoadFile(
    IN OS_LOADER_BLOCK *LoaderBlock, 
    IN CHAR16 *FilePath, 
    IN OS_MEMORY_TYPE BufferMemoryType,
    OUT VOID **Buffer, 
    OUT UINTN *Size)
{
    EFI_FILE *RootDirectory = LoaderBlock->Base.RootDirectory;
    EFI_FILE *TargetFile = NULL;
    EFI_STATUS Status = EFI_SUCCESS;

    UINTN FileBufferSize = 0;
    UINTN FileInfoSize = 0;
    EFI_PHYSICAL_ADDRESS FileBuffer = 0;
    EFI_PHYSICAL_ADDRESS FileInfo = 0;

    do
    {
        Status = RootDirectory->Open(RootDirectory, &TargetFile,
            FilePath, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);

        if (Status != EFI_SUCCESS)
        {
            TRACEF(L"Failed to Open the file (0x%lx)\r\n", (UINT64)Status);
            break;
        }

        // This returns required FileInfoSize with error.
        TargetFile->GetInfo(TargetFile, &gEfiFileInfoGuid, &FileInfoSize, NULL);

        Status = OslAllocatePages(FileInfoSize, &FileInfo, FALSE, OsTemporaryData);
        if (Status != EFI_SUCCESS)
            break;

        Status = TargetFile->GetInfo(TargetFile, &gEfiFileInfoGuid, &FileInfoSize, (void *)FileInfo);
        if (Status != EFI_SUCCESS)
            break;

        DTRACEF(LoaderBlock, L"File `%ls', Size 0x%lX\r\n",
            ((EFI_FILE_INFO *)FileInfo)->FileName,
            ((EFI_FILE_INFO *)FileInfo)->FileSize);

        FileBufferSize = ((EFI_FILE_INFO *)FileInfo)->FileSize;

        Status = OslAllocatePages(FileBufferSize, &FileBuffer, FALSE, BufferMemoryType);
        if (Status != EFI_SUCCESS)
        {
            TRACEF(L"Failed to Allocate the Memory (%d Pages)\r\n", EFI_SIZE_TO_PAGES(FileBufferSize));
            FileBuffer = 0;
            break;
        }

        Status = TargetFile->Read(TargetFile, &FileBufferSize, (void *)FileBuffer);
        if (Status != EFI_SUCCESS)
            break;

        if (FileBufferSize >= 0x100000000ULL)
            break;

        OslFreePages(FileInfo, FileInfoSize);

        *Buffer = (VOID *)FileBuffer;
        *Size = FileBufferSize;

        return TRUE;

    } while (FALSE);

    if (FileInfo)
        OslFreePages(FileInfo, FileInfoSize);

    if (FileBuffer)
        OslFreePages(FileBuffer, FileBufferSize);

    return FALSE;
}

/**
 * @brief Loads boot files.
 * 
 * @param [in] LoaderBlock  Loader block.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
EFIAPI
OslLoadBootFiles(
    IN OS_LOADER_BLOCK *LoaderBlock)
{
    UINTN FileBufferSize = 0;
    EFI_PHYSICAL_ADDRESS FileBuffer = 0;

    if (!OslLoadFile(LoaderBlock, L"Efi\\Boot\\Core.sys", OsTemporaryData, (VOID **)&FileBuffer, &FileBufferSize))
        return FALSE;

    // Calculate virtual base.
    UINT64 Random1 = *(UINT64 *)LoaderBlock->LoaderData.Random;
    UINT64 Random2 = *(UINT64 *)&LoaderBlock->LoaderData.Random[8];
    UINT32 Value1 = (UINT32)((Random1 * 0xc0c75f942f9aebca) >> 32);
    UINT32 Value2 = (UINT32)((Random2 * 0xfd74c63348a7c25f) >> 32);
    UINT64 Result = Value1 | ((UINT64)Value2 << 32);
    UINT64 VirtualBase = ((Result % KERNEL_VA_SIZE_LOADER_SPACE_ASLR_GAP) & ~(EFI_PAGE_SIZE - 1))
        + KERNEL_VA_START_LOADER_SPACE;

    UINTN MappedSize = 0;
    EFI_PHYSICAL_ADDRESS KernelPhysicalBase = OslPeLoadImage((void *)FileBuffer, FileBufferSize, &MappedSize, VirtualBase);
    OslFreePages(FileBuffer, FileBufferSize);

    if (!OslLoadFile(LoaderBlock, L"Efi\\Boot\\init.bin", OsBootImage, (VOID **)&FileBuffer, &FileBufferSize))
    {
        OslFreePages(KernelPhysicalBase, MappedSize);
        return FALSE;
    }

    LoaderBlock->LoaderData.OffsetToVirtualBase = VirtualBase - KernelPhysicalBase;
    LoaderBlock->LoaderData.KernelPhysicalBase = KernelPhysicalBase;
    LoaderBlock->LoaderData.MappedSize = MappedSize;
    LoaderBlock->LoaderData.BootImageBase = FileBuffer;
    LoaderBlock->LoaderData.BootImageSize = FileBufferSize;

    OslDbgWaitEnterKey(LoaderBlock, NULL);

    return TRUE;
}

/**
 * @brief Searches the configuration table by given GUID.
 * 
 * @param [in] Guid         The table identifier.
 * 
 * @return Non-zero table address if succeeds, zero otherwise.
 */
VOID *
EFIAPI
OslLookupConfigurationTable(
    IN EFI_GUID *Guid)
{
    UINTN i;

    for (i = 0; i < gST->NumberOfTableEntries; i++)
    {
        if (EfiIsEqualGuid(&gST->ConfigurationTable[i].VendorGuid, Guid))
        {
            return gST->ConfigurationTable[i].VendorTable;
        }
    }

    return NULL;
}

/**
 * @brief Gets the ACPI/SMBIOS configuration tables.
 * 
 * @param [in] LoaderBlock  Loader block.
 *  
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
EFIAPI
OslQueryConfigurationTables(
    IN OS_LOADER_BLOCK *LoaderBlock)
{
    //  EFI_GUID AcpiGuid = EFI_ACPI_TABLE_GUID;
    //  gEfiSmbios3TableGuid
    //  #error FIXME: Must implement query ACPI tables and SMBIOS tables!!!!

    VOID *AcpiTable = OslLookupConfigurationTable(&gEfiAcpi20TableGuid);

    LoaderBlock->Configuration.AcpiTable = (EFI_PHYSICAL_ADDRESS)AcpiTable;

    if(AcpiTable)
    {
        TRACE(L"Found ACPI table address (2.0 or higher) 0x%p\r\n", AcpiTable);
    }
    else
    {
        TRACE(L"Cannot find ACPI table\r\n");
    }

    return (BOOLEAN)(AcpiTable != NULL);
}

/**
 * @brief Evaulates the score of video mode.
 * 
 * @param [in] ModeInfo     Mode information.
 * 
 * @return Score of video mode. Higher is better.
 */
UINT32
EFIAPI
OslEvaluateVideoMode(
    IN EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *ModeInfo)
{
#ifdef OSLOADER_EFI_SCREEN_RESOLUTION_FIX_800_600 // 800x600 test resolution
    if (ModeInfo->HorizontalResolution == 800 && ModeInfo->VerticalResolution == 600)
        return 100;
    else
        return 0;
#else
    UINT32 Score = 0;
    UINT32 Ratio = 0;

    if (!ModeInfo)
        return 0;

    // We support pixel format RGBx8888 or BGRx8888 only.
    // Pixel - 32bpp, RGBx or BGRx (in byte-order).
    if (ModeInfo->PixelFormat != PixelRedGreenBlueReserved8BitPerColor &&
        ModeInfo->PixelFormat != PixelBlueGreenRedReserved8BitPerColor)
        return 0;

    // Assume no gaps between lines.
    if (ModeInfo->PixelsPerScanLine != ModeInfo->HorizontalResolution)
        return 0;

    // Minimum resolution must be >= 1024x768.
    // Maximum resolution must be < 2000x???.
    if (ModeInfo->HorizontalResolution < 1024 || ModeInfo->VerticalResolution < 768 || 
        ModeInfo->HorizontalResolution >= 2000)
        return 0;

    // H:V - 4:3 to 16:9 (1.25 ~ 1.778). 16:9 is better.
    Ratio = ModeInfo->HorizontalResolution * 1000 / ModeInfo->VerticalResolution;
    if (1250 <= Ratio && Ratio <= 1778)
    {
        Score = (Ratio - 1200) * (ModeInfo->HorizontalResolution - 1000);
    }

    return Score;
#endif
}

/**
 * @brief Queries the video mode information and performs mode switch.
 * 
 * @param [in] LoaderBlock  Loader block.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
EFIAPI
OslQuerySwitchVideoModes(
    IN OS_LOADER_BLOCK *LoaderBlock)
{
    EFI_GRAPHICS_OUTPUT_PROTOCOL *GfxOut;
    EFI_STATUS Status;

    UINTN HandleCount = 0;
    EFI_HANDLE *HandleBuffer = NULL;
    BOOLEAN Found = FALSE;
    EFI_PHYSICAL_ADDRESS VideoModeBuffer = 0;
    UINTN VideoModeBufferLength = 0;

    UINT32 SelectedMode = 0xffffffff;

    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiGraphicsOutputProtocolGuid, NULL, &HandleCount, &HandleBuffer);
    if (Status != EFI_SUCCESS)
    {
        TRACEF(L"Failed to LocateHandleBuffer\r\n");
        return FALSE;
    }

    if (!HandleCount || !HandleBuffer)
    {
        TRACEF(L"Failed to LocateHandleBuffer\r\n");
        return FALSE;
    }

    do
    {
        UINT32 Mode;
        UINT32 MaxMode = 0;
        UINTN ValidCount = 0;
        UINTN Offset = 0;
        UINT32 Score = 0;
        UINT32 MaxScore = 0;

        UINTN SizeOfInfo;
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *ModeInfo;
        UINT32 XResolution;
        UINT32 YResolution;
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION FillColor;

        Status = gBS->HandleProtocol(HandleBuffer[0], &gEfiGraphicsOutputProtocolGuid, (void **)&GfxOut);
        if (Status != EFI_SUCCESS)
        {
            TRACEF(L"Failed to HandleProtocol\r\n");
            break;
        }

        TRACE(L"Querying the Video Mode...\r\n");
        TRACE(L"MaxModeNumber 0x%X\r\n", GfxOut->Mode->MaxMode);
        TRACE(L"Current Mode Framebuffer 0x%p - 0x%p\r\n", 
            GfxOut->Mode->FrameBufferBase, 
            GfxOut->Mode->FrameBufferBase + GfxOut->Mode->FrameBufferSize - 1);

        MaxMode = GfxOut->Mode->MaxMode;

        //
        // Query the buffer size.
        // Evaluate the video modes.
        //

        for (Mode = 0; Mode <= MaxMode; Mode++)
        {
            if (GfxOut->QueryMode(GfxOut, Mode, &SizeOfInfo, &ModeInfo)
                == EFI_SUCCESS)
            {
                Score = OslEvaluateVideoMode(ModeInfo);
                if (MaxScore <= Score)
                {
                    SelectedMode = Mode;
                    MaxScore = Score;

                    XResolution = ModeInfo->HorizontalResolution;
                    YResolution = ModeInfo->VerticalResolution;
                }

                VideoModeBufferLength += (SizeOfInfo + sizeof(OS_VIDEO_MODE_RECORD));

                // Must free the buffer that QueryMode returned
                gBS->FreePool((void *)ModeInfo);
            }
        }

        if (SelectedMode > MaxMode)
        {
            TRACE(L"Failed to lookup supported video modes\r\n");
            break;
        }

        if (OslAllocatePages(VideoModeBufferLength, &VideoModeBuffer, FALSE, OsLoaderData)
            != EFI_SUCCESS)
            break;

        //
        // Build the video mode buffer.
        //

        for (Mode = 0; Mode <= MaxMode; Mode++)
        {
            if (GfxOut->QueryMode(GfxOut, Mode, &SizeOfInfo, &ModeInfo)
                == EFI_SUCCESS)
            {
                OS_VIDEO_MODE_RECORD *Record = (OS_VIDEO_MODE_RECORD *)(VideoModeBuffer + Offset);

                Record->Mode = Mode;
                Record->SizeOfInfo = (UINT32)SizeOfInfo;
                gBS->CopyMem((VOID *)(Record + 1), ModeInfo, SizeOfInfo);
                Offset += (SizeOfInfo + sizeof(OS_VIDEO_MODE_RECORD));

                TRACE(L" %c%2u: %4u x %4u, PixelsPerScanLine %4u, PixelFormat: ", 
                    (SelectedMode == Mode) ? '*' : ' ', 
                    Mode, 
                    ModeInfo->HorizontalResolution, 
                    ModeInfo->VerticalResolution, 
                    ModeInfo->PixelsPerScanLine);

                switch (ModeInfo->PixelFormat)
                {
                case PixelRedGreenBlueReserved8BitPerColor:
                    // x[31:24] B[23:16] G[15:8] R[7:0]
                    TRACE(L"RGBx8888");
                    break;
                case PixelBlueGreenRedReserved8BitPerColor:
                    // x[31:24] R[23:16] G[15:8] B[7:0]
                    TRACE(L"BGRx8888");
                    break;
                case PixelBitMask:
                    TRACE(L"BitMask (R:%u, G:%u, B:%u, x:%u)",
                        ModeInfo->PixelInformation.RedMask,
                        ModeInfo->PixelInformation.GreenMask,
                        ModeInfo->PixelInformation.BlueMask,
                        ModeInfo->PixelInformation.ReservedMask);
                    break;
                case PixelBltOnly:
                    TRACE(L"< Physical framebuffer not supported >");
                    break;
                default:
                    TRACE(L"< Unknown >");
                }

                TRACE(L"\r\n");

                // Must free the buffer that QueryMode returned
                gBS->FreePool((void *)ModeInfo);

                ValidCount++;

                if (!(ValidCount % 20))
                {
                    OslDbgWaitEnterKey(LoaderBlock, NULL);
                }
            }
        }

        TRACE(L"End of Video Modes.\r\n");
        TRACE(L"Switch to % 4u x % 4u...\r\n", XResolution, YResolution);
        OslDbgWaitEnterKey(LoaderBlock, NULL);

        if (GfxOut->SetMode(GfxOut, SelectedMode) != EFI_SUCCESS)
        {
            TRACE(L"Failed to set video mode\r\n");
            break;
        }

        TRACE(L"Mode switched to %4u x %4u\r\n", 
            GfxOut->Mode->Info->HorizontalResolution, 
            GfxOut->Mode->Info->VerticalResolution);
        TRACE(L"Framebuffer 0x%p - 0x%p\r\n",
            GfxOut->Mode->FrameBufferBase,
            GfxOut->Mode->FrameBufferBase + GfxOut->Mode->FrameBufferSize - 1);

        OslDbgWaitEnterKey(LoaderBlock, NULL);


        // Clear the screen once more.
        FillColor.Raw = 0;
        GfxOut->Blt(GfxOut, 
            &FillColor.Pixel, 
            EfiBltVideoFill, 
            0, 0, 0, 0,
            GfxOut->Mode->Info->HorizontalResolution,
            GfxOut->Mode->Info->VerticalResolution,
            0);

        LoaderBlock->LoaderData.VideoModeBase = VideoModeBuffer;
        LoaderBlock->LoaderData.VideoModeSize = VideoModeBufferLength;
        LoaderBlock->LoaderData.VideoMaxMode = MaxMode;
        LoaderBlock->LoaderData.VideoFramebufferBase = GfxOut->Mode->FrameBufferBase;
        LoaderBlock->LoaderData.VideoModeSelected = SelectedMode;
        LoaderBlock->LoaderData.VideoFramebufferSize = GfxOut->Mode->FrameBufferSize;

        Found = TRUE;
    } while (FALSE);

    if (HandleBuffer)
        gBS->FreePool((void *)HandleBuffer);

    if (!Found)
    {
        if(VideoModeBuffer)
            OslFreePages(VideoModeBuffer, VideoModeBufferLength);
    }

    return Found;
}


/**
 * @brief Entry point of Osloader.
 * 
 * @param [in] ImageHandle      Image handle.
 * @param [in] SystemTable      EFI system table.
 * 
 * @return EFI_STATUS code. Normally this routine does not return.
 */
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
    
    gConOut->SetAttribute(gConOut, 0x0f);
    TRACE(L"PseudoOS Loader for UEFI environment.\r\n");
    gConOut->SetAttribute(gConOut, 0x07);

    TRACE(L"Latest build date: %s \r\n\r\n", __DATE__);
    TRACE(L"ImageHandle 0x%p, SystemTable 0x%p\r\n", ImageHandle, SystemTable);

    //
    // Disable watchdog timer.
    //

    gBS->SetWatchdogTimer(0, 0, 0, NULL);

    //
    // Initialize our Loader Block.
    //

    Status = OslInitializeLoaderBlock(&OslLoaderBlock, ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS)
    {
        TRACEF(L"Failed to Initialize\r\n");
        gBS->Stall(5 * 1000 * 1000);
        return Status;
    }

    TRACE(L"Press the F1 Key to DebugTrace...\r\n");
    if (OslWaitForKeyInput(SCAN_F1, 0, 1 * 1000 * 1000))
    {
        TRACE(L"DBG: DebugTrace Enabled\r\n\r\n");
        OslLoaderBlock.Debug.DebugPrint = TRUE;
    }

#if 0
    #define ARG_HELPER(_s)      ASCII(_s), sizeof(_s)

    TRACE(L"%s = %u\r\n", ARG_HELPER(EFI_TABLE_HEADER));
    TRACE(L"%s = %u\r\n", ARG_HELPER(EFI_SYSTEM_TABLE));
    TRACE(L"%s = %u\r\n", ARG_HELPER(EFI_BOOT_SERVICES));
    TRACE(L"%s = %u\r\n", ARG_HELPER(EFI_RUNTIME_SERVICES));
    TRACE(L"%s = %u\r\n", ARG_HELPER(EFI_MEMORY_DESCRIPTOR));
    TRACE(L"%s = %u\r\n", ARG_HELPER(EFI_TIME));
    TRACE(L"%s = %u\r\n", ARG_HELPER(EFI_TIME_CAPABILITIES));
    TRACE(L"%s = %u\r\n", ARG_HELPER(EFI_CAPSULE_HEADER));
    TRACE(L"%s = %u\r\n", ARG_HELPER(EFI_GUID));
    TRACE(L"%s = %u\r\n", ARG_HELPER(EFI_CONFIGURATION_TABLE));
    TRACE(L"%s = %u\r\n", ARG_HELPER(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));

    TRACE(L"%s = %u\r\n", ARG_HELPER(OS_LOADER_BLOCK));

    OslDbgWaitEnterKey(&OslLoaderBlock, NULL);

    #undef ARG_HELPER
#endif

    //
    // Open the Boot Partition so that we can load the kernel image.
    //

    TRACE(L"Opening the Boot Partition...\r\n");

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
#if 1
    TRACE(L"Dumping Low 1M Area\r\n");

    OslDbgDumpLowMemoryToDisk(&OslLoaderBlock);
#endif

    TRACE(L"Loading boot files...\r\n");

    if (!OslLoadBootFiles(&OslLoaderBlock))
    {
        TRACEF(L"Failed to load boot files\r\n");
        gBS->Stall(5 * 1000 * 1000);
        return EFI_LOAD_ERROR;
    }

    //
    // Query the configuration tables.
    // Currently we use ACPI table only.
    //

    OslQueryConfigurationTables(&OslLoaderBlock);


    //
    // Print the System Memory Map.
    //

    TRACE(L"Query Memory Map...\r\n");

    Status = OslQueryMemoryMap(&OslLoaderBlock);
    if (Status != EFI_SUCCESS)
    {
        TRACEF(L"Failed to query memory map");
        SystemTable->BootServices->Stall(5 * 1000 * 1000);
        return Status;
    }

    OslDumpMemoryMap(
        OslLoaderBlock.Memory.Map,
        OslLoaderBlock.Memory.MapCount * OslLoaderBlock.Memory.DescriptorSize,
        OslLoaderBlock.Memory.DescriptorSize);

    //
    // Query the Graphics Mode.
    //
    // NOTE : DO NOT ASSUME that the VGA mode is available.
    //        Although UEFI supports text mode, it is emulated mode.
    //        (writing to 0xb8000 does not take an effect anymore.)
    //

    if (!OslQuerySwitchVideoModes(&OslLoaderBlock))
    {
        gBS->Stall(5 * 1000 * 1000);
        return EFI_NOT_STARTED;
    }

    //
    // Query the memory map once again because memory map may change by calling other UEFI service calls.
    //

    Status = OslQueryMemoryMap(&OslLoaderBlock);
    if (Status != EFI_SUCCESS)
    {
        TRACEF(L"Failed to query memory map");
        gBS->Stall(5 * 1000 * 1000);
        return Status;
    }

    //
    // Setup our paging structure.
    //

    if (!OslSetupPaging(&OslLoaderBlock))
    {
        TRACEF(L"Failed to setup paging structure");
        gBS->Stall(5 * 1000 * 1000);
        return EFI_NOT_STARTED;
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
        OslWaitForKeyInput(0, 0x0d, 0x00ffffffffffffff);

        return Status;
    }

    //
    // Transfer control to the kernel.
    //

    if (!OslTransferToKernel(&OslLoaderBlock))
    {
        return EFI_NOT_STARTED;
    }

    return EFI_ABORTED;
}

