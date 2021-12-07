
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
#include <ke/ke.h>
#include <mm/pool.h>
#include <mm/mminit.h>
#include <mm/paging.h>
#include <hal/halinit.h>


OS_LOADER_BLOCK PiLoaderBlockTemporary;
ZIP_CONTEXT PiBootImageContext; // Boot image context

/**
 * @brief Do the pre-initialization.
 * 
 * @param [in] LoaderBlock          Loader block.
 * @param [in] LoaderBlockSize      Size of loader block.
 * 
 * @return None. 
 */
VOID
KERNELAPI
PiPreInitialize(
    IN OS_LOADER_BLOCK *LoaderBlock,
    IN U32 LoaderBlockSize)
{
    DbgTraceF(TraceLevelDebug, "%s (%p, %X)\n", __FUNCTION__, LoaderBlock, LoaderBlockSize);

    if (sizeof(*LoaderBlock) != LoaderBlockSize)
    {
        FATAL("Loader block size mismatch\n");
    }

    OS_LOADER_BLOCK *LoaderBlockTemp = &PiLoaderBlockTemporary;
    *LoaderBlockTemp = *LoaderBlock;

    U64 OffsetToVirtualBase = LoaderBlockTemp->LoaderData.OffsetToVirtualBase;

    LoaderBlockTemp->Base.BootServices = NULL;
    LoaderBlockTemp->Base.ImageHandle = NULL;
    LoaderBlockTemp->Base.RootDirectory = NULL;


    //
    // Initialize the boot image context.
    //

    if (!ZipInitializeReaderContext(&PiBootImageContext,
        (PVOID)(LoaderBlockTemp->LoaderData.BootImageBase + OffsetToVirtualBase),
        (U32)LoaderBlockTemp->LoaderData.BootImageSize))
    {
        FATAL("Invalid boot image");
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
        (PVOID)(LoaderBlockTemp->LoaderData.VideoFramebufferCopy + OffsetToVirtualBase),
        LoaderBlockTemp->LoaderData.VideoFramebufferSize,
        &PiBootImageContext))
    {
        FATAL("Failed to initialize graphics");
    }

    BGXTRACE_C(
        BGX_COLOR_LIGHT_GREEN,
        "Pseudo kernel v%d.%d. (Last built: %s %s)\n\n",
        0, 1, __DATE__, __TIME__);

    BGXTRACE("Pre-init graphics initialized\n");

    //
    // Initialize the pre-init pool and XAD trees.
    //

    ESTATUS Status = MiPreInitialize(LoaderBlockTemp);

    if (!E_IS_SUCCESS(Status))
    {
        FATAL("Failed to initialize memory (0x%08x)", Status);
    }

    //
    // Set framebuffer attributes to WC.
    // This greatly improves copy speed (compared to UC).
    //

    MiArchX64SetPageAttribute(MiPML4TBase, MiRPML4TBase, 
        LoaderBlockTemp->LoaderData.VideoFramebufferBase, 
        LoaderBlockTemp->LoaderData.VideoFramebufferSize, 
        ARCH_X64_PAT_WRITE_COMBINING);


    //
    // Dump XADs.
    //

    MiPreDumpXad();


    //
    // Read and vaildate ACPI table.
    //

    HalPreInitialize((PVOID)LoaderBlockTemp->Configuration.AcpiTable);
}

