
/**
 * @file main.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Kernel main entry point.
 * @version 0.1
 * @date 2021-09-12
 * 
 * @copyright Copyright (c) 2021
 * 
 * @note Compiler & toolchain:\n
 *       - Compiler: gcc version 10.2.0 (GCC) [mingw-w64-x86_64-gcc (MSYS2)]\n
 *                   ('pacman -S mingw-w64-x86_64-gcc' to install)
 *       - Toolchain: [mingw-w64-x86_64-toolchain (MSYS2)]\n
 *                   ('pacman -S mingw-w64-x86_64-toolchain' to install)
 *       - Debugger: gdb version 8.1 (mingw-w64, x86_64-8.1.0-posix-seh-rt_v6-rev0)\n
 *                   (Recent gdb doesn't working with vscode/QEMU)
 * 
 * @todo There are many works to do:\n
 *       - Thread scheduling and load balancing
 *       - Waitable objects (timer, mutex, event, ...) and wait operation
 * 
 * @todo Current issues:\n
 *       - Kernel crashes randomly when interrupt is enabled [RESOLVED].\n
 *         Resolved by adding -mno-red-zone option.
 *       - Kernel crashes when handling arguments in variadic function (which uses sysv_abi) [RESOLVED].\n
 *         Looks like va_start() gives invalid result.\n
 *         Resolved by using ms_abi for variadic function.
 *       - Compilation fails with internal compiler error when compile with -O3 [RESOLVED].\n
 *         Resolved by using recent version of gcc (10.3.0)
 *       - AP initialization is not working when starting 3rd processor. (1st=BSP, 2nd=AP) [RESOLVED]\n
 *         Resolved. Turned out to be a simple bug in 16-bit AP stub.
 *       - BSP hangs in real machine while sending INIT-SIPI-SIPI to first AP. [RESOLVED]\n
 *         Looks like BSP hangs when writing ICR high/ICR low (first SIPI command).
 *       - Kernel crashes during AP initialization (Getting triple fault). [RESOLVED]\n
 *         Resolved. It was also a AP stub bug (sets rsp to invalid address). Same as BSP hang issue.
 */

#include <base/base.h>
#include <init/preinit.h>
#include <init/bootgfx.h>
#include <ke/ke.h>
#include <ke/kprocessor.h>
#include <mm/mminit.h>
#include <mm/pool.h>

#include <hal/halinit.h>
#include <hal/processor.h>
#include <hal/ptimer.h>


/**
 * @brief Kernel main entry point.
 * 
 * @param [in] LoadedBase           Kernel base.
 * @param [in] LoaderBlock          Loader block provided by Osloader.
 * @param [in] SizeOfLoaderBlock    Size of loader block.
 * @param [in] Reserved             Reserved. Currently zero.
 * 
 * @return This function never returns.
 */
U64
__attribute__((ms_abi))
KiKernelStart(
	IN PTR LoadedBase, 
	IN OS_LOADER_BLOCK *LoaderBlock, 
	IN U32 SizeOfLoaderBlock, 
	IN PTR Reserved)
{
	DbgInitialize(LoaderBlock, TraceLevelDebug);

	DbgTraceF(TraceLevelDebug, "%s (%p, %p, %X, %p)\n",
		__FUNCTION__, LoadedBase, LoaderBlock, SizeOfLoaderBlock, Reserved);

    PiPreInitialize(LoaderBlock, SizeOfLoaderBlock);

    MmInitialize();


    BGXTRACE_C(BGX_COLOR_LIGHT_YELLOW, "NonPagedPool/PagedPool allocation test\n");

    PVOID Test = NULL;
    U64 Tsc = __rdtsc();
    Test = MmAllocatePool(PoolTypeNonPaged, 0x1234, 0, 0);
    BGXTRACE_C(BGX_COLOR_LIGHT_YELLOW, 
        "NonPagedPool allocation result 0x%llx (took %lld cycles)\n", 
        Test, __rdtsc() - Tsc);

    if (Test)
    {
        MmFreePool(Test);
    }

    Tsc = __rdtsc();
    Test = MmAllocatePool(PoolTypePaged, 0x4321, 0, 0);
    BGXTRACE_C(BGX_COLOR_LIGHT_YELLOW, 
        "PagedPool allocation result 0x%llx (took %lld cycles)\n", 
        Test, __rdtsc() - Tsc);

    if (Test)
    {
        MmFreePool(Test);
    }

    BGXTRACE_C(BGX_COLOR_LIGHT_YELLOW, "Test done.\n");

    BGXTRACE_C(BGX_COLOR_LIGHT_YELLOW, "Initializing BSP...\n");

    KiInitialize();
    HalInitialize();

    //
    // Test!
    //

    static U32 test1[4] = { 0x11111111, 0x11111111, 0x11111111, 0x11111111 };
    static U32 test2[4] = { 0x22222222, 0x22222222, 0x22222222, 0x22222222 };
    volatile U32 test_r1[4];
    volatile U32 test_r2[4];

    __asm__ __volatile__ (
        "movdqu xmm0, xmmword ptr [%0]\n\t"
        "movdqu xmm1, xmmword ptr [%1]\n\t"
        :
        : "r"(test1), "r"(test2)
        : "memory"
    );

    for (U64 c = 0; ; c++)
    {
        __halt();

        for (U32 i = 0; i < KeGetProcessorCount(); i++)
        {
            HAL_PRIVATE_DATA *PrivateData = (HAL_PRIVATE_DATA *)KiProcessorBlocks[i]->HalPrivateData;
            BGXTRACE("P%d: %10d | ", i, PrivateData->ApicTickCount);
        }

        BGXTRACE("SystemTimerTick: %10lld\r", HalGetTickCount());

        __asm__ __volatile__ (
            "movdqa xmm2, xmm0\n\t"
            "movdqa xmm0, xmm1\n\t"
            "movdqa xmm1, xmm2\n\t"
            "movdqu xmmword ptr [%0], xmm0\n\t"
            "movdqu xmmword ptr [%1], xmm1\n\t"
            : "=m"(test_r1), "=m"(test_r2)
            : 
            : "memory"
        );

        if (c & 1)
        {
            DASSERT(!memcmp(test_r1, test1, 16) && !memcmp(test_r2, test2, 16));
        }
        else
        {
            DASSERT(!memcmp(test_r1, test2, 16) && !memcmp(test_r2, test1, 16));
        }

    }

	return 0;
}

