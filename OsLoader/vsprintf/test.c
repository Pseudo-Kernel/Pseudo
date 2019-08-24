
#include <stdio.h>
#include <windows.h>

#include "UefiTypes.h"
#include "SFormatU8.h"

INT
StrFill(
	OUT USHORT *Buffer,
	IN UINT BufferLength,
	IN UINT FillLength,
	IN USHORT FillChar)
{
	UINT Count = 0;

	while (Count < BufferLength && FillLength-- > 0)
		Buffer[Count++] = FillChar;

	return Count;
}

#if 0
INT
SPrint(
	OUT USHORT *Buffer,
	IN UINT BufferLength,
	IN USHORT *Format,
	...)
{
	va_list VarList;
	INT Return;

	va_start(VarList, Format);
	Return = StrFormatV(Buffer, BufferLength, Format, VarList);
	va_end(VarList);

	if (StrTerminate(Buffer, BufferLength, Return))
		wprintf(L"%ws\n", Buffer);
	else
		wprintf(L"\n");

	return Return;
}
#else
INT
SPrint(
	OUT CHAR8 *Buffer,
	IN UINT BufferLength,
	IN CHAR8 *Format,
	...)
{
	va_list VarList;
	INT Return;

	va_start(VarList, Format);
	Return = ClStrFormatU8V(Buffer, BufferLength, Format, VarList);
	va_end(VarList);

	if (ClStrTerminateU8(Buffer, BufferLength, Return))
		printf("%s\n", Buffer);
	else
		printf("\n");

	return Return;
}
#endif

int main()
{
	char buf[11];

	if (sizeof(wchar_t) != 2)
		__debugbreak();

#define	__TOSTR(_x)				#_x
#define	_TOSTR(_x)				__TOSTR(_x)
#define	_DUMP(_x)				{ printf("%s -> ", _TOSTR(_x)); _x; };

#if 0
	_DUMP(printf("[%7.5d]\n", 123));
	_DUMP(printf("[%7.4d]\n", 123));
	_DUMP(printf("[%7.3d]\n", 123));
	_DUMP(printf("[%7.2d]\n", 123));
	_DUMP(printf("[%6.5d]\n", 123));
	_DUMP(printf("[%5.5d]\n", 123));
	_DUMP(printf("[%4.5d]\n", 123));
	_DUMP(printf("[%3.5d]\n", 123));
	_DUMP(printf("[%2.5d]\n", 123));
	_DUMP(printf("[%7.5d]\n", 123456));
	_DUMP(printf("[%7.5d]\n", 1234567));
	_DUMP(printf("[%7.5d]\n", 12345678));
#endif

//	SPrint(buf, _countof(buf), L"test! [%7.5d] [%12.6s] [%12.6ls]", 123, "12345678", L"65432");
//	printf("test! [%7.5d] [%12.6s] [%12.6ls]\n", 123, "12345678", L"65432");

//	SPrint(buf, _countof(buf), L"[0x%08X] [%#llx] [%#08x] [%#24p]\n", 0xdead, 0x123456789abcdef0ULL, 65535, (void *)2147483648);
//	printf("[0x%08X] [%#llx] [%#08x] [%#24p]\n", 0xdead, 0x123456789abcdef0ULL, 65535, (void *)2147483648);

	SPrint(buf, _countof(buf), "%lc %s %#hhx1234567890\n", L'¤±', "123", 0x80de);
	printf("%#hhx\n", 0x80de);

//	printf("    [%9.7d]\n", 0x123);
	return 0;
}

