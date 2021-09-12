
/**
 * @file oskernel.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements transfer-to-kernel routines.
 * @version 0.1
 * @date 2019-08-24
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "OsLoader.h"
#include "ospeimage.h"
#include "osmemory.h"
#include "oskernel.h"


/**
 * @brief Halts the system.
 * 
 * @return None.
 */
VOID
EFIAPI
OslReturnedFromKernel(
    VOID)
{
    for (;;)
    {
        __asm__ __volatile__("hlt" : : : "memory");
    }
}

/**
 * @brief Transfers control to the kernel.
 * 
 * @param [in] LoaderBlock  The loader block which contains OS loader information.
 * 
 * @return This function only returns FALSE if kernel is not loaded.
 */
BOOLEAN
EFIAPI
OslTransferToKernel(
    IN OS_LOADER_BLOCK *LoaderBlock)
{
    // Set our page mapping. This will invalidate the TLB.
    OslArchX64SetCr3(LoaderBlock->LoaderData.PML4TBase);

    EFI_VIRTUAL_ADDRESS KernelBase = 
        LoaderBlock->LoaderData.KernelPhysicalBase
        + LoaderBlock->LoaderData.OffsetToVirtualBase;
    UINT64 *KernelStackTop = (UINT64 *)(
        LoaderBlock->LoaderData.KernelStackBase
        + LoaderBlock->LoaderData.OffsetToVirtualBase
        + LoaderBlock->LoaderData.StackSize);

    IMAGE_NT_HEADERS_3264 *Nt3264 = OslPeImageBaseToNtHeaders((void *)KernelBase);

    if (!Nt3264)
        return FALSE;

    PKERNEL_START_ENTRY StartEntry = (PKERNEL_START_ENTRY)(
        KernelBase + Nt3264->Nt64.OptionalHeader.AddressOfEntryPoint);
    UINT64 *StackPointer = KernelStackTop - 5;

    UINT32 LoaderBlockSize = sizeof(*LoaderBlock);

    __asm__ __volatile__
    (
        "mov rax, %0\n\t"
        "mov r10, %1\n\t"
        "mov rcx, %2\n\t"
        "mov rdx, %3\n\t"
        "mov r8d, %4\n\t"
        "mov r9, %5\n\t"
        "xchg rsp, r10\n\t" // stack switch!
        "push rax\n\t"
        "ret\n\t"
        :
        : "m"(StartEntry), "m"(StackPointer),
          "m"(KernelBase), "m"(LoaderBlock), "m"(LoaderBlockSize), "n"(0)
        :
    );

    OslReturnedFromKernel();

    return FALSE;
}
