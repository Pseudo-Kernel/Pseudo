
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

//    __asm__ __volatile__ ("hlt" : : : "memory");


    //
    // Initialize the boot image context.
    //

    DFOOTPRN(10);
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

    DFOOTPRN(11);
    if (!BootGfxInitialize(
        (PVOID)(LoaderBlockTemp->LoaderData.VideoModeBase + OffsetToVirtualBase),
        (U32)LoaderBlockTemp->LoaderData.VideoModeSize,
        LoaderBlockTemp->LoaderData.VideoModeSelected,
        (PVOID)LoaderBlockTemp->LoaderData.VideoFramebufferBase,
        (PVOID)(LoaderBlockTemp->LoaderData.VideoFramebufferCopy + OffsetToVirtualBase),
        LoaderBlockTemp->LoaderData.VideoFramebufferSize,
        &PiBootImageContext))
    {
        BootGfxFatalStop("Failed to initialize graphics");
    }

    BootGfxPrintTextFormat("Pre-init graphics initialized\n");

    //
    // Initialize the pre-init pool and XAD trees.
    //

    DFOOTPRN(12);
    ESTATUS Status = MiPreInitialize(LoaderBlockTemp);

    if (!E_IS_SUCCESS(Status))
    {
        BootGfxFatalStop("Failed to initialize memory");
    }

    //
    // Dump XADs.
    //

    DFOOTPRN(13);
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

