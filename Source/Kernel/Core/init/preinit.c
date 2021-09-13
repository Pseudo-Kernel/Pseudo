
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


#define PREINIT_PAGE_RESERVE_ADD(_addr, _size)  \
{                                               \
    PiPageReserve[PiPageReserveCount].BaseAddress = (UPTR)(_addr);  \
    PiPageReserve[PiPageReserveCount].Size = (SIZE_T)(_size);       \
    PiPageReserveCount++;                                           \
}

#if 0
__attribute__((naked))
VOID
KERNELAPI
PiPreStackSwitch(
    IN OS_LOADER_BLOCK *LoaderBlock,
    IN U32 SizeOfLoaderBlock, 
    IN PVOID SafeStackTop)
{
    __asm__ __volatile__(
        "mov rcx, %0\n\t"
        "mov edx, %1\n\t"
        "mov r8, %2\n\t"
        "and r8, not 0x07\n\t"              // align to 16-byte boundary
        "sub r8, 0x20\n\t"                  // shadow area
        "mov qword ptr [r8], rcx\n\t"       // 1st param
        "mov qword ptr [r8+0x08], rdx\n\t"  // 2nd param
        "xchg r8, rsp\n\t"                  // switch the stack
        "call PiPreInitialize\n\t"
        "\n\t"                              // MUST NOT REACH HERE!
        "__InitFailed:\n\t"
        "cli\n\t"
        "hlt\n\t"
        "jmp __InitFailed\n\t"
        :
        : "r"(LoaderBlock), "r"(SizeOfLoaderBlock), "r"(SafeStackTop)
        : "memory"
    );
}
#endif

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

    if (!MiPreInitialize(LoaderBlockTemp))
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
    // Initialize the memory mapping.
    //

//  MiInitializeMemoryMap(
//      LoaderBlock->Memory.Map, 
//      LoaderBlock->Memory.MapCount, 
//      LoaderBlock->Memory.DescriptorSize, 
//      PiPageReserve, 
//      PiPageReserveCount);

    BootGfxPrintTextFormat("Test!\n");

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

