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


SIZE_T
KERNELAPI
ClStrFormatU8V(
	OUT CHAR8 *Buffer,
	IN SIZE_T BufferLength,
	IN CHAR8 *Format,
	IN va_list VarList);

SIZE_T
VARCALL
ClStrFormatU8(
	OUT CHAR8 *Buffer,
	IN SIZE_T BufferLength,
	IN CHAR8 *Format,
	...);

SIZE_T
KERNELAPI
ClStrTerminateU8(
	IN OUT CHAR8 *Buffer,
	IN SIZE_T BufferLength,
	IN SIZE_T Position);

SIZE_T
KERNELAPI
ClStrCopyU8(
    OUT CHAR8 *Buffer,
    IN SIZE_T BufferLength,
    IN CHAR8 *SourceBuffer);
