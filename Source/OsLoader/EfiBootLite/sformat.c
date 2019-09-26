
#include <Base.h>
#include "SFormat.h"

UINTN
StrLengthAscii(
	IN CHAR8 *Buffer,
	IN UINTN BufferLength)
{
	UINTN Length = 0;

	while (Buffer[Length] && Length < BufferLength)
		Length++;

	return Length;
}

UINTN
StrLength(
	IN CHAR16 *Buffer,
	IN UINTN BufferLength)
{
	UINTN Length = 0;

	while (Buffer[Length] && Length < BufferLength)
		Length++;

	return Length;
}

UINTN
FormatHelper_InternalStrFill(
	IN OUT CHAR16 *Buffer,
	IN UINTN BufferLength,
	IN OUT UINTN *FillTo,
	IN CHAR16 FillChar,
	IN UINTN FillLength)
{
	UINTN Index = *FillTo;
	UINTN Count = 0;

	while (Index < BufferLength && FillLength-- > 0)
	{
		Buffer[Index++] = FillChar;
		Count++;
	}

	*FillTo = Index;

	return Count;
}

UINTN
FormatHelper_InternalStrCopyAsciiToU16(
	IN OUT CHAR16 *Buffer,
	IN UINTN BufferLength,
	IN OUT UINTN *CopyTo,
	IN CHAR8 *SourceBuffer,
	IN UINTN SourceBufferLength)
{
	UINTN Index = *CopyTo;
	UINTN Count = 0;

	while (Index < BufferLength &&
		SourceBuffer[Count] &&
		SourceBufferLength-- > 0)
	{
		Buffer[Index++] = (CHAR16)(UINT8)SourceBuffer[Count++];
	}

	*CopyTo = Index;

	return Count;
}

UINTN
FormatHelper_InternalStrCopy(
	IN OUT CHAR16 *Buffer,
	IN UINTN BufferLength,
	IN OUT UINTN *CopyTo,
	IN CHAR16 *SourceBuffer,
	IN UINTN SourceBufferLength)
{
	UINTN Index = *CopyTo;
	UINTN Count = 0;

	while (Index < BufferLength &&
		SourceBuffer[Count] &&
		SourceBufferLength-- > 0)
	{
		Buffer[Index++] = SourceBuffer[Count++];
	}

	*CopyTo = Index;

	return Count;
}

UINTN
FormatHelper_StrCopyAsciiToU16(
	OUT CHAR16 *Buffer,
	IN UINTN BufferLength,
	IN UINTN MinimumWidth,
	IN UINTN PrecisionWidth,
	IN CHAR8 *SourceBuffer,
	IN UINTN Flags)
{
	UINTN SpaceCount = 0;
	UINTN SourceBufferLength = 0;
	UINTN OutputLength = 0;

	if (Flags & _FORMAT_FLAG_PRECISION)
		SourceBufferLength = StrLengthAscii(SourceBuffer, PrecisionWidth);
	else
		SourceBufferLength = StrLengthAscii(SourceBuffer, -1);

	if (MinimumWidth > SourceBufferLength)
		SpaceCount = MinimumWidth - SourceBufferLength;

	if (SourceBufferLength > MinimumWidth)
		MinimumWidth = SourceBufferLength;

	if (!(Flags & _FORMAT_FLAG_LEFTJUSTIFY))
		FormatHelper_InternalStrFill(Buffer, BufferLength, &OutputLength, ' ', SpaceCount);

	FormatHelper_InternalStrCopyAsciiToU16(Buffer, BufferLength, &OutputLength, SourceBuffer, SourceBufferLength);

	if (Flags & _FORMAT_FLAG_LEFTJUSTIFY)
		FormatHelper_InternalStrFill(Buffer, BufferLength, &OutputLength, ' ', SpaceCount);

	return OutputLength;
}

UINTN
FormatHelper_StrCopy(
	OUT CHAR16 *Buffer,
	IN UINTN BufferLength,
	IN UINTN MinimumWidth,
	IN UINTN PrecisionWidth,
	IN CHAR16 *SourceBuffer,
	IN UINTN Flags)
{
	UINTN SpaceCount = 0;
	UINTN SourceBufferLength = 0;
	UINTN OutputLength = 0;

	if (Flags & _FORMAT_FLAG_PRECISION)
		SourceBufferLength = StrLength(SourceBuffer, PrecisionWidth);
	else
		SourceBufferLength = StrLength(SourceBuffer, -1);

	if (MinimumWidth > SourceBufferLength)
		SpaceCount = MinimumWidth - SourceBufferLength;

	if (SourceBufferLength > MinimumWidth)
		MinimumWidth = SourceBufferLength;

	if (!(Flags & _FORMAT_FLAG_LEFTJUSTIFY))
		FormatHelper_InternalStrFill(Buffer, BufferLength, &OutputLength, ' ', SpaceCount);

	FormatHelper_InternalStrCopy(Buffer, BufferLength, &OutputLength, SourceBuffer, SourceBufferLength);

	if (Flags & _FORMAT_FLAG_LEFTJUSTIFY)
		FormatHelper_InternalStrFill(Buffer, BufferLength, &OutputLength, ' ', SpaceCount);

	return OutputLength;
}

UINTN
FormatHelper_ValueToString(
	OUT CHAR16 *Buffer,
	IN UINTN BufferLength,
	IN UINTN MinimumWidth,
	IN UINTN MinimumPrecisionWidth,
	IN INT64 Value,
	IN UINTN Radix,
	IN UINTN Flags)
{
	static CHAR16 DigitTable[] = L"0123456789abcdefghijklmnopqrstuvwxyz";

	CHAR16 ValueString[64 + 1];     // Value + Null
	CHAR16 PrefixBuffer[2 + 1 + 1]; // Prefix (0x-like) + Sign + Null
	CHAR16 TempBuffer[64 + 1];      // Value + Null

	UINTN OutputLength = 0;
	UINTN ValueLength = 0;
	UINTN PrefixLength = 0;
	UINTN TempLength = 0;
	CHAR16 CaseBit = 0;
	INTN Sgn = 0;

	UINT64 Unsigned64;
	UINTN WriteCount;
	UINTN i;

	// Cannot write anything
	if (!BufferLength)
		return 0;

	if (Radix < 2 || Radix > ARRAY_SIZE(DigitTable))
		return 0;

	if (Flags & _FORMAT_FLAG_TREAT_UNSIGNED)
	{
		Unsigned64 = Value;
		Sgn = 1;
	}
	else
	{
		Unsigned64 = Value >= 0 ? Value : -Value;
		if(Value)
			Sgn = Value > 0 ? 1 : -1;
	}

	if (Flags & _FORMAT_FLAG_UPCASE)
		CaseBit = 'a' - 'A';

	do
	{
		ValueString[ValueLength++] = DigitTable[Unsigned64 % Radix];
		Unsigned64 /= Radix;
	} while (Unsigned64 != 0);

	if ((Flags & _FORMAT_FLAG_PRINT_SIGN) || Sgn < 0)
	{
		CHAR16 SignChar[2] = { '+', 0 };
		switch (Sgn)
		{
		case 1: SignChar[0] = '+'; break;
		case 0: break;
		case -1: SignChar[0] = '-'; break;
		default: break;
		}

		FormatHelper_InternalStrCopy(PrefixBuffer, ARRAY_SIZE(PrefixBuffer), &PrefixLength, SignChar, ARRAY_SIZE(SignChar));
	}

	if (Flags & _FORMAT_FLAG_PRINT_PREFIX)
	{
		CHAR16 *Prefix = L"";
		UINTN Length = 0;

		switch (Radix)
		{
		case 2: Prefix = L"0b"; Length = 2; break;
		case 8: Prefix = L"0";  Length = 1; break;
			// case 10: Prefix = L"0n"; Length = 2; break;
		case 16: Prefix = L"0x"; Length = 2; break;
		default: Prefix = L""; Length = 0;
		}

		FormatHelper_InternalStrCopy(PrefixBuffer, ARRAY_SIZE(PrefixBuffer), &PrefixLength, Prefix, Length);
	}

	for (i = 0; i < ValueLength; i++)
	{
		CHAR16 ch = ValueString[ValueLength - i - 1];

		if ('a' <= ch && ch <= 'z')
			ch &= ~CaseBit;

		TempBuffer[TempLength++] = ch;
	}

	//
	//   <1>   <2>     <3>   
	// [SPACE][ZERO][INTEGER]
	//
	// IF LEN(OutStr) > Width          THEN: Width          = LEN(OutStr)
	// IF LEN(OutStr) > PrecisionWidth THEN: PrecisionWidth = LEN(OutStr)
	// IF Width < PrecisionWidth       THEN: Width          = PrecisionWidth;
	//
	// IF NOT LeftJustification        THEN: SPACE(Width - PrecisionWidth);
	// ZERO(PrecisionWidth - LEN(OutStr));
	// PRINT(OutStr);
	// IF LeftJustification            THEN: SPACE(Width - PrecisionWidth);
	//

	if (TempLength + PrefixLength > MinimumWidth)
		MinimumWidth = TempLength + PrefixLength;

	if (TempLength + PrefixLength > MinimumPrecisionWidth)
		MinimumPrecisionWidth = TempLength + PrefixLength;

	if (!(Flags & _FORMAT_FLAG_PRECISION))
		MinimumPrecisionWidth = MinimumWidth;

	if (MinimumWidth < MinimumPrecisionWidth)
		MinimumWidth = MinimumPrecisionWidth;


	if (!(Flags & _FORMAT_FLAG_LEFTJUSTIFY))
		FormatHelper_InternalStrFill(Buffer, BufferLength, &OutputLength, ' ', MinimumWidth - MinimumPrecisionWidth);


	WriteCount = ((Flags & _FORMAT_FLAG_PRECISION) ?
		MinimumPrecisionWidth : MinimumWidth) - (TempLength + PrefixLength);
	if (Flags & _FORMAT_FLAG_ZERO)
	{
		// [Prefix][Zero][Number]
		//       0x  0..0   12345
		FormatHelper_InternalStrCopy(Buffer, BufferLength, &OutputLength, PrefixBuffer, PrefixLength);
		FormatHelper_InternalStrFill(Buffer, BufferLength, &OutputLength, '0', WriteCount);
	}
	else
	{
		// [Space][Prefix][Number]
		//              0x   12345
		FormatHelper_InternalStrFill(Buffer, BufferLength, &OutputLength, ' ', WriteCount);
		FormatHelper_InternalStrCopy(Buffer, BufferLength, &OutputLength, PrefixBuffer, PrefixLength);
	}

	FormatHelper_InternalStrCopy(Buffer, BufferLength, &OutputLength, TempBuffer, TempLength);


	if (Flags & _FORMAT_FLAG_LEFTJUSTIFY)
		FormatHelper_InternalStrFill(Buffer, BufferLength, &OutputLength, ' ', MinimumWidth - MinimumPrecisionWidth);


	// ASSERT(OutputLength =< BufferLength);

	// Do not terminate with null

	return OutputLength;
}

UINTN
StrFormatV(
	OUT CHAR16 *Buffer,
	IN UINTN BufferLength,
	IN CHAR16 *Format,
	IN VA_LIST VarList)
{
	//
	// %[flags][width][.pricision][length]specifier
	// flags: '-' left-justify
	//        '+' forces sign
	//        ' ' ????
	//        '#' adds prefix 0x for hexadecimal format.
	//        '0' fills zero at left
	// width: (number) minimum number of characters to be printed.
	//        '*'      number specified by argument.
	// pricision: see details.
	//            http://www.cplusplus.com/reference/cstdio/printf/
	// length: 'l'  long. ex) %lx -> long
	//         'll' long long. ex) %llx -> long long
	//         'h'  short. ex) %hx -> short
	// prefix: 'd', 'i'      int.
	//         'u', 'x', 'X' unsigned int.
	//         'c'           char.
	//         's'           char *.
	//         'p'           void *.
	//

	#define	WRITE_CHAR(_ch)	\
		do										\
		{										\
			if (OutputLength >= BufferLength)	\
			{									\
				goto _Return;					\
			}									\
												\
			*d++ = (_ch);						\
			OutputLength++;						\
		} while (0)

	UINTN OutputLength = 0;
	UINTN InputLength = 0;

	CHAR16 *s = Format;
	CHAR16 *d = Buffer;

	while (*s)
	{
		UINTN Flags = 0;
		BOOLEAN ParseEnd = FALSE;

		UINTN Width = 0;
		UINTN PrecisionWidth = 0;
		UINTN VarSize = 0;
		UINTN i;

		BOOLEAN Integer = TRUE;
		UINTN Radix = 0;
		INT64 Value = 0;

		UINTN RemainingLength = 0;

		static struct {
			CHAR16 *Prefix;
			UINTN Flag;
		} LengthTable[] = {
			{ L"ll", _FORMAT_FLAG_LENGTH_LONG, },
			{ L"hh", _FORMAT_FLAG_LENGTH_SHORT2, },
			{ L"l", _FORMAT_FLAG_LENGTH_LONG, },
			{ L"h", _FORMAT_FLAG_LENGTH_SHORT, },
		};

		if (OutputLength >= BufferLength)
			break;

		if (*s != '%')
		{
			// Write character
			*d++ = *s++;
			OutputLength++;
			continue;
		}

		// [flag]
		while (!ParseEnd)
		{
			switch (*++s)
			{
			case '-': Flags |= _FORMAT_FLAG_LEFTJUSTIFY; break;
			case '+': Flags |= _FORMAT_FLAG_PRINT_SIGN; break;
			case ' ': Flags |= _FORMAT_FLAG_SPACE; break; // ???
			case '#': Flags |= _FORMAT_FLAG_PRINT_PREFIX; break;
			default:
				ParseEnd = TRUE;
			}
		}

		// [width]
		if (*s == '*')
		{
			Width = VA_ARG(VarList, UINTN);
			s++;
		}
		else
		{
			Width = 0;

			if (*s == '0')
				Flags |= _FORMAT_FLAG_ZERO;

			while ('0' <= *s && *s <= '9')
				Width = (Width * 10) + (*s++ - '0');
		}

		// [.precision]
		if (*s == '.')
		{
			s++;

			Flags |= (_FORMAT_FLAG_PRECISION | _FORMAT_FLAG_ZERO);

			if (*s == '*')
			{
				PrecisionWidth = VA_ARG(VarList, UINTN);
				s++;
			}
			else
			{
				PrecisionWidth = 0;

				while ('0' <= *s && *s <= '9')
					PrecisionWidth = (PrecisionWidth * 10) + (*s++ - '0');
			}
		}

		// [length]
		for (i = 0; i < ARRAY_SIZE(LengthTable); i++)
		{
			CHAR16 *p1 = s;
			CHAR16 *p2 = LengthTable[i].Prefix;
			BOOLEAN Matched = TRUE;

			while (*p2)
			{
				if (*p1++ != *p2++)
				{
					Matched = FALSE;
					break;
				}
			}

			if (Matched)
			{
				Flags |= LengthTable[i].Flag;
				s = p1;
				break;
			}
		}

		// specifier
		switch (*s)
		{
			// prefix: 'd', 'i'      int.
			//         'u', 'x', 'X' unsigned int.
			//         'c'           char.
			//         's'           char *.
			//         'p'           void *.
		case 'u':
			Flags |= _FORMAT_FLAG_TREAT_UNSIGNED;
			Radix = 10;
			break;

		case 'd':
		case 'i':
			Radix = 10;
			break;

		case 'X':
			Flags |= _FORMAT_FLAG_UPCASE;
		case 'x':
			Flags |= _FORMAT_FLAG_TREAT_UNSIGNED;
			Radix = 16;
			break;

		case 'p':
			if (sizeof(void *) == sizeof(long long) /*sizeof(__int64)*/)
				Flags |= _FORMAT_FLAG_LENGTH_LONG;

			Flags |= (_FORMAT_FLAG_TREAT_UNSIGNED | _FORMAT_FLAG_ZERO | _FORMAT_FLAG_PRECISION | _FORMAT_FLAG_UPCASE);
			Flags &= ~(_FORMAT_FLAG_PRINT_SIGN | _FORMAT_FLAG_PRINT_PREFIX);

			Radix = 16;
			PrecisionWidth = sizeof(void *) << 1;
			break;

		case 'c':
			if (Flags & _FORMAT_FLAG_LENGTH_LONG) // wide
				Value = VA_ARG(VarList, CHAR16) & 0xffff;
			else
				Value = VA_ARG(VarList, char/*__int8*/) & 0xff;

			WRITE_CHAR((CHAR16)Value);

			Integer = FALSE;
			break;

		case 's':
			if (1)
			{
				VOID *String = VA_ARG(VarList, VOID *);
				UINTN StringLength = 0;

				if (BufferLength > OutputLength)
					RemainingLength = BufferLength - OutputLength;

				if (Flags & _FORMAT_FLAG_LENGTH_LONG)
					StringLength = FormatHelper_StrCopy(d, RemainingLength, Width, PrecisionWidth, (CHAR16 *)String, Flags);
				else
					StringLength = FormatHelper_StrCopyAsciiToU16(d, RemainingLength, Width, PrecisionWidth, (CHAR8 *)String, Flags);

				OutputLength += StringLength;
				d += StringLength;
			}

			Integer = FALSE;
			break;

		default:
			// Invalid specifier
			continue;
			break;
		}

		s++;

		if (Integer)
		{
			UINTN NumberWidth = 0;

			if (Flags & _FORMAT_FLAG_LENGTH_LONG) // long or long long
				Value = VA_ARG(VarList, INT64);
			else if (Flags & _FORMAT_FLAG_LENGTH_SHORT) // short
				Value = (UINT16)VA_ARG(VarList, INT16);
			else if (Flags & _FORMAT_FLAG_LENGTH_SHORT2) // char
				Value = (UINT8)VA_ARG(VarList, INT8);
			else
				Value = (UINT32)VA_ARG(VarList, INT32);

			if (BufferLength > OutputLength)
				RemainingLength = BufferLength - OutputLength;

			NumberWidth = FormatHelper_ValueToString(d, RemainingLength, Width, PrecisionWidth, Value, Radix, Flags);

			OutputLength += NumberWidth;
			d += NumberWidth;
		}
	}

	#undef WRITE_CHAR

_Return:

	return OutputLength;
}

UINTN
StrFormat(
	OUT CHAR16 *Buffer,
	IN UINTN BufferLength,
	IN CHAR16 *Format,
	...)
{
	VA_LIST List;
	UINTN Result;

	VA_START(List, Format);
	Result = StrFormatV(Buffer, BufferLength, Format, List);
	VA_END(List);

	return Result;
}

UINTN
StrTerminate(
	IN OUT CHAR16 *Buffer,
	IN UINTN BufferLength,
	IN UINTN Position)
{
	if (!BufferLength || Position > BufferLength)
		return 0;

	if (Position == BufferLength)
		Position--;

	Buffer[Position] = 0;

	return 1;
}

UINTN
StrCopy(
	OUT CHAR16 *Buffer,
	IN UINTN BufferLength,
	IN CHAR16 *SourceBuffer)
{
	UINTN Count = 0;

	while (SourceBuffer[Count] && Count < BufferLength)
	{
		Buffer[Count] = SourceBuffer[Count];
		Count++;
	}

	StrTerminate(Buffer, BufferLength, Count);

	return Count;
}
