#pragma once

#define	_FORMAT_FLAG_LEFTJUSTIFY			0x0001
#define	_FORMAT_FLAG_PRINT_SIGN				0x0002
#define	_FORMAT_FLAG_SPACE					0x0004
#define	_FORMAT_FLAG_PRINT_PREFIX			0x0008
#define	_FORMAT_FLAG_ZERO					0x0010

#define	_FORMAT_FLAG_UPCASE					0x0020

#define	_FORMAT_FLAG_LENGTH_LONG			0x0040 // long (or long long)
#define	_FORMAT_FLAG_LENGTH_SHORT			0x0080 // short
#define	_FORMAT_FLAG_LENGTH_SHORT2			0x0100 // char

#define	_FORMAT_FLAG_TREAT_UNSIGNED			0x0200 // treat integer as unsigned
#define	_FORMAT_FLAG_PRECISION				0x0400


UINTN
StrFormatV(
	OUT CHAR16 *Buffer,
	IN UINTN BufferLength,
	IN CHAR16 *Format,
	IN VA_LIST VarList);

UINTN
StrFormat(
	OUT CHAR16 *Buffer,
	IN UINTN BufferLength,
	IN CHAR16 *Format,
	...);

UINTN
StrTerminate(
	IN OUT CHAR16 *Buffer,
	IN UINTN BufferLength,
	IN UINTN Position);

UINTN
StrCopy(
	OUT CHAR16 *Buffer,
	IN UINTN BufferLength,
	IN CHAR16 *SourceBuffer);

