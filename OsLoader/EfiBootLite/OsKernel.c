/** @file
	Transfer-to-Kernel Routines.
**/
#include "OsLoader.h"

/**
	Halts the system.
	This function is called when returned from the kernel entry point.
**/
VOID
EFIAPI
OslReturnedFromKernel(
	VOID)
{
	for (;;)
	{
		__halt();
	}
}


/**
	Transfer to the kernel.

	@param[in] LoaderBlock    The loader block which contains OS loader information.
  
	@retval Not-FALSE         The operation is completed successfully.
	@retval FALSE             An error occurred during the operation.

**/
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

	// Start the Kernel!
	StartEntry(KernelBase, (UINT64)LoaderBlock, sizeof(OS_LOADER_BLOCK), 0);

	OslReturnedFromKernel();

	return FALSE;
}
