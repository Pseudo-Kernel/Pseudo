/** @file
  OS Boot Image Routines.
**/
#include "OSLDR.h"

/**
  Loads the Boot Image.
  The Boot Image must be located in EFI System Partition(ESP).

  @param[in] LoaderBlock    The loader block which contains OS loader information.
  
  @retval TRUE              The operation is completed successfully.
  @retval FALSE             An error occurred during the operation.

**/
BOOLEAN
EFIAPI
OslLoadBootImage (
  IN OS_LOADER_BLOCK *LoaderBlock, 
  IN CHAR16          *ImagePath
  )
{
  return FALSE;
}
