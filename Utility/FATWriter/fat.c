
#include <stdio.h>
#include <windows.h>

#include "device.h"
#include "gpt.h"
#include "fat.h"

FAT_VOLUME_SIZE FatSzVolumeTableFat16[] = 
{
	{ 8400, 0, 512 }, /* disks up to 4.1 MB, the 0 value for SecPerClusVal trips an error */
	{ 32680, 2, 512 }, /* disks up to 16 MB, 1k cluster */
	{ 262144, 4, 512 }, /* disks up to 128 MB, 2k cluster */
	{ 524288, 8, 512 }, /* disks up to 256 MB, 4k cluster */
	{ 1048576, 16, 512 }, /* disks up to 512 MB, 8k cluster */
					 /* The entries after this point are not used unless FAT16 is forced */
	{ 2097152, 32, 512 }, /* disks up to 1 GB, 16k cluster */
	{ 4194304, 64, 512 }, /* disks up to 2 GB, 32k cluster */
	{ 0xFFFFFFFF, 0, 512 } /*any disk greater than 2GB, 0 value for SecPerClusVal trips an error */
};

FAT_VOLUME_SIZE FatSzVolumeTableFat32[] =
{
	{ 66600, 0, 512 }, /* disks up to 32.5 MB, the 0 value for SecPerClusVal trips an error */
	{ 532480, 1, 512 }, /* disks up to 260 MB, .5k cluster */
	{ 16777216, 8, 512 }, /* disks up to 8 GB, 4k cluster */
	{ 33554432, 16, 512 }, /* disks up to 16 GB, 8k cluster */
	{ 67108864, 32, 512 }, /* disks up to 32 GB, 16k cluster */
	{ 0xFFFFFFFF, 64, 512 }/* disks greater than 32GB, 32k cluster */
};

UINT8 FatIllegalShortNameChar[] =
{
	// Values less then 0x20 (except for 0x05)
	0x00, 0x01, 0x02, 0x03, 0x04, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,

	// Special
	0x22, 0x2a, 0x2b, 0x2c, 0x2e, 0x2f, 0x3a, 0x3b,
	0x3c, 0x3d, 0x3e, 0x3f, 0x5b, 0x5c, 0x5d, 0x7c,

	// Lowercase characters
	0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 
	0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 
	0x78, 0x79, 0x7a, 
};


BOOLEAN
APIENTRY
FatInitializeVolume(
	IN struct _RAW_CONTEXT *Context, 
	IN FAT_VOLUME_TYPE VolumeType, 
	IN ULONG ClusterSize)
{
	ULONG VolumeSize;
	ULONG BytesPerSector;
	ULONG SectorsPerCluster;
	ULONG SectorsPerClusterValid;
	ULONG TotalSectors;
//	ULONG TotalDataClusters;
	ULONG NumberOfFATs;

	ULONG DirEntryCountForRoot;
	ULONG SectorsPerRootDirectory;
	ULONG ReservedSectors;
	ULONG SectorsPerFAT;

//	ULONG FirstSectorForData;
	ULONG FirstSectorForFAT;
	ULONG Temp1, Temp2;

	UCHAR Fat[512];

	INT i;
	FAT_BPB Bpb;

	ULONG Offset;

	GPT_PARTITION_ENTRY_HEADER EntryHeader;
	static UCHAR ZeroBuffer[4096];

	/*
		FAT File System Layout:
		[BootSect]
		[ReservedSect]
		[FAT #1 ... FAT #N]
		[Root Directory]
		[Cluster 2] <- Normally Cluster #2
		[Cluster 3]
		...
	*/

	BytesPerSector = Context->SizeOfSector;

	if (BytesPerSector != 0x200 &&
		BytesPerSector != 0x400 &&
		BytesPerSector != 0x800 &&
		BytesPerSector != 0x1000)
		return FALSE;

	if (ClusterSize % BytesPerSector)
		return FALSE;

	if (!PartGetGptEntryHeader(Context, &EntryHeader))
		return FALSE;

	VolumeSize = (ULONG)((EntryHeader.EndingLBA - EntryHeader.StartingLBA + 1) * BytesPerSector);

	SectorsPerCluster = ClusterSize / BytesPerSector;
	TotalSectors = VolumeSize / BytesPerSector;
	NumberOfFATs = 1;

	// FIXME : Compare size correctly!
	switch (VolumeType)
	{
	case FatVolumeType12:
		if (VolumeSize > 0x400000)
			return FALSE;

		SectorsPerClusterValid = 1;
		DirEntryCountForRoot = 512;
		break;

	case FatVolumeType16:
		SectorsPerClusterValid = 0;
		DirEntryCountForRoot = 512;

		for (i = 0; i < _countof(FatSzVolumeTableFat16); i++)
		{
			UINT32 FatSzVolume = FatSzVolumeTableFat16[i].ClusterCount *
				FatSzVolumeTableFat16[i].BytesPerSector *
				FatSzVolumeTableFat16[i].SectorsPerCluster;
			if (VolumeSize <= FatSzVolume)
			{
				SectorsPerClusterValid = FatSzVolumeTableFat16[i].SectorsPerCluster;
				break;
			}
		}
		break;

	case FatVolumeType32:
		SectorsPerClusterValid = 0;
		DirEntryCountForRoot = VolumeSize / sizeof(FAT_DIRENT) / 0x1000;

		for (i = 0; i < _countof(FatSzVolumeTableFat32); i++)
		{
			UINT32 FatSzVolume = FatSzVolumeTableFat32[i].ClusterCount *
				FatSzVolumeTableFat32[i].BytesPerSector *
				FatSzVolumeTableFat32[i].SectorsPerCluster;
			if (VolumeSize <= FatSzVolume)
			{
				SectorsPerClusterValid = FatSzVolumeTableFat32[i].SectorsPerCluster;
				break;
			}
		}
		break;

	default:
		return FALSE;
	}

/*
	FATxx:
	BPB.ReservedSectors = 2
	BPB.NumberOfFATs = 1
	BPB.Media = 0xf0 (removable) or 0xf8 (fixed)

	BPB.FATSz16 = FAT12/16: ..., FAT32: 0
	BPB.TotalSectors16 = FAT12/16: ..., FAT32: 0
	BPB.RootEntCnt = FAT12/16: 512, FAT32: ...
	BPB.TotalSectors32 = FAT12/16: 0, FAT32: ...

*/

	if (!SectorsPerCluster)
		SectorsPerCluster = SectorsPerClusterValid;

	if (SectorsPerCluster != SectorsPerClusterValid)
		return FALSE;


	// Align first FAT to cluster size
	// [BootSect] [FSINFO(FAT32 only)] ... 

	// FIXME : reserved sectors calculation
	FirstSectorForFAT = (1 + 1 + (SectorsPerCluster - 1)) / SectorsPerCluster; // for FSINFO(FAT32) or reserved
	ReservedSectors = FirstSectorForFAT - 1;

	SectorsPerRootDirectory = (DirEntryCountForRoot * sizeof(FAT_DIRENT)) + (BytesPerSector - 1) / BytesPerSector;
	Temp1 = VolumeSize - (ReservedSectors + SectorsPerRootDirectory);
	Temp2 = 256 * SectorsPerCluster + NumberOfFATs;

	if (VolumeType == FatVolumeType32)
		Temp2 /= 2;

	SectorsPerFAT = (Temp1 + (Temp2 - 1)) / Temp2;

#if 0
	//	ReservedSectors + (NumberOfFATs * SectorsPerFAT) + SectorsPerRootDirectory;
	//	TotalDataClusters = VolumeSize / ClusterSize;

	if (TotalClusters < 4085)
	{
		// FAT12
	}
	else if (TotalClusters < 65525)
	{
		// FAT16
	}
	else
	{
		// FAT32
	}
#endif


	//
	// 1. Create the BPB.
	//

	memset(&Bpb, 0, sizeof(Bpb));
	Bpb.Jmp[0] = 0xeb;
	Bpb.Jmp[1] = sizeof(Bpb);
	Bpb.Jmp[2] = 0x90;

	Bpb.BytesPerSector = (UINT16)BytesPerSector;
	Bpb.HiddenSectors = 2; // FIXME: First Data Sector

	Bpb.MediaType = 0xf8;
	Bpb.NumberOfFATs = (UINT8)NumberOfFATs;
	Bpb.NumberOfHeads = 0;
	Bpb.SectorsPerTrack = 0;
	Bpb.ReservedSectors = (UINT16)ReservedSectors;
	Bpb.SectorsPerCluster = (UINT8)SectorsPerCluster;
	strncpy(Bpb.OemName, "PSEUDO  ", 8);

	switch (VolumeType)
	{
	case FatVolumeType12:
		Bpb.TotalSectors16 = (UINT16)TotalSectors;
		Bpb.SectorsPerFAT = (UINT16)SectorsPerFAT;
		Bpb.DirEntryCountForRoot = (UINT16)DirEntryCountForRoot;
		Bpb.u.FatExt.Signature[0] = 0x55;
		Bpb.u.FatExt.Signature[1] = 0xaa;
		strncpy(Bpb.u.FatExt.Ext2.FileSystemType, "FAT12   ", 8);
		Bpb.u.FatExt.Ext2.VolumeSerial = 0xdeadbeef;
		break;

	case FatVolumeType16:
		Bpb.TotalSectors16 = (UINT16)TotalSectors;
		Bpb.SectorsPerFAT = (UINT16)SectorsPerFAT;
		Bpb.DirEntryCountForRoot = (UINT16)DirEntryCountForRoot;
		Bpb.u.FatExt.Signature[0] = 0x55;
		Bpb.u.FatExt.Signature[1] = 0xaa;
		strncpy(Bpb.u.FatExt.Ext2.FileSystemType, "FAT16   ", 8);
		Bpb.u.FatExt.Ext2.VolumeSerial = 0xdeadbeef;
		break;

	case FatVolumeType32:
		Bpb.TotalSectors32 = TotalSectors;
		Bpb.DirEntryCountForRoot = 0; // Zero if FAT32, always.
		Bpb.u.Fat32Ext.Signature[0] = 0x55;
		Bpb.u.Fat32Ext.Signature[1] = 0xaa;
		Bpb.u.Fat32Ext.ClusterRootDir = 2; // Should be 2 or first usable cluster
		Bpb.u.Fat32Ext.ExtendedFlags = 0x80; // Only one FAT is active
		Bpb.u.Fat32Ext.SectorBootSectBackup = 0;
		Bpb.u.Fat32Ext.SectorFsInfo = 1;
		Bpb.u.Fat32Ext.Version = 0;
		strncpy(Bpb.u.FatExt.Ext2.FileSystemType, "FAT32   ", 8);
		Bpb.u.Fat32Ext.SectorFsInfo = 1; // Usually 1
		Bpb.u.Fat32Ext.Ext2.VolumeSerial = 0xdeadbeef;
		Bpb.u.Fat32Ext.SectorsPerFAT32 = SectorsPerFAT;
		break;

	default:
		return FALSE;
	}

	// Write all zero.
	for(Offset = 0; Offset < VolumeSize; Offset += sizeof(ZeroBuffer))
		PartWritePartition(Context, Offset, ZeroBuffer, sizeof(ZeroBuffer), NULL);


	// Write to the Media.
	PartWritePartition(Context, 0, &Bpb, sizeof(Bpb), NULL);

	if (VolumeType == FatVolumeType32)
	{
		// Write the FSINFO for FAT32.
		FAT32_FSINFO FsInfo;

		memset(&FsInfo, 0, sizeof(FsInfo));
		FsInfo.Signature = FAT32_FSINFO_SIGNATURE;
		FsInfo.Signature2 = FAT32_FSINFO_SIGNATURE2;
		FsInfo.Signature3 = FAT32_FSINFO_SIGNATURE3;
		FsInfo.ClusterNextFree = 0xffffffff;
		FsInfo.FreeClusterCount = 0xffffffff;

		PartWritePartition(Context, Bpb.u.Fat32Ext.SectorFsInfo * BytesPerSector,
			&FsInfo, sizeof(FsInfo), NULL);
	}

	//
	// Initialize and write FAT.
	// 1st and 2nd entry is in allocated state.
	//
	// ClusterFreed        0x000   -   0x000
	// ClusterAllocated    0x002   -   0x???
	// ClusterReserved     0x???+1 -   0xff6
	// ClusterBad          0xff7   -   0xff7
	// ClusterReserved2    0xff8   -   0xffe
	// ClusterAllocatedEnd 0xfff   -   0xfff
	//

	memset(Fat, 0, sizeof(Fat));
	switch (VolumeType)
	{
	case FatVolumeType12:
		// 0xfff 0xfff
		*(PUINT32)Fat = 0xffffff;
		break;

	case FatVolumeType16:
		// 0xffff 0xffff
		*(PUINT32)Fat = 0xffffffff;
		break;

	case FatVolumeType32:
		{
			UINT32 ClustersPerRootDirectory = (SectorsPerRootDirectory + SectorsPerCluster - 1) / SectorsPerCluster;
			ULONG ClusterNumber = 0;
			UINT32 FatEntryCount = sizeof(Fat) / sizeof(UINT32);
			UINT32 SectorsWritten = 0;

			// 0xffffffff 0xffffffff
			*(PUINT32)Fat = 0xffffffff;
			*((PUINT32)Fat + 1) = 0xffffffff;

			// NOTE : Only the FAT32 needs to allocate RootDirectory from FAT.
			for (ClusterNumber = 2; ClusterNumber < ClustersPerRootDirectory + 2; ClusterNumber++)
			{
				BOOLEAN LastCluster = FALSE;

				if (ClusterNumber + 1 == ClustersPerRootDirectory + 2)
					LastCluster = TRUE;

				if (!(ClusterNumber % FatEntryCount))
				{
					// Write and flush.
					PartWritePartition(Context, (FirstSectorForFAT + SectorsWritten) * BytesPerSector, Fat, sizeof(Fat), NULL);
					memset(Fat, 0, sizeof(Fat));
					SectorsWritten++;
				}

				if (LastCluster)
					*((PUINT32)Fat + (ClusterNumber % FatEntryCount)) = 0xffffffff;
				else
					*((PUINT32)Fat + (ClusterNumber % FatEntryCount)) = ClusterNumber + 1;

				if (LastCluster)
				{
					// Write and flush.
					PartWritePartition(Context, (FirstSectorForFAT + SectorsWritten) * BytesPerSector, Fat, sizeof(Fat), NULL);
					memset(Fat, 0, sizeof(Fat));
					SectorsWritten++;
				}
			}
		}
		break;

	default:
		return FALSE;
	}


	Context->SectorsPerCluster = SectorsPerCluster;

	return TRUE;
}

