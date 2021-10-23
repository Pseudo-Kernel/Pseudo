
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
 *       - Interrupt registration and dispatch
 *       - Processor initialization (IOAPIC, LAPIC, per-processor data and tables)
 *       - Thread scheduling and load balancing
 *       - Synchronization primitives
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
 *       - BSP hangs in real machine while sending INIT-SIPI-SIPI to first AP.\n
 *         Looks like BSP hangs when writing ICR high/ICR low (first SIPI command).
 */

#include <base/base.h>
#include <init/preinit.h>
#include <init/bootgfx.h>
#include <ke/ke.h>
#include <mm/mminit.h>
#include <mm/pool.h>

#include <hal/halinit.h>
#include <hal/8254pit.h>
#include <hal/8259pic.h>


/**
 * @brief Kernel main entry point.
 * 
 * @param LoadedBase            Kernel base.
 * @param LoaderBlock           Loader block provided by Osloader.
 * @param SizeOfLoaderBlock     Size of loader block.
 * @param Reserved              Reserved. Currently zero.
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

    BGXTRACE_C(BGX_COLOR_LIGHT_YELLOW, "Initializing table registers...\n");

    KiInitialize();

    BGXTRACE_C(BGX_COLOR_LIGHT_YELLOW, "Done.\n\n");

    HalInitialize();

    BGXTRACE_C(BGX_COLOR_LIGHT_YELLOW, "Setting 8254 timer...\n");

    // Interrupt Test

    HalPicEnableInterrupt(~0, IRQL_TO_VECTOR_START(IRQL_LEGACY));
    Hal_8254Initialize();
    HalPicMaskInterrupt(~(1 << 0));

    _enable();

    for (U32 i = 0; ; i++)
    {
        __halt();
        BGXTRACE("Tick = %lld\r", Hal_8254GetTickCount());
//        BGXTRACE("Cnt = %d\n", i);
    }



    U64 Cr0 = 0, Cr2 = 0, Cr3 = 0, Cr4 = 0, Cr8 = 0;
    __asm__ __volatile__ (
        "mov %0, cr0\n\t"
        "mov %1, cr2\n\t"
        "mov %2, cr3\n\t"
        "mov %3, cr4\n\t"
        "mov %4, cr8\n\t"
        : "=r"(Cr0), "=r"(Cr2), "=r"(Cr3), "=r"(Cr4), "=r"(Cr8)
        :
        : "memory"
    );

    BGXTRACE_C(BGX_COLOR_LIGHT_YELLOW, 
        "CR0 = 0x%016llx, CR2 = 0x%016llx, CR3 = 0x%016llx,\n"
        "CR4 = 0x%016llx, CR8 = 0x%016llx\n",
        Cr0, Cr2, Cr3, Cr4, Cr8);

    BGXTRACE_C(BGX_COLOR_LIGHT_YELLOW, "System halt.\n");

	for (;;)
	{
		_disable();
		__halt();
	}

	return 0;
}

