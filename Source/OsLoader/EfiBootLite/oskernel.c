
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
	IMAGE_NT_HEADERS_3264 *Nt3264;
	EFI_PHYSICAL_ADDRESS KernelBase;
	PKERNEL_START_ENTRY StartEntry;

	KernelBase = LoaderBlock->LoaderData.KernelPhysicalBase;
	Nt3264 = OslPeImageBaseToNtHeaders((void *)KernelBase);

	if (!Nt3264)
		return FALSE;

	StartEntry = (PKERNEL_START_ENTRY)(
		KernelBase + Nt3264->Nt64.OptionalHeader.AddressOfEntryPoint);

	// Start the kernel!
	StartEntry(KernelBase, (UINT64)LoaderBlock, sizeof(OS_LOADER_BLOCK), 0);

	OslReturnedFromKernel();

	return FALSE;
}
