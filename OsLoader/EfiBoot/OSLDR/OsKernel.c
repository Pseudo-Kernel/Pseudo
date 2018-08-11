/** @file
  Transfer-to-Kernel Routines.
**/
#include "OSLDR.h"

/**
  Transfer to the kernel.

  @param[in] LoaderBlock    The loader block which contains OS loader information.
  
  @retval Not-FALSE         The operation is completed successfully.
  @retval FALSE             An error occurred during the operation.

**/
BOOLEAN
EFIAPI
OslTransferToKernel (
  IN OS_LOADER_BLOCK *LoaderBlock
  )
{
  return FALSE;
}
