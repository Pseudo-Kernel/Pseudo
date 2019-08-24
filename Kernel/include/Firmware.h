#pragma once

typedef	UPTR										EFI_STATUS;
typedef	VOID										*EFI_HANDLE;

typedef	GUID										EFI_GUID;
typedef	U64											EFI_PHYSICAL_ADDRESS;
typedef	U64											EFI_VIRTUAL_ADDRESS;

typedef	struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL		EFI_SIMPLE_TEXT_INPUT_PROTOCOL;
typedef	struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL		EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef	struct _EFI_BOOT_SERVICES					EFI_BOOT_SERVICES;
typedef	struct _EFI_RUNTIME_SERVICES				EFI_RUNTIME_SERVICES;

typedef struct _EFI_CAPSULE_HEADER					EFI_CAPSULE_HEADER;


#define	EFIAPI										__cdecl


typedef struct _EFI_TABLE_HEADER {
	U64 Signature;
	U32 Revision;
	U32 HeaderSize;
	U32 CRC32;
	U32 Reserved;
} EFI_TABLE_HEADER;



typedef
EFI_STATUS
(EFIAPI *EFI_GET_VARIABLE)(
	IN     CHAR16                      *VariableName,
	IN     EFI_GUID                    *VendorGuid,
	OUT    U32                         *Attributes, OPTIONAL
	IN OUT UPTR                        *DataSize,
	OUT    VOID                        *Data        OPTIONAL
	);

typedef
EFI_STATUS
(EFIAPI *EFI_GET_NEXT_VARIABLE_NAME)(
	IN OUT UPTR                     *VariableNameSize,
	IN OUT CHAR16                   *VariableName,
	IN OUT EFI_GUID                 *VendorGuid
	);

typedef
EFI_STATUS
(EFIAPI *EFI_SET_VARIABLE)(
	IN  CHAR16                       *VariableName,
	IN  EFI_GUID                     *VendorGuid,
	IN  U32                          Attributes,
	IN  UPTR                         DataSize,
	IN  VOID                         *Data
	);

typedef struct {
	U16  Year;
	U8   Month;
	U8   Day;
	U8   Hour;
	U8   Minute;
	U8   Second;
	U8   Pad1;
	U32  Nanosecond;
	S16  TimeZone;
	U8   Daylight;
	U8   Pad2;
} EFI_TIME;

COMPILER_PACK(4)
typedef struct {
	U32    Resolution;
	U32    Accuracy;
	BOOLEAN   SetsToZero;
} EFI_TIME_CAPABILITIES;

typedef
EFI_STATUS
(EFIAPI *EFI_GET_TIME)(
	OUT  EFI_TIME                    *Time,
	OUT  EFI_TIME_CAPABILITIES       *Capabilities OPTIONAL
	);

typedef
EFI_STATUS
(EFIAPI *EFI_SET_TIME)(
	IN  EFI_TIME                     *Time
	);

typedef
EFI_STATUS
(EFIAPI *EFI_GET_WAKEUP_TIME)(
	OUT BOOLEAN                     *Enabled,
	OUT BOOLEAN                     *Pending,
	OUT EFI_TIME                    *Time
	);

typedef
EFI_STATUS
(EFIAPI *EFI_SET_WAKEUP_TIME)(
	IN  BOOLEAN                      Enable,
	IN  EFI_TIME                     *Time   OPTIONAL
	);


typedef enum {
	EfiResetCold,
	EfiResetWarm,
	EfiResetShutdown,
	EfiResetPlatformSpecific
} EFI_RESET_TYPE;

typedef
VOID
(EFIAPI *EFI_RESET_SYSTEM)(
	IN EFI_RESET_TYPE           ResetType,
	IN EFI_STATUS               ResetStatus,
	IN UPTR                     DataSize,
	IN VOID                     *ResetData OPTIONAL
	);

typedef
EFI_STATUS
(EFIAPI *EFI_GET_NEXT_MONOTONIC_COUNT)(
	OUT U64                  *Count
	);

typedef
EFI_STATUS
(EFIAPI *EFI_GET_NEXT_HIGH_MONO_COUNT)(
	OUT U32                  *HighCount
	);

typedef
EFI_STATUS
(EFIAPI *EFI_CONVERT_POINTER)(
	IN     UPTR                       DebugDisposition,
	IN OUT VOID                       **Address
	);



//
// Memory cacheability attributes
//

#define EFI_MEMORY_UC               0x0000000000000001ULL
#define EFI_MEMORY_WC               0x0000000000000002ULL
#define EFI_MEMORY_WT               0x0000000000000004ULL
#define EFI_MEMORY_WB               0x0000000000000008ULL
#define EFI_MEMORY_UCE              0x0000000000000010ULL
#define EFI_MEMORY_WP               0x0000000000001000ULL
#define EFI_MEMORY_RP               0x0000000000002000ULL
#define EFI_MEMORY_XP               0x0000000000004000ULL
#define EFI_MEMORY_RO               0x0000000000020000ULL
#define EFI_MEMORY_NV               0x0000000000008000ULL
#define EFI_MEMORY_MORE_RELIABLE    0x0000000000010000ULL
#define EFI_MEMORY_RUNTIME          0x8000000000000000ULL

#define EFI_MEMORY_DESCRIPTOR_VERSION 1


typedef enum {
	///
	/// Not used.
	///
	EfiReservedMemoryType,
	///
	/// The code portions of a loaded application.
	/// (Note that UEFI OS loaders are UEFI applications.)
	///
	EfiLoaderCode,
	///
	/// The data portions of a loaded application and the default data allocation
	/// type used by an application to allocate pool memory.
	///
	EfiLoaderData,
	///
	/// The code portions of a loaded Boot Services Driver.
	///
	EfiBootServicesCode,
	///
	/// The data portions of a loaded Boot Serves Driver, and the default data
	/// allocation type used by a Boot Services Driver to allocate pool memory.
	///
	EfiBootServicesData,
	///
	/// The code portions of a loaded Runtime Services Driver.
	///
	EfiRuntimeServicesCode,
	///
	/// The data portions of a loaded Runtime Services Driver and the default
	/// data allocation type used by a Runtime Services Driver to allocate pool memory.
	///
	EfiRuntimeServicesData,
	///
	/// Free (unallocated) memory.
	///
	EfiConventionalMemory,
	///
	/// Memory in which errors have been detected.
	///
	EfiUnusableMemory,
	///
	/// Memory that holds the ACPI tables.
	///
	EfiACPIReclaimMemory,
	///
	/// Address space reserved for use by the firmware.
	///
	EfiACPIMemoryNVS,
	///
	/// Used by system firmware to request that a memory-mapped IO region
	/// be mapped by the OS to a virtual address so it can be accessed by EFI runtime services.
	///
	EfiMemoryMappedIO,
	///
	/// System memory-mapped IO region that is used to translate memory
	/// cycles to IO cycles by the processor.
	///
	EfiMemoryMappedIOPortSpace,
	///
	/// Address space reserved by the firmware for code that is part of the processor.
	///
	EfiPalCode,
	///
	/// A memory region that operates as EfiConventionalMemory, 
	/// however it happens to also support byte-addressable non-volatility.
	///
	EfiPersistentMemory,
	EfiMaxMemoryType
} EFI_MEMORY_TYPE;

COMPILER_PACK(8)
typedef struct {
	EFI_MEMORY_TYPE       Type;
	EFI_PHYSICAL_ADDRESS  PhysicalStart;
	EFI_VIRTUAL_ADDRESS   VirtualStart;
	U64                   NumberOfPages;
	U64                   Attribute;
} EFI_MEMORY_DESCRIPTOR;

typedef
EFI_STATUS
(EFIAPI *EFI_SET_VIRTUAL_ADDRESS_MAP)(
	IN  UPTR                         MemoryMapSize,
	IN  UPTR                         DescriptorSize,
	IN  U32                          DescriptorVersion,
	IN  EFI_MEMORY_DESCRIPTOR        *VirtualMap
	);


typedef
EFI_STATUS
(EFIAPI *EFI_UPDATE_CAPSULE)(
	IN EFI_CAPSULE_HEADER     **CapsuleHeaderArray,
	IN UPTR                   CapsuleCount,
	IN EFI_PHYSICAL_ADDRESS   ScatterGatherList   OPTIONAL
	);

typedef
EFI_STATUS
(EFIAPI *EFI_QUERY_CAPSULE_CAPABILITIES)(
	IN  EFI_CAPSULE_HEADER     **CapsuleHeaderArray,
	IN  UPTR                   CapsuleCount,
	OUT U64                    *MaximumCapsuleSize,
	OUT EFI_RESET_TYPE         *ResetType
	);

typedef
EFI_STATUS
(EFIAPI *EFI_QUERY_VARIABLE_INFO)(
	IN  U32               Attributes,
	OUT U64               *MaximumVariableStorageSize,
	OUT U64               *RemainingVariableStorageSize,
	OUT U64               *MaximumVariableSize
	);

typedef struct _EFI_RUNTIME_SERVICES {
	EFI_TABLE_HEADER                Hdr;
	EFI_GET_TIME                    GetTime;
	EFI_SET_TIME                    SetTime;
	EFI_GET_WAKEUP_TIME             GetWakeupTime;
	EFI_SET_WAKEUP_TIME             SetWakeupTime;
	EFI_SET_VIRTUAL_ADDRESS_MAP     SetVirtualAddressMap;
	EFI_CONVERT_POINTER             ConvertPointer;
	EFI_GET_VARIABLE                GetVariable;
	EFI_GET_NEXT_VARIABLE_NAME      GetNextVariableName;
	EFI_SET_VARIABLE                SetVariable;
	EFI_GET_NEXT_HIGH_MONO_COUNT    GetNextHighMonotonicCount;
	EFI_RESET_SYSTEM                ResetSystem;
	EFI_UPDATE_CAPSULE              UpdateCapsule;
	EFI_QUERY_CAPSULE_CAPABILITIES  QueryCapsuleCapabilities;
	EFI_QUERY_VARIABLE_INFO         QueryVariableInfo;
} EFI_RUNTIME_SERVICES;

typedef struct {
	EFI_GUID                          VendorGuid;
	VOID                              *VendorTable;
} EFI_CONFIGURATION_TABLE;

COMPILER_PACK(8)
typedef struct _EFI_SYSTEM_TABLE {
	EFI_TABLE_HEADER                  Hdr;
	CHAR16                            *FirmwareVendor;
	U32                               FirmwareRevision;
	EFI_HANDLE                        ConsoleInHandle;
	EFI_SIMPLE_TEXT_INPUT_PROTOCOL    *ConIn;
	EFI_HANDLE                        ConsoleOutHandle;
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *ConOut;
	EFI_HANDLE                        StandardErrorHandle;
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *StdErr;
	EFI_RUNTIME_SERVICES              *RuntimeServices;
	EFI_BOOT_SERVICES                 *BootServices;
	UPTR                              NumberOfTableEntries;
	EFI_CONFIGURATION_TABLE           *ConfigurationTable;
} EFI_SYSTEM_TABLE;





typedef struct {
	U32            RedMask;
	U32            GreenMask;
	U32            BlueMask;
	U32            ReservedMask;
} EFI_PIXEL_BITMASK;

typedef enum {
	PixelRedGreenBlueReserved8BitPerColor, // BGRx8888
	PixelBlueGreenRedReserved8BitPerColor, // RGBx8888
	PixelBitMask,                          // See EFI_PIXEL_BITMASK.
	PixelBltOnly,                          // Physical framebuffer not supported.
	PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;

COMPILER_PACK(4)
typedef struct {
	U32                        Version;
	U32                        HorizontalResolution;
	U32                        VerticalResolution;
	EFI_GRAPHICS_PIXEL_FORMAT  PixelFormat;
	EFI_PIXEL_BITMASK          PixelInformation;
	U32                        PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

