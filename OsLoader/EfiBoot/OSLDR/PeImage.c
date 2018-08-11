/** @file
  PE32+ File Loader.
**/

#include "OSLDR.h"
#include "PeImage.h"



//
// PE image support.
//

/**
  Get IMAGE_NT_HEADERS of Image Base.

  @param[in] ImageBase      Base Address of the Image.  
  
  @retval Non-NULL          The operation is completed successfully.
  @retval other             An error occurred during the operation.

**/
PIMAGE_NT_HEADERS_3264
EFIAPI
OslPeImageBaseToNtHeaders(
	IN VOID *ImageBase)
{
	PIMAGE_DOS_HEADER DosHeader;
	PIMAGE_NT_HEADERS_3264 NtHeaders;

	DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	if(DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return NULL;

	NtHeaders = (PIMAGE_NT_HEADERS_3264)((CHAR8 *)ImageBase + DosHeader->e_lfanew);
	if(NtHeaders->Nt32.Signature != IMAGE_NT_SIGNATURE)
		return NULL;

	return NtHeaders;
}

/**
  Determine if the Image Type is AMD64.

  @param[in] NtHeaders      NT Header of the Image.  
  
  @retval Not-FALSE         Image Type is AMD64 (x64).
  @retval FALSE             Not an Image or Image operation.

**/
BOOLEAN
EFIAPI
OslPeIsImageTypeSupported(
	IN PIMAGE_NT_HEADERS_3264 NtHeaders)
{
	if(NtHeaders->Nt32.Signature == IMAGE_NT_SIGNATURE && 
		NtHeaders->Nt32.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC && 
		NtHeaders->Nt32.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
		return TRUE;

	return FALSE;
}

/**
  Do Image Fixups.

  @param[in] ImageBase                  Base Address of Image.
  @param[in] CurrentPreferredBase       Currently Preferred Base Address.
  @param[in] FixupBase                  New Base Address of Image.
  @param[in] UseDefaultPreferredBase    If true, Use Preferred Base Address Specified in Header.
  
  @retval Non-FALSE                     The operation is completed successfully.
                                        Nothing to fixup (Relocation information is not found).
  @retval FALSE                         An error occurred during the operation.

**/
BOOLEAN
EFIAPI
OslPeFixupImage(
	IN VOID     *ImageBase,
	IN UINT64    CurrentPreferredBase,
	IN UINT64    FixupBase,
	IN BOOLEAN   UseDefaultPreferredBase)
{
	PIMAGE_NT_HEADERS_3264 Nt;
	UINT64 PreferredBase;
	UINT64 ImageDefaultBase;
	PIMAGE_DATA_DIRECTORY DataDirectory;
	UINT64 OffsetDelta;

	Nt = OslPeImageBaseToNtHeaders(ImageBase);
	if(!Nt)
		return FALSE;

	if(!OslPeIsImageTypeSupported(Nt))
		return FALSE;


	PreferredBase = UseDefaultPreferredBase ? ImageDefaultBase : CurrentPreferredBase;
	OffsetDelta = FixupBase - PreferredBase;

	if(DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress)
	{
		PIMAGE_BASE_RELOCATION Reloc;
		PIMAGE_BASE_RELOCATION RelocCurr;
		PIMAGE_TYPE_OFFSET TypeOffset;
		UINTN i;
		UINTN Count;

		Reloc = (PIMAGE_BASE_RELOCATION)((CHAR8 *)ImageBase + 
			DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

		//
		// Check all the records before fixup.
		//

		for(RelocCurr = Reloc; ; )
		{
			TypeOffset = (PIMAGE_TYPE_OFFSET)(RelocCurr + 1);
			Count = (RelocCurr->SizeOfBlock - sizeof(*RelocCurr)) / sizeof(*TypeOffset);

			for(i = 0; i < Count; i++)
			{
				UINT16 Type = TypeOffset[i].s.Type;

				// Accept record types for PE32+ only.
				if(Type != IMAGE_REL_BASED_ABSOLUTE && Type != IMAGE_REL_BASED_DIR64)
					return FALSE;
			}

			RelocCurr = (PIMAGE_BASE_RELOCATION)((CHAR8 *)RelocCurr + RelocCurr->SizeOfBlock);

			if ((UINT64)RelocCurr - (UINT64)Reloc >= DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
				break;
		}

		//
		// Do the fixups.
		//

		for(RelocCurr = Reloc; ; )
		{
			TypeOffset = (PIMAGE_TYPE_OFFSET)(RelocCurr + 1);
			Count = (RelocCurr->SizeOfBlock - sizeof(*RelocCurr)) / sizeof(*TypeOffset);

			for(i = 0; i < Count; i++)
			{
				union
				{
					UINT64 *u64;
					UINT64 p;
				} Pointer;

				Pointer.p = (UINT64)ImageBase + RelocCurr->VirtualAddress + TypeOffset[i].s.Offset;

				switch(TypeOffset[i].s.Type)
				{
				case IMAGE_REL_BASED_ABSOLUTE:
					break;
				case IMAGE_REL_BASED_HIGHLOW:	// PE, 32-bit offset
					break;
				case IMAGE_REL_BASED_DIR64:		// PE32+, 64-bit offset
					*Pointer.u64 += OffsetDelta;
					break;
				}
			}

			RelocCurr = (PIMAGE_BASE_RELOCATION)((CHAR8 *)RelocCurr + RelocCurr->SizeOfBlock);

			if ((UINT64)RelocCurr - (UINT64)Reloc >= DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
				break;
		}
	}
	else
	{
//		DbgPrintf("Relocation Directory Not Found, Nothing to do\n");
	}

	return TRUE;
}

/**
  Loads the PE32+ Image.
  
  @param[in] FileBuffer                 Buffer which contains file content.
  @param[in] PreferredBase              Preferred Base Address.
  
  @retval Non-NULL                      The operation is completed successfully.
  @retval NULL                          An error occurred during the operation.
  
**/
VOID *
EFIAPI
OslPeLoadSystemImage(
	IN VOID *FileBuffer, 
	IN VOID *PreferredBase)
{
#if 0
	PIMAGE_NT_HEADERS_3264 Nt;
	PIMAGE_SECTION_HEADER SectionHeader;
	ULONG_PTR BaseAddress;
	ULONG SectionAlignment;
	ULONG SizeOfHeaders;
	ULONG SizeOfImage;
	ULONG64 ImageDefaultBase;
	ULONG i;

	Nt = OslPeImageBaseToNtHeaders(FileBuffer);
	if(!Nt)
		return NULL;

  if(!OslPeIsImageTypeSupported(Nt))
    return NULL;

	BaseAddress = LdrpHelperAllocatePage(PreferredBase, SizeOfImage);
	if(!BaseAddress)
		return 0;

	//
	// Locate the headers.
	//

	memset((PVOID)BaseAddress, 0, (SizeOfHeaders + SectionAlignment - 1) & ~(SectionAlignment - 1));
	memcpy((PVOID)BaseAddress, FileBuffer, SizeOfHeaders);

	//
	// Locate the sections.
	//

	SectionHeader = IMAGE_FIRST_SECTION(&Nt->Nt32); // returns same result for Nt32/Nt64.
	for(i = 0; i < Nt->Nt32.FileHeader.NumberOfSections; i++)
	{
		ULONG SectionVirtualSize;

		SectionVirtualSize = SectionHeader[i].Misc.VirtualSize;
		SectionVirtualSize = (SectionVirtualSize + SectionAlignment - 1) & ~(SectionAlignment - 1);

//		LDR_TRACE("Section '%s' -> 0x%p - 0x%p\n", SectionHeader[Index].Name, 
//			(ULONG)BaseAddress+SectionHeader[Index].VirtualAddress, 
//			(ULONG)BaseAddress+SectionHeader[Index].VirtualAddress+SectionVirtualSize-1);

		memset((PVOID)((ULONG_PTR)BaseAddress + SectionHeader[i].VirtualAddress), 0, SectionVirtualSize);
		memcpy((PVOID)((ULONG_PTR)BaseAddress + SectionHeader[i].VirtualAddress), 
			(PUCHAR)FileBuffer + SectionHeader[i].PointerToRawData, SectionHeader[i].SizeOfRawData);
	}

	//
	// Do the relocation fixup.
	//

	if(!LdrFixupSystemImage(BaseAddress, 0, BaseAddress, TRUE))
	{
		LdrpHelperFreePage(BaseAddress);

		return 0;
	}

	return BaseAddress;
#endif

	return NULL;
}

/**
  Loads the PE Image.

  @param[in] LoaderBlock    The loader block pointer.  
  @param[in] FilePath       File Path.
  
  @retval Non-NULL          The operation is completed successfully.
  @retval other             An error occurred during the operation.

**/
VOID *
EFIAPI
OslPeLoadImage64 (
  IN OS_LOADER_BLOCK  *LoaderBlock, 
  IN VOID             *FileBuffer, 
  IN UINT32            BufferSize
  )
{
//  void *ImageBase = NULL;
//  EFI_STATUS Status = EFI_SUCCESS;

//  Status = LoaderBlock->Base.BootServices->AllocatePages(
//    EfiLoaderData, 

  return NULL;
}

