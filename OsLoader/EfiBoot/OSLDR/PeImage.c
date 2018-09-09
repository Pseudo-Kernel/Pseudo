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
	IN EFI_PHYSICAL_ADDRESS    ImageBase,
	IN EFI_PHYSICAL_ADDRESS    CurrentPreferredBase,
	IN EFI_PHYSICAL_ADDRESS    FixupBase,
	IN BOOLEAN                 UseDefaultPreferredBase)
{
	PIMAGE_NT_HEADERS_3264 Nt;
	EFI_PHYSICAL_ADDRESS PreferredBase;
	EFI_PHYSICAL_ADDRESS ImageDefaultBase;
	PIMAGE_DATA_DIRECTORY DataDirectory;
	UINT64 OffsetDelta;

	Nt = OslPeImageBaseToNtHeaders((void *)ImageBase);
	if(!Nt)
		return FALSE;

	if(!OslPeIsImageTypeSupported(Nt))
		return FALSE;

	ImageDefaultBase = Nt->Nt64.OptionalHeader.ImageBase;
	DataDirectory = Nt->Nt64.OptionalHeader.DataDirectory;

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
  
  @param[in] BootServices				Pointer to EFI boot services table.
  @param[in] FileBuffer                 Buffer which contains file content.
  @param[in] FileBufferLength           Length of the FileBuffer.
  
  @retval Non-NULL                      The operation is completed successfully.
  @retval NULL                          An error occurred during the operation.
  
**/
EFI_PHYSICAL_ADDRESS
EFIAPI
OslPeLoadImage(
	IN EFI_BOOT_SERVICES *BootServices, 
	IN VOID *FileBuffer, 
	IN UINTN FileBufferLength)
{
	PIMAGE_NT_HEADERS_3264 Nt;
	PIMAGE_SECTION_HEADER SectionHeader;
	UINTN SectionAlignment;
	UINTN SizeOfImage;
	UINTN SizeOfHeaders;
	UINTN PageCount;
	EFI_PHYSICAL_ADDRESS BaseAddress;

	UINTN i;

	Nt = OslPeImageBaseToNtHeaders(FileBuffer);
	if (!Nt)
		return (EFI_PHYSICAL_ADDRESS)NULL;

	if (!OslPeIsImageTypeSupported(Nt))
		return (EFI_PHYSICAL_ADDRESS)NULL;

	SectionAlignment = Nt->Nt64.OptionalHeader.SectionAlignment;
	SizeOfImage = Nt->Nt64.OptionalHeader.SizeOfImage;
	SizeOfHeaders = Nt->Nt64.OptionalHeader.SizeOfHeaders;

	if (SizeOfHeaders >= Nt->Nt64.OptionalHeader.SizeOfImage)
		return (EFI_PHYSICAL_ADDRESS)NULL;


	//
	// Allocate zero-filled pages for our image.
	//

	PageCount = EFI_SIZE_TO_PAGES(SizeOfImage);

	if (BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, PageCount, &BaseAddress)
		!= EFI_SUCCESS)
	{
		return (EFI_PHYSICAL_ADDRESS)NULL;
	}

	BootServices->SetMem((void *)BaseAddress, SizeOfImage, 0);

	//
	// Copy the headers and section data.
	//

	BootServices->CopyMem((void *)BaseAddress, FileBuffer, SizeOfHeaders);
	
	SectionHeader = IMAGE_FIRST_SECTION(&Nt->Nt64);

	for (i = 0; i < Nt->Nt64.FileHeader.NumberOfSections; i++)
	{
		UINTN SectionVirtualSize;
		UINTN SectionRawSize;
		UINTN SectionRva;
		UINTN SectionRawOffset;

		EFI_PHYSICAL_ADDRESS Source;
		EFI_PHYSICAL_ADDRESS Destination;

		SectionVirtualSize = SectionHeader[i].Misc.VirtualSize;
		SectionVirtualSize = (SectionVirtualSize + SectionAlignment - 1) & ~(SectionAlignment - 1);
		SectionRawSize = SectionHeader[i].SizeOfRawData;

		SectionRva = BaseAddress + SectionHeader[i].VirtualAddress;
		SectionRawOffset = SectionHeader[i].PointerToRawData;

		if (SectionRva >= SizeOfImage ||
			SectionVirtualSize >= SizeOfImage ||
			SectionRva + SectionVirtualSize > SizeOfImage ||
			SectionRawOffset >= FileBufferLength ||
			SectionRawSize >= FileBufferLength ||
			SectionRawOffset + SectionRawSize > FileBufferLength)
		{
			// Section out of bounds.
			BootServices->FreePages(BaseAddress, PageCount);
			return (EFI_PHYSICAL_ADDRESS)NULL;
		}

//		LDR_TRACE("Section '%s' -> 0x%p - 0x%p\n", SectionHeader[Index].Name, 
//			(ULONG)BaseAddress+SectionHeader[Index].VirtualAddress, 
//			(ULONG)BaseAddress+SectionHeader[Index].VirtualAddress+SectionVirtualSize-1);

		Source = (EFI_PHYSICAL_ADDRESS)FileBuffer + SectionRawOffset;
		Destination = BaseAddress + SectionRva;

		BootServices->SetMem((void *)Destination, SectionVirtualSize, 0);
		BootServices->CopyMem((void *)Destination, (void *)Source, SectionRawSize);
	}


	if (!OslPeFixupImage(BaseAddress, BaseAddress, (EFI_PHYSICAL_ADDRESS)NULL, TRUE))
	{
		// Relocation failed.
		BootServices->FreePages(BaseAddress, PageCount);
		return (EFI_PHYSICAL_ADDRESS)NULL;
	}

	return BaseAddress;
}
