
/**
 * @file preinit.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Impelements pre-initialization routines.
 * @version 0.1
 * @date 2021-09-12
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <base/base.h>
#include <init/preinit.h>
#include <init/bootgfx.h>
#include <init/zip.h>
#include <mm/pool.h>
#include <mm/mminit.h>


OS_LOADER_BLOCK PiLoaderBlockTemporary;
PREINIT_PAGE_RESERVE PiPageReserve[32]; // Page list which must not be discarded
U32 PiPageReserveCount = 0;

ZIP_CONTEXT PiBootImageContext; // Boot image context


VOID
KERNELAPI
PiPreInitialize(
    IN OS_LOADER_BLOCK *LoaderBlock,
    IN U32 LoaderBlockSize)
{
    DbgTraceF(TraceLevelDebug, "%s (%p, %X)\n", __FUNCTION__, LoaderBlock, LoaderBlockSize);

    if (sizeof(*LoaderBlock) != LoaderBlockSize)
    {
        BootGfxFatalStop("Loader block size mismatch\n");
    }

    OS_LOADER_BLOCK *LoaderBlockTemp = &PiLoaderBlockTemporary;
    *LoaderBlockTemp = *LoaderBlock;

    U64 OffsetToVirtualBase = LoaderBlockTemp->LoaderData.OffsetToVirtualBase;

    LoaderBlockTemp->Base.BootServices = NULL;
    LoaderBlockTemp->Base.ImageHandle = NULL;
    LoaderBlockTemp->Base.RootDirectory = NULL;

    //
    // Initialize the pre-init pool and XAD trees.
    //

    ESTATUS Status = MiPreInitialize(LoaderBlockTemp);

    if (!E_IS_SUCCESS(Status))
    {
        BootGfxFatalStop("Failed to initialize memory");
    }

    //
    // Initialize the boot image context.
    //

    if (!ZipInitializeReaderContext(&PiBootImageContext,
        (PVOID)(LoaderBlockTemp->LoaderData.BootImageBase + OffsetToVirtualBase),
        (U32)LoaderBlockTemp->LoaderData.BootImageSize))
    {
        BootGfxFatalStop("Invalid boot image");
    }

    //
    // Initialize the pre-init graphics.
    // Note that we use physical address of framebuffer because it is identity mapped.
    //

    if (!BootGfxInitialize(
        (PVOID)(LoaderBlockTemp->LoaderData.VideoModeBase + OffsetToVirtualBase),
        (U32)LoaderBlockTemp->LoaderData.VideoModeSize,
        LoaderBlockTemp->LoaderData.VideoModeSelected,
        (PVOID)LoaderBlockTemp->LoaderData.VideoFramebufferBase,
        LoaderBlockTemp->LoaderData.VideoFramebufferSize,
        &PiBootImageContext))
    {
        BootGfxFatalStop("Failed to initialize graphics");
    }

    //
    // Dump XADs.
    //

    MiPreDumpXad();

    BootGfxPrintTextFormat("System Halt.");

    for (;;)
    {
        __PseudoIntrin_DisableInterrupt();
        __PseudoIntrin_Halt();
    }

//  if (!PiDiscardFirmwareMemory())
//  {
//      DbgTraceF(TraceLevelError, "Failed to free unused memory\n");
//      return FALSE;
//  }
}

