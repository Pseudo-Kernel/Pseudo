
#include <stdio.h>
#include <windows.h>
#include <Shlwapi.h>
#include "Arch/Arch.h"

#include "zip.h"

#pragma comment(lib, "shlwapi.lib")


typedef struct _ZIP_WRITE_CONTEXT {
	HANDLE hZipFile;
	HANDLE hTempFile;
	ULONG CentralDirectoryCount;
} ZIP_WRITE_CONTEXT, *PZIP_WRITE_CONTEXT;

BOOLEAN
ZipAddFileRecordInternal(
	IN PZIP_WRITE_CONTEXT ZipContext, 
	IN CHAR16 *FileName, 
	IN ULONG NameIndex)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	ULONG FileSize = 0;
	BOOLEAN DirectoryFile;

	U32 Crc32 = 0;
	ZIP_LOCAL_FILE_HEADER LocalFileHeader;
	ZIP_CENTRAL_DIRECTORY_HEADER CentralFileHeader;

	CHAR16 *DestFileName;
	CHAR8 Utf8Name[MAX_PATH * 3];
	INT Utf8Length;

	PVOID FileBuffer = NULL;
	ULONG FileOffsetToLocalFileHeader;
	DWORD BytesRead, BytesWritten;

	INT i;

	ZeroMemory(&LocalFileHeader, sizeof(LocalFileHeader));
	ZeroMemory(&CentralFileHeader, sizeof(CentralFileHeader));

	DirectoryFile = !!PathIsDirectoryW(FileName);
	DestFileName = FileName + NameIndex;

	Utf8Length = WideCharToMultiByte(CP_UTF8, 0, DestFileName, wcslen(DestFileName), NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, DestFileName, wcslen(DestFileName), Utf8Name, Utf8Length, NULL, NULL);

	for (i = 0; i < Utf8Length; i++)
	{
		if (Utf8Name[i] == '\\')
			Utf8Name[i] = '/';
	}

	wprintf(L"Adding `%s'\n", FileName);

	if (!DirectoryFile)
	{
		hFile = CreateFileW(FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
			return FALSE;

		FileSize = GetFileSize(hFile, NULL);

		FileBuffer = malloc(FileSize);
		if (!FileBuffer)
		{
			CloseHandle(hFile);
			return FALSE;
		}

		if (!ReadFile(hFile, FileBuffer, FileSize, &BytesRead, NULL))
		{
			free(FileBuffer);
			CloseHandle(hFile);
			return FALSE;
		}

		Crc32 = ZipCrc32Message(FileBuffer, FileSize);
	}
	else
	{
		// Add backslash.
		if (Utf8Name[Utf8Length - 1] != '/' &&
			Utf8Name[Utf8Length - 1] != '\\')
			Utf8Name[Utf8Length++] = '/';
	}

	// Get File Offset.
	FileOffsetToLocalFileHeader = SetFilePointer(ZipContext->hZipFile, 0, NULL, FILE_CURRENT);

	// Add Local File Header.
	LocalFileHeader.Signature = 0x04034b50;
	LocalFileHeader.VersionRequired = 0x0200;
	LocalFileHeader.Flags = 1 << 11; // language encoding flag;
	LocalFileHeader.CompressionMethod = 0x0000;
	LocalFileHeader.LastModifiedFileTime = 0x0000;
	LocalFileHeader.LastModifiedFileDate = 0x0000;
	LocalFileHeader.Crc32 = Crc32;
	LocalFileHeader.CompressedSize = FileSize;
	LocalFileHeader.UncompressedSize = FileSize;
	LocalFileHeader.FileNameLength = Utf8Length;
	LocalFileHeader.ExtraFieldLength = 0;

	if (!WriteFile(ZipContext->hZipFile, &LocalFileHeader, sizeof(LocalFileHeader), &BytesWritten, NULL) || 
		!WriteFile(ZipContext->hZipFile, Utf8Name, LocalFileHeader.FileNameLength, &BytesWritten, NULL))
	{
		if (FileBuffer)
			free(FileBuffer);

		CloseHandle(hFile);
		return FALSE;
	}

	if (!DirectoryFile)
	{
		if (!WriteFile(ZipContext->hZipFile, FileBuffer, FileSize, &BytesWritten, NULL))
		{
			if (FileBuffer)
				free(FileBuffer);

			CloseHandle(hFile);
			return FALSE;
		}
	}

	CentralFileHeader.Signature = 0x02014b50;
	CentralFileHeader.VersionCreated = 0x0000;
	CentralFileHeader.VersionRequired = 0x0200;
	CentralFileHeader.Flags = 1 << 11; // language encoding flag;
	CentralFileHeader.CompressionMethod = 0x0000;
	CentralFileHeader.LastModifiedFileTime = 0x0000;
	CentralFileHeader.LastModifiedFileDate = 0x0000;
	CentralFileHeader.Crc32 = Crc32;
	CentralFileHeader.CompressedSize = FileSize;
	CentralFileHeader.UncompressedSize = FileSize;
	CentralFileHeader.FileNameLength = Utf8Length;
	CentralFileHeader.ExtraFieldLength = 0;
	CentralFileHeader.FileCommentLength = 0;
	CentralFileHeader.DiskNumberStart = 0;
	CentralFileHeader.InternalFileAttributes = 0;
	CentralFileHeader.ExternalFileAttributes = 0;
	CentralFileHeader.RelativeOffsetToLocalHeader = FileOffsetToLocalFileHeader;

	if (!WriteFile(ZipContext->hTempFile, &CentralFileHeader, sizeof(CentralFileHeader), &BytesWritten, NULL) ||
		!WriteFile(ZipContext->hTempFile, Utf8Name, CentralFileHeader.FileNameLength, &BytesWritten, NULL))
	{
		if (FileBuffer)
			free(FileBuffer);

		CloseHandle(hFile);
		return FALSE;
	}

	ZipContext->CentralDirectoryCount++;

	if (FileBuffer)
		free(FileBuffer);

	CloseHandle(hFile);

	return TRUE;
}

BOOLEAN
ZipAddFileRecord(
	IN PZIP_WRITE_CONTEXT ZipContext,
	IN CHAR16 *FileName)
{
	ULONG NameIndex = PathFindFileNameW(FileName) - FileName;

	return ZipAddFileRecordInternal(ZipContext, FileName, NameIndex);
}


BOOLEAN
ZipAddDirectoryInternal(
	IN PZIP_WRITE_CONTEXT ZipContext, 
	IN CHAR16 *Directory, 
	IN ULONG NameIndex)
{
	WIN32_FIND_DATAW FindData;
	HANDLE hFind;

	CHAR16 SearchPath[MAX_PATH];
	CHAR16 TempBuffer[MAX_PATH];
	CHAR16 FilePath[MAX_PATH];

	wcscpy(SearchPath, Directory);
	PathAppendW(SearchPath, L"*");

	hFind = FindFirstFileW(SearchPath, &FindData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		BOOL Found = TRUE;
		while (Found)
		{
			wcscpy(TempBuffer, Directory);
			PathAppendW(TempBuffer, FindData.cFileName);

			PathCanonicalizeW(FilePath, TempBuffer);

			if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (!wcscmp(FindData.cFileName, L".")) // must "."
				{
					// Add directory as a file.
					if (!ZipAddFileRecordInternal(ZipContext, FilePath, NameIndex))
					{
						FindClose(hFind);
						return FALSE;
					}
				}
				else if (wcscmp(FindData.cFileName, L"..")) // not ".."
				{
					// And files too...
					if (!ZipAddDirectoryInternal(ZipContext, FilePath, NameIndex))
					{
						FindClose(hFind);
						return FALSE;
					}
				}
			}
			else
			{
				if (!ZipAddFileRecordInternal(ZipContext, FilePath, NameIndex))
				{
					FindClose(hFind);
					return FALSE;
				}
			}

			Found = FindNextFileW(hFind, &FindData);
		}

		FindClose(hFind);
	}

	return TRUE;
}

BOOLEAN
ZipAddDirectory(
	IN PZIP_WRITE_CONTEXT ZipContext,
	IN CHAR16 *Directory)
{
	CHAR16 *FileName = PathFindFileNameW(Directory);
	ULONG NameIndex = FileName - Directory;

	return ZipAddDirectoryInternal(ZipContext, Directory, NameIndex);
}

BOOLEAN
ZipMiscMarkDeleteFile(
	IN HANDLE hZipFile)
{
	FILE_DISPOSITION_INFO DispositionInfo;

	DispositionInfo.DeleteFileW = TRUE;
	return !!SetFileInformationByHandle(hZipFile, FileDispositionInfo, &DispositionInfo, sizeof(DispositionInfo));
}

BOOLEAN
ZipWriteFile(
	IN CHAR16 *ZipName, 
	IN U32 Count, 
	IN CHAR16 **FileNames)
{
	U32 i;
	ZIP_WRITE_CONTEXT ZipContext;
	CHAR16 TempFileName[MAX_PATH];

	ZIP_END_OF_CENTRAL_DIRECTORY CentralDirectoryEnd;

	PVOID CentralDirectory;
	ULONG CentralDirectoryLength;
	ULONG Dummy;

	GetTempFileNameW(L".", L"~CENTRAL", 0, TempFileName);

	ZipContext.CentralDirectoryCount = 0;

	ZipContext.hZipFile = CreateFileW(ZipName, GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (ZipContext.hZipFile == INVALID_HANDLE_VALUE)
		return FALSE;

	ZipContext.hTempFile = CreateFileW(TempFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL);
	if (ZipContext.hTempFile == INVALID_HANDLE_VALUE)
	{
		ZipMiscMarkDeleteFile(ZipContext.hZipFile);
		CloseHandle(ZipContext.hZipFile);
		return FALSE;
	}

	for (i = 0; i < Count; i++)
	{
		BOOLEAN Succeed = FALSE;

		if (PathIsDirectoryW(FileNames[i]))
			Succeed = ZipAddDirectory(&ZipContext, FileNames[i]);
		else
			Succeed = ZipAddFileRecord(&ZipContext, FileNames[i]);

		if (!Succeed)
		{
			ZipMiscMarkDeleteFile(ZipContext.hZipFile);
			CloseHandle(ZipContext.hZipFile);
			CloseHandle(ZipContext.hTempFile);
			return FALSE;
		}
	}

	CentralDirectoryLength = GetFileSize(ZipContext.hTempFile, &Dummy);


	// End of Central Directory.
	CentralDirectoryEnd.AbsoluteOffsetToCentralDirectoryStart = SetFilePointer(ZipContext.hZipFile, 0, NULL, FILE_CURRENT);
	CentralDirectoryEnd.CentralDirectoryCount = (U16)ZipContext.CentralDirectoryCount;
	CentralDirectoryEnd.CentralDirectoryStartDiskNumber = 0;
	CentralDirectoryEnd.CommentLength = 0;
	CentralDirectoryEnd.DiskNumber = 0;
	CentralDirectoryEnd.Signature = 0x06054b50;
	CentralDirectoryEnd.SizeOfCentralDirectory = CentralDirectoryLength;
	CentralDirectoryEnd.TotalCentralDirectoryCount = (U16)ZipContext.CentralDirectoryCount;



	CentralDirectory = malloc(CentralDirectoryLength);
	if (!CentralDirectory)
	{
		ZipMiscMarkDeleteFile(ZipContext.hZipFile);
		CloseHandle(ZipContext.hZipFile);
		CloseHandle(ZipContext.hTempFile);
		return FALSE;
	}

	SetFilePointer(ZipContext.hTempFile, 0, NULL, FILE_BEGIN);

	if (!ReadFile(ZipContext.hTempFile, CentralDirectory, CentralDirectoryLength, &Dummy, NULL) ||
		!WriteFile(ZipContext.hZipFile, CentralDirectory, CentralDirectoryLength, &Dummy, NULL) ||
		!WriteFile(ZipContext.hZipFile, &CentralDirectoryEnd, sizeof(CentralDirectoryEnd), &Dummy, NULL))
	{
		free(CentralDirectory);
		ZipMiscMarkDeleteFile(ZipContext.hZipFile);
		CloseHandle(ZipContext.hZipFile);
		CloseHandle(ZipContext.hTempFile);
		return FALSE;
	}

	free(CentralDirectory);
	CloseHandle(ZipContext.hTempFile);
	CloseHandle(ZipContext.hZipFile);

	return TRUE;
}

int wmain(int argc, wchar_t **argv)
{
	// ZipImage [zipfile] [file1 [file2 file3 ...]]
#if 1
	if (1)
	{
		PVOID Buffer = NULL;
		ULONG FileSize = 0, Dummy = 0;
		HANDLE hFile = CreateFile(TEXT("test.zip"), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			printf("failed to open file\n");
			return -1;
		}

		FileSize = GetFileSize(hFile, NULL);
		Buffer = malloc(FileSize);

		if (!ReadFile(hFile, Buffer, FileSize, &Dummy, NULL))
		{
			printf("failed to read\n");
			return -1;
		}

		CloseHandle(hFile);

		if (1)
		{
			ZIP_CONTEXT Context;
			U32 Offset;
			VOID *Address;
			U32 Size;

			if (!ZipInitializeReaderContext(&Context, Buffer, FileSize))
			{
				printf("failed to initialize zip context\n");
				return -1;
			}

			if (ZipLookupFile_U8(&Context, u8"arch/X64/Intrin64.h", &Offset))
			{
				printf("file found!\n");
				if (ZipGetFileAddress(&Context, Offset, &Address, &Size))
				{
					printf("address = 0x%p, size = 0x%x\n", Address, Size);
				}
				else
				{
					printf("failed to get file address\n");
				}
			}
			else
			{
				printf("failed to lookup file\n");
			}

		}

		return -1;
	}
#endif

	if (argc >= 3)
	{
		wprintf(L"\nzipimage, the simple file archiver\n");
		if (!ZipWriteFile(argv[1], (U32)(argc - 2), &argv[2]))
		{
			wprintf(L"failed to create `%s'\n", argv[1]);
			return -2;
		}

		wprintf(L"`%s' created successfully.\n", argv[1]);
	}
	else
	{
		wprintf(L"usage: %s [zipfile] [file1 [file2 file3 ...]]\n", argv[0]);
		return -1;
	}

	wprintf("L\n");

	return 0;
}

