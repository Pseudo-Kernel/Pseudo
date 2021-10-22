

#include <stdio.h>
#include <windows.h>


#define	TIME		PrintLocalTime();
#define	LOGT		TIME printf
#define	LOG     	printf

VOID PrintLocalTime()
{
	SYSTEMTIME t;
	GetLocalTime(&t);

	printf("[%04d-%02d-%02d %02d:%02d:%02d] ", 
		t.wYear, t.wMonth, t.wDay, 
		t.wHour, t.wMinute, t.wSecond);
}

VOID SetTextAttribute(UCHAR Attr)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (USHORT)Attr);
}

int main(int argc, char **argv)
{
	HANDLE hPipe;
	CHAR Buffer[512];
	DWORD BytesRead;

	/*
	hPipe=CreateNamedPipe(ComPipeName, 
		PIPE_ACCESS_DUPLEX, 
		0, 
		PIPE_UNLIMITED_INSTANCES, 
		0x1000, 
		0x1000, 
		-1, 
		NULL);
	*/

	SetTextAttribute(0x0f);
	LOGT("Initializing pipe client...\n\n");
	SetTextAttribute(0x07);

	if (argc < 2)
	{
		LOG("Argument is missing!\n");
		LOG("Example: %s \"\\\\.\\PIPE\\COM_1\"\n\n", argv[0]);
		return -1;
	}

	for (;;)
	{
		SetTextAttribute(0x0a);
        LOGT("Waiting %s ...\n", argv[1]);
		do { Sleep(1); } while(!WaitNamedPipeA(argv[1], -1));

		SetTextAttribute(0x0e);
        LOGT("Connected!\n");

		hPipe = CreateFileA(argv[1], GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
			OPEN_EXISTING, 0, NULL);

		if (hPipe == INVALID_HANDLE_VALUE)
		{
			SetTextAttribute(0x0c);
			LOG("Failed to open the pipe!\n");
			continue;
		}

		SetTextAttribute(0x07);
		while (ReadFile(hPipe, Buffer, sizeof(Buffer) - 1, &BytesRead, NULL))
		{
			Buffer[BytesRead] = 0;
			LOG("%s", Buffer);
		}

		SetTextAttribute(0x0d);
		LOGT("Disconnected ***\n\n");
		CloseHandle(hPipe);
	}

    return 0;
}

