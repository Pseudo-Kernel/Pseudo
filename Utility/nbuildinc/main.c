
#include <stdio.h>
#include <Windows.h>

int increment_build_number(const wchar_t *path, const wchar_t *path2, const wchar_t *name)
{
	HANDLE hFile, hFile2;
	CHAR buffer[64] = { 0 };
	CHAR new_buf[256];
	ULONG build_number = 0;
	ULONG BytesRead, BytesWritten;
	int nret = -1;

	hFile = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
		return -1;

	hFile2 = CreateFileW(path2, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if(hFile2 == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
		return -1;
	}

	do
	{
		memset(buffer, 0, sizeof(buffer));
		if(!ReadFile(hFile, buffer, sizeof(buffer) - 1, &BytesRead, NULL))
		{
			printf(" --- Cannot read file!\n");
			break;
		}

		build_number = atoi(buffer);

		printf(" *** Previous BuildNumber = %d, Current BuildNumber = %d\n", 
			build_number, build_number + 1);

		if(SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		{
			printf(" --- SetFilePointer failed\n");
			break;
		}

		itoa(build_number + 1, buffer, 10);
		if(!WriteFile(hFile, buffer, strlen(buffer), &BytesWritten, NULL))
		{
			printf(" --- Cannot write to a file!\n");
			break;
		}

		sprintf(new_buf, "#define %ws\t\t%u\n", name, build_number + 1);
		if(!WriteFile(hFile2, new_buf, strlen(new_buf), &BytesWritten, NULL))
		{
			printf(" --- Cannot write to header file!\n");
			break;
		}

		nret = 0;
	}
	while(FALSE);

	CloseHandle(hFile);
	return nret;
}

int wmain(int argc, const wchar_t **argv)
{
	int nret = -1;
	printf("\n");

	if(argc < 4)
		printf(" --- Argument is missing\n    <build_number_file> <header_file> <define_name>\n");
	else
		nret = increment_build_number(argv[1], argv[2], argv[3]);

	if(nret != 0)
		printf(" --- fatal error.\n");

	printf("\n");
	return nret;
}
