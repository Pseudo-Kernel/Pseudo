
#ifndef __PEIMAGE_H__
#define __PEIMAGE_H__

//
// PE File Format, From WINNT.H
//


#define IMAGE_DOS_SIGNATURE                 0x5A4D      // MZ
#define IMAGE_OS2_SIGNATURE                 0x454E      // NE
#define IMAGE_OS2_SIGNATURE_LE              0x454C      // LE
#define IMAGE_VXD_SIGNATURE                 0x454C      // LE
#define IMAGE_NT_SIGNATURE                  0x00004550  // PE00

//
// Dos Header.
//

#pragma pack(push, 2)

typedef struct _IMAGE_DOS_HEADER {      // DOS .EXE header
	UINT16 e_magic;                     // Magic number
	UINT16 e_cblp;                      // Bytes on last page of file
	UINT16 e_cp;                        // Pages in file
	UINT16 e_crlc;                      // Relocations
	UINT16 e_cparhdr;                   // Size of header in paragraphs
	UINT16 e_minalloc;                  // Minimum extra paragraphs needed
	UINT16 e_maxalloc;                  // Maximum extra paragraphs needed
	UINT16 e_ss;                        // Initial (relative) SS value
	UINT16 e_sp;                        // Initial SP value
	UINT16 e_csum;                      // Checksum
	UINT16 e_ip;                        // Initial IP value
	UINT16 e_cs;                        // Initial (relative) CS value
	UINT16 e_lfarlc;                    // File address of relocation table
	UINT16 e_ovno;                      // Overlay number
	UINT16 e_res[4];                    // Reserved words
	UINT16 e_oemid;                     // OEM identifier (for e_oeminfo)
	UINT16 e_oeminfo;                   // OEM information; e_oemid specific
	UINT16 e_res2[10];                  // Reserved words
	INT32   e_lfanew;                    // File address of new exe header
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

#pragma pack(pop)

#pragma pack(push, 4)

//
// File header format.
//

typedef struct _IMAGE_FILE_HEADER {
	UINT16 Machine;
	UINT16 NumberOfSections;
	UINT32  TimeDateStamp;
	UINT32  PointerToSymbolTable;
	UINT32  NumberOfSymbols;
	UINT16 SizeOfOptionalHeader;
	UINT16 Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

#define IMAGE_SIZEOF_FILE_HEADER             20

#define IMAGE_FILE_RELOCS_STRIPPED           0x0001  // Relocation info stripped from file.
#define IMAGE_FILE_EXECUTABLE_IMAGE          0x0002  // File is executable  (i.e. no unresolved externel references).
#define IMAGE_FILE_LINE_NUMS_STRIPPED        0x0004  // Line nunbers stripped from file.
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED       0x0008  // Local symbols stripped from file.
#define IMAGE_FILE_AGGRESIVE_WS_TRIM         0x0010  // Agressively trim working set
#define IMAGE_FILE_LARGE_ADDRESS_AWARE       0x0020  // App can handle >2gb addresses
#define IMAGE_FILE_BYTES_REVERSED_LO         0x0080  // Bytes of machine word are reversed.
#define IMAGE_FILE_32BIT_MACHINE             0x0100  // 32 bit word machine.
#define IMAGE_FILE_DEBUG_STRIPPED            0x0200  // Debugging info stripped from file in .DBG file
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP   0x0400  // If Image is on removable media, copy and run from the swap file.
#define IMAGE_FILE_NET_RUN_FROM_SWAP         0x0800  // If Image is on Net, copy and run from the swap file.
#define IMAGE_FILE_SYSTEM                    0x1000  // System File.
#define IMAGE_FILE_DLL                       0x2000  // File is a DLL.
#define IMAGE_FILE_UP_SYSTEM_ONLY            0x4000  // File should only be run on a UP machine
#define IMAGE_FILE_BYTES_REVERSED_HI         0x8000  // Bytes of machine word are reversed.

#define IMAGE_FILE_MACHINE_UNKNOWN           0
#define IMAGE_FILE_MACHINE_I386              0x014c  // Intel 386.
#define IMAGE_FILE_MACHINE_ARM               0x01c0  // ARM Little-Endian
#define IMAGE_FILE_MACHINE_AMD64             0x8664  // AMD64 (K8)


//
// Directory format.
//

typedef struct _IMAGE_DATA_DIRECTORY {
	UINT32 VirtualAddress;
	UINT32 Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16

//
// Optional header format.
//

typedef struct _IMAGE_OPTIONAL_HEADER {
	//
	// Standard fields.
	//

	UINT16  Magic;
	UINT8    MajorLinkerVersion;
	UINT8    MinorLinkerVersion;
	UINT32   SizeOfCode;
	UINT32   SizeOfInitializedData;
	UINT32   SizeOfUninitializedData;
	UINT32   AddressOfEntryPoint;
	UINT32   BaseOfCode;
	UINT32   BaseOfData;

	//
	// NT additional fields.
	//

	UINT32   ImageBase;
	UINT32   SectionAlignment;
	UINT32   FileAlignment;
	UINT16  MajorOperatingSystemVersion;
	UINT16  MinorOperatingSystemVersion;
	UINT16  MajorImageVersion;
	UINT16  MinorImageVersion;
	UINT16  MajorSubsystemVersion;
	UINT16  MinorSubsystemVersion;
	UINT32   Win32VersionValue;
	UINT32   SizeOfImage;
	UINT32   SizeOfHeaders;
	UINT32   CheckSum;
	UINT16  Subsystem;
	UINT16  DllCharacteristics;
	UINT32   SizeOfStackReserve;
	UINT32   SizeOfStackCommit;
	UINT32   SizeOfHeapReserve;
	UINT32   SizeOfHeapCommit;
	UINT32   LoaderFlags;
	UINT32   NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

typedef struct _IMAGE_OPTIONAL_HEADER64 {
	UINT16    Magic;
	UINT8      MajorLinkerVersion;
	UINT8      MinorLinkerVersion;
	UINT32     SizeOfCode;
	UINT32     SizeOfInitializedData;
	UINT32     SizeOfUninitializedData;
	UINT32     AddressOfEntryPoint;
	UINT32     BaseOfCode;
	UINT64   ImageBase;
	UINT32     SectionAlignment;
	UINT32     FileAlignment;
	UINT16    MajorOperatingSystemVersion;
	UINT16    MinorOperatingSystemVersion;
	UINT16    MajorImageVersion;
	UINT16    MinorImageVersion;
	UINT16    MajorSubsystemVersion;
	UINT16    MinorSubsystemVersion;
	UINT32     Win32VersionValue;
	UINT32     SizeOfImage;
	UINT32     SizeOfHeaders;
	UINT32     CheckSum;
	UINT16    Subsystem;
	UINT16    DllCharacteristics;
	UINT64   SizeOfStackReserve;
	UINT64   SizeOfStackCommit;
	UINT64   SizeOfHeapReserve;
	UINT64   SizeOfHeapCommit;
	UINT32     LoaderFlags;
	UINT32     NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

#define IMAGE_NT_OPTIONAL_HDR32_MAGIC      0x10b
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC      0x20b
#define IMAGE_ROM_OPTIONAL_HDR_MAGIC       0x107


//
// Nt Headers.
//

typedef struct _IMAGE_NT_HEADERS64 {
	UINT32 Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64, *PIMAGE_NT_HEADERS64;

typedef struct _IMAGE_NT_HEADERS {
	UINT32 Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32, *PIMAGE_NT_HEADERS32;

typedef	IMAGE_NT_HEADERS32			IMAGE_NT_HEADERS, *PIMAGE_NT_HEADERS;


// IMAGE_FIRST_SECTION doesn't need 32/64 versions since the file header is the same either way.

#define	FIELD_OFFSET(_type, _field)	\
	( (UINTN)( &((_type *)0)->_field ) )

#define IMAGE_FIRST_SECTION( ntheader ) ((PIMAGE_SECTION_HEADER)        \
    ((CHAR8 *)(ntheader) +                                            \
     FIELD_OFFSET( IMAGE_NT_HEADERS, OptionalHeader ) +                 \
     ((ntheader))->FileHeader.SizeOfOptionalHeader   \
    ))

// Subsystem Values

#define IMAGE_SUBSYSTEM_UNKNOWN              0   // Unknown subsystem.
#define IMAGE_SUBSYSTEM_NATIVE               1   // Image doesn't require a subsystem.
#define IMAGE_SUBSYSTEM_WINDOWS_GUI          2   // Image runs in the Windows GUI subsystem.
#define IMAGE_SUBSYSTEM_WINDOWS_CUI          3   // Image runs in the Windows character subsystem.
#define IMAGE_SUBSYSTEM_OS2_CUI              5   // image runs in the OS/2 character subsystem.
#define IMAGE_SUBSYSTEM_POSIX_CUI            7   // image runs in the Posix character subsystem.
#define IMAGE_SUBSYSTEM_NATIVE_WINDOWS       8   // image is a native Win9x driver.
#define IMAGE_SUBSYSTEM_WINDOWS_CE_GUI       9   // Image runs in the Windows CE subsystem.
#define IMAGE_SUBSYSTEM_EFI_APPLICATION      10  //
#define IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER  11   //
#define IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER   12  //
#define IMAGE_SUBSYSTEM_EFI_ROM              13
#define IMAGE_SUBSYSTEM_XBOX                 14
#define IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION 16

// DllCharacteristics Entries

//      IMAGE_LIBRARY_PROCESS_INIT            0x0001     // Reserved.
//      IMAGE_LIBRARY_PROCESS_TERM            0x0002     // Reserved.
//      IMAGE_LIBRARY_THREAD_INIT             0x0004     // Reserved.
//      IMAGE_LIBRARY_THREAD_TERM             0x0008     // Reserved.
#define IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE 0x0040     // DLL can move.
#define IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY    0x0080     // Code Integrity Image
#define IMAGE_DLLCHARACTERISTICS_NX_COMPAT    0x0100     // Image is NX compatible
#define IMAGE_DLLCHARACTERISTICS_NO_ISOLATION 0x0200     // Image understands isolation and doesn't want it
#define IMAGE_DLLCHARACTERISTICS_NO_SEH       0x0400     // Image does not use SEH.  No SE handler may reside in this image
#define IMAGE_DLLCHARACTERISTICS_NO_BIND      0x0800     // Do not bind this image.
//                                            0x1000     // Reserved.
#define IMAGE_DLLCHARACTERISTICS_WDM_DRIVER   0x2000     // Driver uses WDM model
//                                            0x4000     // Reserved.
#define IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE     0x8000

// Directory Entries

#define IMAGE_DIRECTORY_ENTRY_EXPORT          0   // Export Directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT          1   // Import Directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE        2   // Resource Directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION       3   // Exception Directory
#define IMAGE_DIRECTORY_ENTRY_SECURITY        4   // Security Directory
#define IMAGE_DIRECTORY_ENTRY_BASERELOC       5   // Base Relocation Table
#define IMAGE_DIRECTORY_ENTRY_DEBUG           6   // Debug Directory
//      IMAGE_DIRECTORY_ENTRY_COPYRIGHT       7   // (X86 usage)
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    7   // Architecture Specific Data
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8   // RVA of GP
#define IMAGE_DIRECTORY_ENTRY_TLS             9   // TLS Directory
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10   // Load Configuration Directory
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11   // Bound Import Directory in headers
#define IMAGE_DIRECTORY_ENTRY_IAT            12   // Import Address Table
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13   // Delay Load Import Descriptors
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14   // COM Runtime descriptor



//
// Section header format.
//

#define IMAGE_SIZEOF_SHORT_NAME              8

typedef struct _IMAGE_SECTION_HEADER {
    UINT8   Name[IMAGE_SIZEOF_SHORT_NAME];
    union {
            UINT32   PhysicalAddress;
            UINT32   VirtualSize;
    } Misc;
    UINT32   VirtualAddress;
    UINT32   SizeOfRawData;
    UINT32   PointerToRawData;
    UINT32   PointerToRelocations;
    UINT32   PointerToLinenumbers;
    UINT16  NumberOfRelocations;
    UINT16  NumberOfLinenumbers;
    UINT32   Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

#define IMAGE_SIZEOF_SECTION_HEADER          40

//
// Section characteristics.
//
//      IMAGE_SCN_TYPE_REG                   0x00000000  // Reserved.
//      IMAGE_SCN_TYPE_DSECT                 0x00000001  // Reserved.
//      IMAGE_SCN_TYPE_NOLOAD                0x00000002  // Reserved.
//      IMAGE_SCN_TYPE_GROUP                 0x00000004  // Reserved.
#define IMAGE_SCN_TYPE_NO_PAD                0x00000008  // Reserved.
//      IMAGE_SCN_TYPE_COPY                  0x00000010  // Reserved.

#define IMAGE_SCN_CNT_CODE                   0x00000020  // Section contains code.
#define IMAGE_SCN_CNT_INITIALIZED_DATA       0x00000040  // Section contains initialized data.
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA     0x00000080  // Section contains uninitialized data.

#define IMAGE_SCN_LNK_OTHER                  0x00000100  // Reserved.
#define IMAGE_SCN_LNK_INFO                   0x00000200  // Section contains comments or some other type of information.
//      IMAGE_SCN_TYPE_OVER                  0x00000400  // Reserved.
#define IMAGE_SCN_LNK_REMOVE                 0x00000800  // Section contents will not become part of image.
#define IMAGE_SCN_LNK_COMDAT                 0x00001000  // Section contents comdat.
//                                           0x00002000  // Reserved.
//      IMAGE_SCN_MEM_PROTECTED - Obsolete   0x00004000
#define IMAGE_SCN_NO_DEFER_SPEC_EXC          0x00004000  // Reset speculative exceptions handling bits in the TLB entries for this section.
#define IMAGE_SCN_GPREL                      0x00008000  // Section content can be accessed relative to GP
#define IMAGE_SCN_MEM_FARDATA                0x00008000
//      IMAGE_SCN_MEM_SYSHEAP  - Obsolete    0x00010000
#define IMAGE_SCN_MEM_PURGEABLE              0x00020000
#define IMAGE_SCN_MEM_16BIT                  0x00020000
#define IMAGE_SCN_MEM_LOCKED                 0x00040000
#define IMAGE_SCN_MEM_PRELOAD                0x00080000

#define IMAGE_SCN_ALIGN_1BYTES               0x00100000  //
#define IMAGE_SCN_ALIGN_2BYTES               0x00200000  //
#define IMAGE_SCN_ALIGN_4BYTES               0x00300000  //
#define IMAGE_SCN_ALIGN_8BYTES               0x00400000  //
#define IMAGE_SCN_ALIGN_16BYTES              0x00500000  // Default alignment if no others are specified.
#define IMAGE_SCN_ALIGN_32BYTES              0x00600000  //
#define IMAGE_SCN_ALIGN_64BYTES              0x00700000  //
#define IMAGE_SCN_ALIGN_128BYTES             0x00800000  //
#define IMAGE_SCN_ALIGN_256BYTES             0x00900000  //
#define IMAGE_SCN_ALIGN_512BYTES             0x00A00000  //
#define IMAGE_SCN_ALIGN_1024BYTES            0x00B00000  //
#define IMAGE_SCN_ALIGN_2048BYTES            0x00C00000  //
#define IMAGE_SCN_ALIGN_4096BYTES            0x00D00000  //
#define IMAGE_SCN_ALIGN_8192BYTES            0x00E00000  //
// Unused                                    0x00F00000
#define IMAGE_SCN_ALIGN_MASK                 0x00F00000

#define IMAGE_SCN_LNK_NRELOC_OVFL            0x01000000  // Section contains extended relocations.
#define IMAGE_SCN_MEM_DISCARDABLE            0x02000000  // Section can be discarded.
#define IMAGE_SCN_MEM_NOT_CACHED             0x04000000  // Section is not cachable.
#define IMAGE_SCN_MEM_NOT_PAGED              0x08000000  // Section is not pageable.
#define IMAGE_SCN_MEM_SHARED                 0x10000000  // Section is shareable.
#define IMAGE_SCN_MEM_EXECUTE                0x20000000  // Section is executable.
#define IMAGE_SCN_MEM_READ                   0x40000000  // Section is readable.
#define IMAGE_SCN_MEM_WRITE                  0x80000000  // Section is writeable.


//
// Debug Format
//

typedef struct _IMAGE_DEBUG_DIRECTORY {
    UINT32   Characteristics;
    UINT32   TimeDateStamp;
    UINT16  MajorVersion;
    UINT16  MinorVersion;
    UINT32   Type;
    UINT32   SizeOfData;
    UINT32   AddressOfRawData;
    UINT32   PointerToRawData;
} IMAGE_DEBUG_DIRECTORY, *PIMAGE_DEBUG_DIRECTORY;

#define IMAGE_DEBUG_TYPE_UNKNOWN          0
#define IMAGE_DEBUG_TYPE_COFF             1
#define IMAGE_DEBUG_TYPE_CODEVIEW         2
#define IMAGE_DEBUG_TYPE_FPO              3
#define IMAGE_DEBUG_TYPE_MISC             4
#define IMAGE_DEBUG_TYPE_EXCEPTION        5
#define IMAGE_DEBUG_TYPE_FIXUP            6
#define IMAGE_DEBUG_TYPE_OMAP_TO_SRC      7
#define IMAGE_DEBUG_TYPE_OMAP_FROM_SRC    8
#define IMAGE_DEBUG_TYPE_BORLAND          9
#define IMAGE_DEBUG_TYPE_RESERVED10       10
#define IMAGE_DEBUG_TYPE_CLSID            11

//
// Function Entry.
//

typedef struct _IMAGE_RUNTIME_FUNCTION_ENTRY {
	UINT32 BeginAddress;
	UINT32 EndAddress;
	UINT32 UnwindInfoAddress;
} IMAGE_RUNTIME_FUNCTION_ENTRY, *PIMAGE_RUNTIME_FUNCTION_ENTRY;


//
// Export Format
//

typedef struct _IMAGE_EXPORT_DIRECTORY {
    UINT32   Characteristics;
    UINT32   TimeDateStamp;
    UINT16    MajorVersion;
    UINT16    MinorVersion;
    UINT32   Name;
    UINT32   Base;
    UINT32   NumberOfFunctions;
    UINT32   NumberOfNames;
    UINT32   AddressOfFunctions;     // RVA from base of image
    UINT32   AddressOfNames;         // RVA from base of image
    UINT32   AddressOfNameOrdinals;  // RVA from base of image
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

//
// Import Format
//

typedef struct _IMAGE_IMPORT_BY_NAME {
    UINT16    Hint;
    UINT8    Name[1];
} IMAGE_IMPORT_BY_NAME, *PIMAGE_IMPORT_BY_NAME;

//#include "pshpack8.h"                       // Use align 8 for the 64-bit IAT.

#pragma pack(push, 8)
typedef struct _IMAGE_THUNK_DATA64 {
    union {
        UINT64 ForwarderString;  // PBYTE 
        UINT64 Function;         // PULONG
        UINT64 Ordinal;
        UINT64 AddressOfData;    // PIMAGE_IMPORT_BY_NAME
    } u1;
} IMAGE_THUNK_DATA64;
typedef IMAGE_THUNK_DATA64 * PIMAGE_THUNK_DATA64;
#pragma pack(pop)

//#include "poppack.h"                        // Back to 4 byte packing
#pragma pack(push, 4)
typedef struct _IMAGE_THUNK_DATA32 {
    union {
        UINT32 ForwarderString;      // PBYTE 
        UINT32 Function;             // PULONG
        UINT32 Ordinal;
        UINT32 AddressOfData;        // PIMAGE_IMPORT_BY_NAME
    } u1;
} IMAGE_THUNK_DATA32;
typedef IMAGE_THUNK_DATA32 * PIMAGE_THUNK_DATA32;
#pragma pack(pop)

#define IMAGE_ORDINAL_FLAG64 0x8000000000000000
#define IMAGE_ORDINAL_FLAG32 0x80000000
#define IMAGE_ORDINAL64(Ordinal) (Ordinal & 0xffff)
#define IMAGE_ORDINAL32(Ordinal) (Ordinal & 0xffff)
#define IMAGE_SNAP_BY_ORDINAL64(Ordinal) ((Ordinal & IMAGE_ORDINAL_FLAG64) != 0)
#define IMAGE_SNAP_BY_ORDINAL32(Ordinal) ((Ordinal & IMAGE_ORDINAL_FLAG32) != 0)


//
// Import Descriptor.
//

#define IMAGE_ORDINAL_FLAG              IMAGE_ORDINAL_FLAG32
#define IMAGE_ORDINAL(Ordinal)          IMAGE_ORDINAL32(Ordinal)
#define IMAGE_SNAP_BY_ORDINAL(Ordinal)  IMAGE_SNAP_BY_ORDINAL32(Ordinal)

typedef struct _IMAGE_IMPORT_DESCRIPTOR {
    union {
        UINT32   Characteristics;            // 0 for terminating null import descriptor
        UINT32   OriginalFirstThunk;         // RVA to original unbound IAT (PIMAGE_THUNK_DATA)
    } u;
    UINT32   TimeDateStamp;                  // 0 if not bound,
                                            // -1 if bound, and real date\time stamp
                                            //     in IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT (new BIND)
                                            // O.W. date/time stamp of DLL bound to (Old BIND)

    UINT32   ForwarderChain;                 // -1 if no forwarders
    UINT32   Name;
    UINT32   FirstThunk;                     // RVA to IAT (if bound this IAT has actual addresses)
} IMAGE_IMPORT_DESCRIPTOR;
typedef IMAGE_IMPORT_DESCRIPTOR *PIMAGE_IMPORT_DESCRIPTOR;

#pragma pack(pop)




#pragma pack(push, 2)

typedef struct _IMAGE_BASE_RELOCATION {
    UINT32 VirtualAddress;
    UINT32 SizeOfBlock;
//  UINT16    TypeOffset[1];
} IMAGE_BASE_RELOCATION, *PIMAGE_BASE_RELOCATION;

//
// Based relocation types.
//

#define IMAGE_REL_BASED_ABSOLUTE              0
#define IMAGE_REL_BASED_HIGH                  1
#define IMAGE_REL_BASED_LOW                   2
#define IMAGE_REL_BASED_HIGHLOW               3
#define IMAGE_REL_BASED_HIGHADJ               4
#define IMAGE_REL_BASED_MIPS_JMPADDR          5
#define IMAGE_REL_BASED_MIPS_JMPADDR16        9
#define IMAGE_REL_BASED_IA64_IMM64            9
#define IMAGE_REL_BASED_DIR64                 10

typedef union _IMAGE_TYPE_OFFSET {
	struct
	{
		UINT16 Offset:12;
		UINT16 Type:4;
	} s;
	UINT16 Bytes;
} IMAGE_TYPE_OFFSET, *PIMAGE_TYPE_OFFSET;

#pragma pack(pop)




typedef union _IMAGE_NT_HEADERS_3264 {
	IMAGE_NT_HEADERS32 Nt32;
	IMAGE_NT_HEADERS64 Nt64;
} IMAGE_NT_HEADERS_3264, *PIMAGE_NT_HEADERS_3264;


#endif // !__PEIMAGE_H__
