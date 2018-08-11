
#include <stdio.h>
#include <Windows.h>
#include <Shlwapi.h>

#include "BootImage.h"

#pragma comment(lib, "shlwapi.lib")

#define	DASSERT(_x)		\
	if(!(_x)) { __asm int 03h }

typedef struct _DUMP_IMAGE_CONTEXT {
	PWSTR FileName;
} DUMP_IMAGE_CONTEXT, *PDUMP_IMAGE_CONTEXT;

PTR
CALLCONV
AllocateMemory(
	IN U32 AllocationSize)
{
	return (PTR)malloc(AllocationSize);
}

VOID
CALLCONV
FreeMemory(
	IN PTR AllocatedMemory)
{
	free((PVOID)AllocatedMemory);
}

BOOLEAN
CALLCONV
DumpImage(
	IN PBIM_HEADER ImageBuffer,
	IN U32 SizeOfImage,
	IN PTR DumpImageContext)
{
	PDUMP_IMAGE_CONTEXT Context = (PDUMP_IMAGE_CONTEXT)DumpImageContext;
	HANDLE FileHandle = CreateFileW(Context->FileName, GENERIC_READ | GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, 0, NULL);
	DWORD BytesWritten = 0;

	printf("ImageSizeUsed %#x, ImageSizeTotal %#x\n", ImageBuffer->SizeUsed, ImageBuffer->SizeTotal);

	if (FileHandle == INVALID_HANDLE_VALUE)
		return FALSE;

	if (!WriteFile(FileHandle, ImageBuffer, SizeOfImage, &BytesWritten, NULL) ||
		BytesWritten != SizeOfImage)
	{
		CloseHandle(FileHandle);
		return FALSE;
	}

	FlushFileBuffers(FileHandle);
	CloseHandle(FileHandle);

	return TRUE;
}

PBIM_FILE_HANDLE
CALLCONV
BimCreateFile_U16(
	IN PBIM_CONTEXT Context,
	IN CHAR16 *FilePath,
	IN U32 FileFlags,
	IN U32 FileSize)
{
	CHAR FilePath_U8[0x400];
	INT WchLength;
	INT Utf8Length;

	WchLength = lstrlenW(FilePath);
	Utf8Length = WideCharToMultiByte(CP_UTF8, 0, FilePath, WchLength, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, FilePath, WchLength, FilePath_U8, Utf8Length, NULL, NULL);
	FilePath_U8[Utf8Length] = '\0';

	return BimCreateFile(Context, FilePath_U8, FileFlags, FileSize);
}

BOOLEAN
APIENTRY
BimWin32CopyFileToImage(
	IN PBIM_CONTEXT Context,
	IN CHAR16 *FilePath,
	IN CHAR16 *DestFilePath)
{
	ULONG FileSize, FileSizeHigh;

	HANDLE FileHandle;
	UCHAR Buffer[0x1000];
	ULONG BytesRead;

	PBIM_FILE_HANDLE Handle;

	FileHandle = CreateFileW(FilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (FileHandle == INVALID_HANDLE_VALUE)
	{
		printf("Error opening source file '%ws'\n", FilePath);
		return FALSE;
	}

	FileSize = GetFileSize(FileHandle, &FileSizeHigh);

	if (GetLastError() != ERROR_SUCCESS || FileSizeHigh > 0)
	{
		CloseHandle(FileHandle);
		return FALSE;
	}

	Handle = BimCreateFile_U16(Context, DestFilePath, BIM_FILE_FLAG_READONLY, FileSize);
	if (!Handle)
	{
		CloseHandle(FileHandle);
		return FALSE;
	}

	if (FileSize != 0)
	{
		ULONG CurrentBytesRead = 0;
//		DASSERT(MFS_FdeWrite(DevContext, &FdeNewFile, FdeFdt, FdeNewFile.IndexSelf));

		for (;;)
		{
			if (CurrentBytesRead >= FileSize)
				break;

			if (!ReadFile(FileHandle, Buffer, sizeof(Buffer), &BytesRead, NULL))
				DASSERT(FALSE);

			DASSERT(
				BimFileReadWrite(Context, Handle, CurrentBytesRead, (PTR)Buffer, BytesRead, FALSE, TRUE)
				== BytesRead
			);

			CurrentBytesRead += BytesRead;
//			TRACE("%I64d bytes of %I64d bytes copied\r", CurrentBytesRead, FileSize);
		}

//		TRACE("\n");
	}

	BimCloseFile(Context, Handle);

	CloseHandle(FileHandle);
	return TRUE;
}

BOOLEAN
APIENTRY
BimWin32CopyDirectoryToImage(
	IN PBIM_CONTEXT Context,
	IN CHAR16 *FilePath,
	IN CHAR16 *DestDirectory)
{
	HANDLE hFindFile;
	WIN32_FIND_DATAW FindData;
	BOOL FileFound;
	USHORT FileFilter[MAX_PATH];
	USHORT DestFilePath[MAX_PATH];
	USHORT SrcFilePath[MAX_PATH];
	PUSHORT FileNamePtr;

	PBIM_FILE_HANDLE Handle;

	if (PathIsDirectoryW(FilePath))
	{
		// Directory file.
		PathCombineW(FileFilter, FilePath, L"*");
		if (PathIsRootW(FilePath))
		{
			FileNamePtr = L"";
		}
		else
		{
			FileNamePtr = PathFindFileNameW(FilePath);
		}
		PathCombineW(DestFilePath, DestDirectory, FileNamePtr);

		//		TRACE("DestDirectory '%ws'\n", DestDirectory);
		//		TRACE("NamePtr '%ws'\n", FileNamePtr);
		//		TRACE("Filter '%ws'\n", FileFilter);

		printf("CopyToFs: Creating directory '%ws' ...\n", DestFilePath);
		Handle = BimCreateFile_U16(Context, DestFilePath, BIM_FILE_FLAG_DIRECTORY, 0);
		if (!Handle)
		{
			return FALSE;
		}

		hFindFile = FindFirstFileW(FileFilter, &FindData);

		if (hFindFile != INVALID_HANDLE_VALUE)
		{
			FileFound = TRUE;

			while (FileFound)
			{
				if (!wcscmp(FindData.cFileName, L".") || 
					!wcscmp(FindData.cFileName, L".."))
				{
					// Skip! (".", "..")
					// TRACE("%ws 0x%p <SKIP>\n", FindData.cFileName, FindData.dwFileAttributes);
				}
				else
				{
					// TRACE("%ws 0x%p %s\n", FindData.cFileName, FindData.dwFileAttributes, 
					//	(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "<DIR>" : "");

					PathCombineW(SrcFilePath, FilePath, FindData.cFileName);

					if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						// Directory file.
						PathCombineW(DestFilePath, DestDirectory, FileNamePtr);
						printf("CopyToFs: Copying '%ws' to '%ws'\n", SrcFilePath, DestFilePath);

						if (!BimWin32CopyDirectoryToImage(Context, SrcFilePath, DestFilePath))
						{
							printf("Break on error! '%ws'\n", DestFilePath);
		//					break;
						}
					}
					else
					{
						// Non-directory file.
						PathCombineW(DestFilePath, DestDirectory, FileNamePtr);
						PathAppendW(DestFilePath, FindData.cFileName);
						printf("Copying '%ws' to: '%ws'\n", SrcFilePath, DestFilePath);

						if (!BimWin32CopyFileToImage(Context, SrcFilePath, DestFilePath))
						{
							printf("Break on error! '%ws'\n", DestFilePath);
		//					break;
						}
					}
				}

				FileFound = FindNextFileW(hFindFile, &FindData);
			}

			FindClose(hFindFile);
		}

		return TRUE;
	}

	return FALSE;
}

int wmain(int argc, wchar_t **argv, wchar_t **envp)
{
	BIM_CONTEXT Context;
	DUMP_IMAGE_CONTEXT DumpImageContext;

	DumpImageContext.FileName = L"dump.bin";

	BimInitializeContext(&Context, AllocateMemory, FreeMemory, DumpImage, (PTR)&DumpImageContext);
	BimCreateImage(&Context, NULL, 0x400000);

	BimWin32CopyDirectoryToImage(&Context, L".", L"");

	BimDumpImage(&Context);

	BimFreeImage(&Context);
	BimDestroyContext(&Context);


	return 0;
}

