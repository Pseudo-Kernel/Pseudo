#pragma once

//
// FAT file system structures.
//
// - From "Microsoft FAT Specification"
//        August 30 2005, Microsoft Corporation.
//

#pragma pack(push, 1)

typedef struct _FAT_BPB_EXT2 {
	// Used by BIOS Service - INT 13h
	UINT8 DriveNumber;
	UINT8 Reserved1;
	UINT8 BootSignature;
	UINT32 VolumeSerial;
	UINT8 VolumeLabel[11];
	UINT8 FileSystemType[8];	// Informational purpose only.
} FAT_BPB_EXT2, *PFAT_BPB_EXT2;

typedef struct _FAT_BPB_EXT {
	FAT_BPB_EXT2 Ext2;
	UINT8 BootCode[448];
	UINT8 Signature[2];			// Must be 0x55 0xaa
} FAT_BPB_EXT, *PFAT_BPB_EXT;

typedef struct _FAT32_BPB_EXT {
	UINT32 SectorsPerFAT32;

	// ExtendedFlags[3:0] Zero-based number of active FAT.
	// ExtendedFlags[6:4] Reserved
	// ExtendedFlags[7] 0 - FAT is mirrored at runtime,
	//                  1 - Only one FAT is active (mirroring disabled mode)
	// ExtendedFlags[15:8] Reserved
	UINT16 ExtendedFlags;

	// Version[7:0] - Minor version.
	// Version[15:8] - Major version.
	UINT16 Version;

	// Cluster number of RootDirectory.
	UINT32 ClusterRootDir;

	// FSInfo sector (not cluster).
	UINT16 SectorFsInfo;

	// Backup bootsector.
	UINT16 SectorBootSectBackup;

	UINT8 Reserved[12];
	FAT_BPB_EXT2 Ext2;

	UINT8 BootCode[420];
	UINT8 Signature[2];			// Must be 0x55 0xaa
} FAT32_BPB_EXT, *PFAT32_BPB_EXT;

typedef struct _FAT_BPB {
	UINT8 Jmp[3];
	UINT8 OemName[8];
	UINT16 BytesPerSector;
	UINT8 SectorsPerCluster;
	UINT16 ReservedSectors;
	UINT8 NumberOfFATs;
	UINT16 DirEntryCountForRoot;	// [FAT12/16] Number of FAT_DIR_ENTRY in RootDirectory.
	UINT16 TotalSectors16;			// [FAT12/16]
	UINT8 MediaType;
	UINT16 SectorsPerFAT;			// [FAT12/16]

	// Used by BIOS Service - INT 13h
	UINT16 SectorsPerTrack;
	UINT16 NumberOfHeads;

	UINT32 HiddenSectors;
	UINT32 TotalSectors32;

	union
	{
		FAT_BPB_EXT FatExt;
		FAT32_BPB_EXT Fat32Ext;
	} u;
} FAT_BPB, *PFAT_BPB;

#pragma pack(pop)

//
// FAT32 FSINFO.
//

#pragma pack(push, 1)

typedef struct _FAT32_FSINFO {
	UINT32 Signature;	// 0x41615252
	UINT8 Reserved[480];
	UINT32 Signature2;	// 0x61417272
	UINT32 FreeClusterCount;	// 0xffffffff for unknown
	UINT32 ClusterNextFree;		// 0xffffffff for unknown
	UINT32 Reserved2[3];
	UINT32 Signature3;			// 0xaa550000
} FAT32_FSINFO, *PFAT32_FSINFO;

#define	FAT32_FSINFO_SIGNATURE				0x41615252
#define	FAT32_FSINFO_SIGNATURE2				0x61417272
#define	FAT32_FSINFO_SIGNATURE3				0xaa550000

#pragma pack(pop)

//
// FAT Directory Entry.
//

#pragma pack(push, 1)

#define	ATTR_READ_ONLY		0x01
#define	ATTR_HIDDEN			0x02
#define	ATTR_SYSTEM			0x04
#define	ATTR_VOLUME_ID		0x08
#define	ATTR_DIRECTORY		0x10
#define	ATTR_ARCHIVE		0x20
#define	ATTR_LONG_NAME		( ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID )
#define ATTR_LONG_NAME_MASK	( ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID | ATTR_DIRECTORY | ATTR_ARCHIVE )

typedef struct _FAT_DIRENT {
	struct
	{
		UINT8 ShortName[8 + 3];
		UINT8 Attribute;
		UINT8 Reserved1;
		UINT8 CreationTimeTenth;
		UINT16 CreationTimeSecond;
		UINT16 CreationDate;
		UINT16 LastAccessedDate;
		UINT16 ClusterToDataHigh;
		UINT16 LastModifiedTimeSecond;
		UINT16 LastModifiedDate;
		UINT16 ClusterToDataLow;
		UINT32 SizeOfFile;
	} Short;

	struct
	{
		UINT8 SequenceId;
		UINT16 LongName1[5];
		UINT8 Attribute;
		UINT8 MustBeZero1;
		UINT8 Checksum;
		UINT16 LongName2[6];
		UINT16 MustBeZero2;
		UINT16 LongName3[2];
	} Long;
} FAT_DIRENT, *PFAT_DIRENT;

#pragma pack(pop)

typedef	struct _FAT_DRIVER_CONTEXT {
	UINT32 SizeOfVolume;
	UINT32 ClusterCount;
	UINT16 SectorsPerCluster;
} FAT_DRIVER_CONTEXT, *PFAT_DRIVER_CONTEXT;


typedef enum _FAT_VOLUME_TYPE {
	FatVolumeType12,
	FatVolumeType16,
	FatVolumeType32,
} FAT_VOLUME_TYPE;

#define	COND_FAT12(_x)				( (_x)->ClusterCount < 4085 )
#define	COND_FAT16(_x)				( (_x)->ClusterCount < 65525 )
#define	COND_FAT32(_x)				( 65525 <= (_x)->ClusterCount )



typedef struct _FAT_VOLUME_SIZE {
	UINT32 ClusterCount;
	UINT8 SectorsPerCluster;
	UINT16 BytesPerSector;
} FAT_VOLUME_SIZE, *PFAT_VOLUME_SIZE;

extern FAT_VOLUME_SIZE FatSzVolumeTableFat16[];
extern FAT_VOLUME_SIZE FatSzVolumeTableFat32[];




BOOLEAN
APIENTRY
FatInitializeVolume(
	IN struct _RAW_CONTEXT *Context,
	IN FAT_VOLUME_TYPE VolumeType,
	IN ULONG ClusterSize);

