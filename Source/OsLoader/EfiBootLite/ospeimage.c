
/**
 * @file ospeimage.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements PE32+ loader.
 * @version 0.1
 * @date 2021-09-10
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "OsLoader.h"
#include "osmisc.h"
#include "osdebug.h"
#include "osmemory.h"
#include "ospeimage.h"


/**
 * @brief Gets IMAGE_NT_HEADERS from image base.
 * 
 * @param [in] ImageBase    Image base.
 * 
 * @return Non-zero value if succeeds, NULL otherwise.
 */
PIMAGE_NT_HEADERS_3264
EFIAPI
OslPeImageBaseToNtHeaders(
    IN VOID *ImageBase)
{
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS_3264 NtHeaders;

    DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
    if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return NULL;

    NtHeaders = (PIMAGE_NT_HEADERS_3264)((CHAR8 *)ImageBase + DosHeader->e_lfanew);
    if (NtHeaders->Nt32.Signature != IMAGE_NT_SIGNATURE)
        return NULL;

    return NtHeaders;
}

/**
 * @brief Determines whether the image type is AMD64.
 * 
 * @param [in] NtHeaders    NT header of image.
 * 
 * @return TRUE if image type is AMD64, FALSE otherwise.
 */
BOOLEAN
EFIAPI
OslPeIsImageTypeSupported(
    IN PIMAGE_NT_HEADERS_3264 NtHeaders)
{
    if (NtHeaders->Nt32.Signature == IMAGE_NT_SIGNATURE && 
        NtHeaders->Nt32.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC && 
        NtHeaders->Nt32.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
        return TRUE;

    return FALSE;
}


/**
 * @brief Do image relocation fixups.
 * 
 * @param [in] ImageBase                Image base.
 * @param [in] CurrentPreferredBase     Currently preferred image base.
 * @param [in] FixupBase                New base address of image we want to relocate.
 * @param [in] UseDefaultPreferredBase  If true, default base address is used.
 * 
 * @return TRUE     1. The operation is completed successfully.\n
 *                  2. Nothing to fixup (Relocation information is not found).
 * @return FALSE    An error occurred during the operation.
 */
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

    DTRACEF(&OslLoaderBlock, L"Fixup Image at 0x%016lX\r\n", ImageBase);

    Nt = OslPeImageBaseToNtHeaders((void *)ImageBase);
    if (!Nt)
        return FALSE;

    if (!OslPeIsImageTypeSupported(Nt))
        return FALSE;

    ImageDefaultBase = Nt->Nt64.OptionalHeader.ImageBase;
    DataDirectory = Nt->Nt64.OptionalHeader.DataDirectory;

    PreferredBase = UseDefaultPreferredBase ? ImageDefaultBase : CurrentPreferredBase;
    OffsetDelta = FixupBase - PreferredBase;

    DTRACEF(&OslLoaderBlock, L"Default ImageBase 0x%016lX\r\n", Nt->Nt64.OptionalHeader.ImageBase);

    if (DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress)
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

        for (RelocCurr = Reloc; ; )
        {
            TypeOffset = (PIMAGE_TYPE_OFFSET)(RelocCurr + 1);
            Count = (RelocCurr->SizeOfBlock - sizeof(*RelocCurr)) / sizeof(*TypeOffset);

            for (i = 0; i < Count; i++)
            {
                UINT16 Type = TypeOffset[i].s.Type;

                //
                // Accept record types for PE32+ only.
                // We can handle HIGH, LOW, HIGHLOW so don't care about it
                //

                if (Type != IMAGE_REL_BASED_ABSOLUTE &&
                    Type != IMAGE_REL_BASED_HIGH &&
                    Type != IMAGE_REL_BASED_LOW &&
                    Type != IMAGE_REL_BASED_HIGHLOW &&
                    Type != IMAGE_REL_BASED_DIR64)
                {
                    DTRACEF(&OslLoaderBlock, L"Unsupported record type 0x%X\r\n", Type);
                    return FALSE;
                }
            }

            RelocCurr = (PIMAGE_BASE_RELOCATION)((CHAR8 *)RelocCurr + ((RelocCurr->SizeOfBlock + 0x03) & ~0x03));

            if ((UINT64)RelocCurr - (UINT64)Reloc >= DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
                break;
        }

        //
        // Do the fixups.
        // More information: https://stackoverflow.com/questions/17436668/how-are-pe-base-relocations-build-up
        //

        for(RelocCurr = Reloc; ; )
        {
            TypeOffset = (PIMAGE_TYPE_OFFSET)(RelocCurr + 1);
            Count = (RelocCurr->SizeOfBlock - sizeof(*RelocCurr)) / sizeof(*TypeOffset);

            for (i = 0; i < Count; i++)
            {
                union
                {
                    UINT64 *u64;
                    UINT32 *u32;
                    UINT16 *u16;
                    UINT64 p;
                } Pointer;

                Pointer.p = (UINT64)ImageBase + RelocCurr->VirtualAddress + TypeOffset[i].s.Offset;

                //
                // Example data of Core.sys: 00003000 00000010 A010 A020 A030 A040
                // 
                // VirtualAddress   0x00003000
                // SizeOfBlock      0x00000010
                // < TypeOffsets >
                //  + DIR64     Offset 0x010
                //  + DIR64     Offset 0x020
                //  + DIR64     Offset 0x030
                //  + DIR64     Offset 0x040
                //

                switch(TypeOffset[i].s.Type)
                {
                case IMAGE_REL_BASED_ABSOLUTE:
                    break;
                case IMAGE_REL_BASED_HIGH:
                    *Pointer.u16 += (UINT16)((OffsetDelta & 0xffff0000) >> 0x10);
                    break;
                case IMAGE_REL_BASED_LOW:
                    *Pointer.u16 += (UINT16)(OffsetDelta & 0xffff);
                    break;
                case IMAGE_REL_BASED_HIGHLOW:   // PE, 32-bit offset
                    *Pointer.u32 += (UINT32)(OffsetDelta & 0xffffffff);
                    break;
                case IMAGE_REL_BASED_DIR64:     // PE32+, 64-bit offset
            //      TRACE(L"Address 0x%lx, OffsetDelta 0x%lx, BeforeFixup 0x%lx, AfterFixup 0x%lx\r\n",
            //          Pointer.u64, OffsetDelta, *Pointer.u64, *Pointer.u64 + OffsetDelta);
                    *Pointer.u64 += OffsetDelta;
                    break;
                }
            }

            RelocCurr = (PIMAGE_BASE_RELOCATION)((CHAR8 *)RelocCurr + ((RelocCurr->SizeOfBlock + 0x03) & ~0x03));

            if ((UINT64)RelocCurr - (UINT64)Reloc >= DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
                break;
        }
    }
    else
    {
        DTRACEF(&OslLoaderBlock, L"Relocation directory not found, skipping...\r\n");
    }

    return TRUE;
}

/**
 * @brief Loads the PE32+ image.
 * 
 * @param [in] FileBuffer           File buffer which contains file content.
 * @param [in] FileBufferLength     Length of file buffer.
 * @param [out] MappedSize          Size of mapped image.
 * @param [in] FixupBase            Fixup base address. If zero is specified, default image base will be used.
 * 
 * @return Returns physical address of image base.\n
 *         Non-zero if succeeds, zero otherwise.
 */
EFI_PHYSICAL_ADDRESS
EFIAPI
OslPeLoadImage(
    IN OS_LOADER_BLOCK *LoaderBlock, 
    IN VOID *FileBuffer, 
    IN UINTN FileBufferLength, 
    OUT UINTN *MappedSize,
    IN UINT64 FixupBase)
{
    PIMAGE_NT_HEADERS_3264 Nt;
    PIMAGE_SECTION_HEADER SectionHeader;
    UINTN SectionAlignment;
    UINTN SizeOfImage;
    UINTN SizeOfHeaders;
    EFI_PHYSICAL_ADDRESS BaseAddress;
    EFI_STATUS AllocationStatus;

    UINTN i;

    DTRACEF(LoaderBlock, L"Loading image...\r\n");

    Nt = OslPeImageBaseToNtHeaders(FileBuffer);
    if (!Nt)
    {
        DTRACEF(LoaderBlock, L"Invalid image header\r\n");
        return 0;
    }

    if (!OslPeIsImageTypeSupported(Nt))
    {
        DTRACEF(LoaderBlock, L"Unsupported image type or architecture\r\n");
        return 0;
    }

    SectionAlignment = Nt->Nt64.OptionalHeader.SectionAlignment;
    SizeOfImage = Nt->Nt64.OptionalHeader.SizeOfImage;
    SizeOfHeaders = Nt->Nt64.OptionalHeader.SizeOfHeaders;

    if (SizeOfHeaders >= Nt->Nt64.OptionalHeader.SizeOfImage)
    {
        DTRACEF(LoaderBlock, L"Corrupted image header\r\n");
        return 0;
    }


    //
    // Allocate zero-filled pages for our image.
    // Address must be in the lower 4GB area.
    //

    DTRACEF(LoaderBlock, L"SizeOfImage = 0x%lX\r\n", SizeOfImage);

    BaseAddress = Nt->Nt64.OptionalHeader.ImageBase;
    AllocationStatus = OslAllocatePagesPreserve(LoaderBlock, SizeOfImage, &BaseAddress, TRUE, OsKernelImage);

    if (AllocationStatus != EFI_SUCCESS)
    {
        DTRACEF(LoaderBlock, L"Failed to allocate pages at preferred base 0x%p\r\n", Nt->Nt64.OptionalHeader.ImageBase);

        // Not a failure, keep going
        AllocationStatus = OslAllocatePagesPreserve(LoaderBlock, SizeOfImage, &BaseAddress, FALSE, OsKernelImage);
        if (AllocationStatus != EFI_SUCCESS)
        {
            DTRACEF(LoaderBlock, L"Failed to allocate pages\r\n");
            return 0;
        }
    }

    //
    // Copy the headers and section data.
    //

    gBS->CopyMem((void *)BaseAddress, FileBuffer, SizeOfHeaders);
    
    SectionHeader = IMAGE_FIRST_SECTION(&Nt->Nt64);

    DTRACE(LoaderBlock,
        L"\r\n"
        L"                FileOffset    RawSize          RVA   VirtSize         Attr\r\n");

    for (i = 0; i < Nt->Nt64.FileHeader.NumberOfSections; i++)
    {
        UINTN SectionVirtualSize;
        UINTN SectionRawSize;
        UINTN SectionRva;
        UINTN SectionRawOffset;

        EFI_PHYSICAL_ADDRESS Source;
        EFI_PHYSICAL_ADDRESS Destination;

        DTRACE(LoaderBlock,
            L"  Section[%2d]:  0x%08lX 0x%08lX   0x%08lX 0x%08lX   0x%08lX\r\n", i, 
            SectionHeader[i].PointerToRawData, SectionHeader[i].SizeOfRawData,
            SectionHeader[i].VirtualAddress, SectionHeader[i].Misc.VirtualSize,
            SectionHeader[i].Characteristics);

        SectionVirtualSize = SectionHeader[i].Misc.VirtualSize;
        SectionVirtualSize = (SectionVirtualSize + SectionAlignment - 1) & ~(SectionAlignment - 1);
        SectionRawSize = SectionHeader[i].SizeOfRawData;

        SectionRva = SectionHeader[i].VirtualAddress;
        SectionRawOffset = SectionHeader[i].PointerToRawData;

        if (SectionRva >= SizeOfImage ||
            SectionVirtualSize >= SizeOfImage ||
            SectionRva + SectionVirtualSize > SizeOfImage ||
            SectionRawOffset >= FileBufferLength ||
            SectionRawSize >= FileBufferLength ||
            SectionRawOffset + SectionRawSize > FileBufferLength)
        {
            // Section out of bounds.
            DTRACEF(LoaderBlock, L"Section[%d] out of bounds\r\n", i);
            OslFreePagesPreserve(LoaderBlock, BaseAddress, SizeOfImage);
            return 0;
        }

        Source = (EFI_PHYSICAL_ADDRESS)FileBuffer + SectionRawOffset;
        Destination = BaseAddress + SectionRva;

        gBS->SetMem((void *)Destination, SectionVirtualSize, 0);
        gBS->CopyMem((void *)Destination, (void *)Source, SectionRawSize);
    }

    EFI_PHYSICAL_ADDRESS TargetFixupBase = BaseAddress;
    BOOLEAN UseFixupBase = FALSE;
    if (FixupBase)
    {
        UseFixupBase = TRUE;
        TargetFixupBase = FixupBase;
    }

    if (!OslPeFixupImage(BaseAddress, 0, TargetFixupBase, UseFixupBase))
    {
        // Relocation failed.
        DTRACEF(LoaderBlock, L"Failed to fixup image\r\n", i);
        OslFreePagesPreserve(LoaderBlock, BaseAddress, SizeOfImage);
        return 0;
    }

    DTRACEF(LoaderBlock, 
        L"Kernel loaded at  : 0x%016lX - 0x%016lX\r\n"
        L"Kernel EntryPoint : 0x%016lX\r\n", 
        BaseAddress, 
        BaseAddress + Nt->Nt64.OptionalHeader.SizeOfImage - 1, 
        BaseAddress + Nt->Nt64.OptionalHeader.AddressOfEntryPoint);

    *MappedSize = SizeOfImage;

    return BaseAddress;
}
