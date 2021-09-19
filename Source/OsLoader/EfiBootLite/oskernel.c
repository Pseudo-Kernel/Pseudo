
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
#include "osdebug.h"


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
 * @brief Disables the interrupts by clearing the CR0.IF.
 * 
 * @return None.
 */
VOID
EFIAPI
OslDisableInterrupts(
    VOID)
{
    __asm__ __volatile__("cli" : : : "memory");
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
    // Disables the interrupts.
    OslDisableInterrupts();

    // Set our page mapping. This will invalidate the TLB.
    OslArchX64SetCr3(
        (OslArchX64GetCr3() & ~ARCH_X64_CR3_PML4_BASE_MASK) | 
        LoaderBlock->LoaderData.PML4TBase);

    EFI_VIRTUAL_ADDRESS KernelBase = 
        LoaderBlock->LoaderData.KernelPhysicalBase
        + LoaderBlock->LoaderData.OffsetToVirtualBase;
    UINT64 *KernelStackTop = (UINT64 *)(
        LoaderBlock->LoaderData.KernelStackBase
        + LoaderBlock->LoaderData.OffsetToVirtualBase
        + LoaderBlock->LoaderData.StackSize);

    OslDbgFillScreen(&OslLoaderBlock, 0xffffff);

    IMAGE_NT_HEADERS_3264 *Nt3264 = OslPeImageBaseToNtHeaders((void *)KernelBase);

    OslDbgFillScreen(&OslLoaderBlock, 0x000000);

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
        "call rax\n\t"
        :
        : "m"(StartEntry), "m"(StackPointer),
          "m"(KernelBase), "m"(LoaderBlock), "m"(LoaderBlockSize), "n"(0)
        :
    );

    OslReturnedFromKernel();

    return FALSE;
}
