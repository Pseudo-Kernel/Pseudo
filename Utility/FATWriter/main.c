
#include <stdio.h>
#include <Windows.h>

#include "device.h"
#include "fat.h"

RAW_CONTEXT DevRawDevice;




BOOLEAN
APIENTRY
DevRawReadBlock(
	IN HANDLE DeviceHandle,
	IN PVOID Buffer,
	IN UINT32 BufferLength,
	OPTIONAL OUT UINT32 *BytesRead)
{
	ULONG ReturnLength = 0;

	if (!ReadFile(DeviceHandle, Buffer, BufferLength, &ReturnLength, NULL))
		return FALSE;

	if (ReturnLength < BufferLength)
		return FALSE;

	if (BytesRead)
		*BytesRead = ReturnLength;

	return TRUE;
}

BOOLEAN
APIENTRY
DevRawWriteBlock(
	IN HANDLE DeviceHandle,
	IN PVOID Buffer,
	IN UINT32 BufferLength,
	OPTIONAL OUT UINT32 *BytesWritten)
{
	ULONG ReturnLength = 0;

	if (!WriteFile(DeviceHandle, Buffer, BufferLength, &ReturnLength, NULL))
		return FALSE;

	if (ReturnLength < BufferLength)
		return FALSE;

	if (BytesWritten)
		*BytesWritten = ReturnLength;

	return TRUE;
}

BOOLEAN
APIENTRY
DevRawSetPosition(
	IN HANDLE DeviceHandle,
	IN ULONG64 Position)
{
	return
		SetFilePointer(DeviceHandle, (LONG)Position, (LONG *)&Position + 1, FILE_BEGIN)
			!= INVALID_SET_FILE_POINTER;
}

BOOLEAN
APIENTRY
DevRawFlush(
	IN HANDLE DeviceHandle)
{
	VHD_FOOTER_DATA Footer;
	ULONG FileSize;
	INT i;
	UINT32 Checksum;
	ULONG BytesWritten;

	FileSize = GetFileSize(DeviceHandle, NULL);

	ZeroMemory(&Footer, sizeof(Footer));
	memcpy(Footer.Cookie, "conectix", 8);

	Footer.Features = ToNet32(0x02);
	Footer.FileFormatVersion = ToNet32(0x10000);
	Footer.DataOffset = ToNet64(0xffffffffffffffffULL);
	Footer.TimeStamp = 0;
	memcpy(&Footer.CreatorApplication, "vpc ", 4);
	Footer.CreatorVersion = ToNet32(0x00010000);
	Footer.CreatorHostOS = ToNet32(0x5769326b);
	Footer.OriginalSize = ToNet64(FileSize);
	Footer.CurrentSize = ToNet64(FileSize);
	Footer.DiskGeometry = 0;
	Footer.DiskType = ToNet32(2); // fixed
	Footer.Checksum = 0;

	*(UINT64 *)&Footer.UUID[0] = 0x0102030405060708ULL;
	*(UINT64 *)&Footer.UUID[8] = 0xf0e0d0c0b0a09080ULL;
	Footer.SavedState = 0;

	for (i = Checksum = 0; i < sizeof(Footer); i++)
	{
		Checksum += ((PUCHAR)&Footer)[i];
	}

	Footer.Checksum = ToNet32(~Checksum);

	SetFilePointer(DeviceHandle, 0, NULL, FILE_END);

	WriteFile(DeviceHandle, &Footer, sizeof(Footer), &BytesWritten, NULL);
	SetEndOfFile(DeviceHandle);

	return FlushFileBuffers(DeviceHandle);
}

VOID
ShowUsage(
	wchar_t **Argv)
{
	wprintf(L"Usage: %s --imagepath <image_path> --script <script_path>\n\n", Argv[0]);
}

int
wcs2int(
	wchar_t *s)
{
	int v = 0;
	char as[256];
	size_t len = wcstombs(as, s, sizeof(as));

	if (as[0] == '0' && as[1] == 'x')
	{
//		v = wcstol(s + 2, s + wcslen(s), 16);
//		swscanf(&s[2], "%x", &v);
		sscanf(&as[2], "%x", &v);
	}
	else
	{
//		v = wcstol(s, s + wcslen(s), 10);
//		swscanf(s, "%d", &v);
		sscanf(as, "%d", &v);
	}

	return v;
}

int
ExecuteScript(
	wchar_t *ImagePath, 
	FILE *ScriptFile)
{
	static struct {
		INT Id;
		INT Args;
		wchar_t *Cmd;
	} CommandTable[] = 
	{
		{ 0, 1, L"create-image", }, // args: <image_byte_size>
		{ 1, 1, L"partition-type", }, // args: <mbr | gpt>
		{ 2, 3, L"create-partition", }, // args: <partition_num> <start_byte_offset> <part_byte_size>
		{ 3, 1, L"select-partition", }, // args: <partition_num>
		{ 4, 0, L"partition-set-esp", }, // args: none
		{ 5, 1, L"fat.format", }, // args: <cluster_size>
		{ 6, 1, L"fat.destpath", }, // args: <copy_dest_path#>
		{ 7, 1, L"fat.copy", }, // args: <source_path#>
		{ 8, 0, L"end", }, // args: none
		{ -1, 0, L";", },
	};

	wchar_t LineBuffer[2048];
	HANDLE DeviceHandle;
	int i;

	BOOLEAN IsGpt = TRUE;

	for (;;)
	{
		PWSTR *Argv;
		INT Argc = 0;

		BOOLEAN Return = FALSE;
		INT ReturnCode = 0;

		LineBuffer[0] = 0;

		fgetws(LineBuffer, _countof(LineBuffer), ScriptFile);
		if (!LineBuffer[0] || LineBuffer[0] == 10 || LineBuffer[0] == 13)
			break;

		Argv = CommandLineToArgvW(LineBuffer, &Argc);

		if (Argv && Argc > 0 && Argv[Argc - 1][wcslen(Argv[Argc - 1]) - 1] == 10)
			Argv[Argc - 1][wcslen(Argv[Argc - 1]) - 1] = 0;

		for (i = 0; i < _countof(CommandTable); i++)
		{
			if (!wcsicmp(Argv[0], CommandTable[i].Cmd))
			{
				if (CommandTable[i].Id < 0)
					break;

				if (Argc < CommandTable[i].Args)
					break;

				switch (i)
				{
				case 0: // create-image
					{
						ULONG SizeOfImage;

						DeviceHandle = CreateFile(ImagePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
						if (DeviceHandle == INVALID_HANDLE_VALUE)
						{
							wprintf(L"error: failed to open `%s'\n", ImagePath);
							return GetLastError();
						}

						SizeOfImage = wcs2int(Argv[1]);
						SizeOfImage = (SizeOfImage + 0x1ff) & ~0x1ff;

						SetFilePointer(DeviceHandle, 0, NULL, FILE_BEGIN);
						SetFilePointer(DeviceHandle, SizeOfImage, NULL, FILE_CURRENT);
						SetEndOfFile(DeviceHandle);

						SetFilePointer(DeviceHandle, 0, NULL, FILE_BEGIN);

						DevRawDevice.SizeOfMedia = SizeOfImage;
						DevRawDevice.hDevice = DeviceHandle;
					}
					break;

				case 1: // partition-type
					if (!wcsicmp(Argv[1], L"mbr"))
					{
						wprintf(L"mbr is not supported yet\n");
						ReturnCode = ERROR_NOT_SUPPORTED;
						Return = TRUE;
					}
					else if (!wcsicmp(Argv[1], L"gpt"))
					{
						IsGpt = TRUE;
						PartInitializeGptHeader(&DevRawDevice, 0x100000);
					}
					break;

				case 2: //create-partition
					{
						ULONG PartitionIndex = wcs2int(Argv[1]);
						ULONG StartByteOffset = wcs2int(Argv[2]);
						ULONG PartitionSize = wcs2int(Argv[3]);

						PartSetGptEntry(&DevRawDevice, PartitionIndex, StartByteOffset, PartitionSize, 0, FALSE);
					}
					break;

				case 3: //select-partition
					{
						ULONG SelectedPartitionIndex = wcs2int(Argv[1]);
						DevRawDevice.SelectedPartition = SelectedPartitionIndex;
					}
					break;

				case 4: //partition-set-esp
					PartSetGptEntrySystemPartition(&DevRawDevice, DevRawDevice.SelectedPartition, TRUE);
					break;

				case 5: //fat.format
					{
						ULONG ClusterSize = wcs2int(Argv[1]);
						FatInitializeVolume(&DevRawDevice, FatVolumeType32, ClusterSize);
					}
					break;

				case 6: //fat.destpath
				case 7: //fat.copy
					break;

				case 8: //end
					PartWriteGpt(&DevRawDevice);

					DevRawDevice.Flush(DevRawDevice.hDevice);
					ReturnCode = ERROR_SUCCESS;
					Return = TRUE;
					break;
				}

				break;
			}
		}

		LocalFree(Argv);

		if (Return)
		{
			return ReturnCode;
		}
	}

	return 1;
}

int wmain(int argc, wchar_t **argv, wchar_t **envp)
{
	//
	// fatwriter --imagepath <image_path> --script <script_path>
	// create-image 0x2200000 ; <image_byte_size>
	// partition-type gpt
	// ; create-partition <partition_num> <start_byte_offset> <part_byte_size>
	// create-partition 0 0x100000 0x2000000
	// select-partition 0
	// partition-set-esp
	// fat.format 0x200 ; <cluster_size>
	// fat.copy boot \
	// 

	int ArgcDbgOverride = 5;
	wchar_t *ArgvDbgOverride[] =
	{
		argv[0],
		L"--imagepath",
		L"fatimage.vhd",
		L"--script",
		L"script.txt"
	};

	FILE *ScriptFile;

	DevRawDevice.ReadBlock = DevRawReadBlock;
	DevRawDevice.WriteBlock = DevRawWriteBlock;
	DevRawDevice.SetPosition = DevRawSetPosition;
	DevRawDevice.Flush = DevRawFlush;
	DevRawDevice.hDevice = NULL;
	DevRawDevice.SizeOfSector = 512;
	DevRawDevice.SizeOfMedia = 0;
	DevRawDevice.GPTEntries = NULL;


	if (IsDebuggerPresent())
	{
		argc = ArgcDbgOverride;
		argv = ArgvDbgOverride;
	}

	if (argc < 5 || 
		wcsicmp(argv[1], L"--imagepath") != 0 ||
		wcsicmp(argv[3], L"--script") != 0
		)
	{
		ShowUsage(argv);
		return ERROR_INVALID_PARAMETER;
	}

	ScriptFile = _wfopen(argv[4], "r");
	if (!ScriptFile)
	{
		wprintf(L"error: failed to open `%s'\n", argv[4]);
		return ERROR_INVALID_PARAMETER;
	}

	return ExecuteScript(argv[2], ScriptFile);
}

