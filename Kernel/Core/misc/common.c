
#include <Base.h>

SIZE_T
ClStrLengthU8(
	IN CHAR8 *Buffer,
	IN SIZE_T BufferLength)
{
	SIZE_T Length = 0;

	while (Buffer[Length] && Length < BufferLength)
		Length++;

	return Length;
}

SIZE_T
ClStrLengthU16(
	IN CHAR16 *Buffer,
	IN SIZE_T BufferLength)
{
	SIZE_T Length = 0;

	while (Buffer[Length] && Length < BufferLength)
		Length++;

	return Length;
}

SIZE_T
FormatHelper_InternalStrFillU8(
	IN OUT CHAR8 *Buffer,
	IN SIZE_T BufferLength,
	IN OUT SIZE_T *FillTo,
	IN CHAR8 FillChar,
	IN SIZE_T FillLength)
{
	SIZE_T Index = *FillTo;
	SIZE_T Count = 0;

	while (Index < BufferLength && FillLength-- > 0)
	{
		Buffer[Index++] = FillChar;
		Count++;
	}

	*FillTo = Index;

	return Count;
}


SIZE_T
FormatHelper_InternalStrCopyU16ToU8(
	IN OUT CHAR8 *Buffer,
	IN SIZE_T BufferLength,
	IN OUT SIZE_T *CopyTo,
	IN CHAR16 *SourceBuffer,
	IN SIZE_T SourceBufferLength)
{
	SIZE_T Index = *CopyTo;
	SIZE_T Count = 0;

	while (Index < BufferLength &&
		SourceBuffer[Count] &&
		SourceBufferLength-- > 0)
	{
		Buffer[Index++] = (CHAR8)SourceBuffer[Count++];
	}

	*CopyTo = Index;

	return Count;
}

SIZE_T
FormatHelper_InternalStrCopyU8(
	IN OUT CHAR8 *Buffer,
	IN SIZE_T BufferLength,
	IN OUT SIZE_T *CopyTo,
	IN CHAR8 *SourceBuffer,
	IN SIZE_T SourceBufferLength)
{
	SIZE_T Index = *CopyTo;
	SIZE_T Count = 0;

	while (Index < BufferLength &&
		SourceBuffer[Count] &&
		SourceBufferLength-- > 0)
	{
		Buffer[Index++] = (CHAR8)SourceBuffer[Count++];
	}

	*CopyTo = Index;

	return Count;
}


SIZE_T
FormatHelper_StrCopyU16ToU8(
	OUT CHAR8 *Buffer,
	IN SIZE_T BufferLength,
	IN SIZE_T MinimumWidth,
	IN SIZE_T PrecisionWidth,
	IN CHAR16 *SourceBuffer,
	IN SIZE_T Flags)
{
	SIZE_T SpaceCount = 0;
	SIZE_T SourceBufferLength = 0;
	SIZE_T OutputLength = 0;

	if (Flags & _FORMAT_FLAG_PRECISION)
		SourceBufferLength = ClStrLengthU16(SourceBuffer, PrecisionWidth);
	else
		SourceBufferLength = ClStrLengthU16(SourceBuffer, -1);

	if (MinimumWidth > SourceBufferLength)
		SpaceCount = MinimumWidth - SourceBufferLength;

	if (SourceBufferLength > MinimumWidth)
		MinimumWidth = SourceBufferLength;

	if (!(Flags & _FORMAT_FLAG_LEFTJUSTIFY))
		FormatHelper_InternalStrFillU8(Buffer, BufferLength, &OutputLength, ' ', SpaceCount);

	FormatHelper_InternalStrCopyU16ToU8(Buffer, BufferLength, &OutputLength, SourceBuffer, SourceBufferLength);

	if (Flags & _FORMAT_FLAG_LEFTJUSTIFY)
		FormatHelper_InternalStrFillU8(Buffer, BufferLength, &OutputLength, ' ', SpaceCount);

	return OutputLength;
}

SIZE_T
FormatHelper_StrCopyU8(
	OUT CHAR8 *Buffer,
	IN SIZE_T BufferLength,
	IN SIZE_T MinimumWidth,
	IN SIZE_T PrecisionWidth,
	IN CHAR8 *SourceBuffer,
	IN SIZE_T Flags)
{
	SIZE_T SpaceCount = 0;
	SIZE_T SourceBufferLength = 0;
	SIZE_T OutputLength = 0;

	if (Flags & _FORMAT_FLAG_PRECISION)
		SourceBufferLength = ClStrLengthU8(SourceBuffer, PrecisionWidth);
	else
		SourceBufferLength = ClStrLengthU8(SourceBuffer, -1);

	if (MinimumWidth > SourceBufferLength)
		SpaceCount = MinimumWidth - SourceBufferLength;

	if (SourceBufferLength > MinimumWidth)
		MinimumWidth = SourceBufferLength;

	if (!(Flags & _FORMAT_FLAG_LEFTJUSTIFY))
		FormatHelper_InternalStrFillU8(Buffer, BufferLength, &OutputLength, ' ', SpaceCount);

	FormatHelper_InternalStrCopyU8(Buffer, BufferLength, &OutputLength, SourceBuffer, SourceBufferLength);

	if (Flags & _FORMAT_FLAG_LEFTJUSTIFY)
		FormatHelper_InternalStrFillU8(Buffer, BufferLength, &OutputLength, ' ', SpaceCount);

	return OutputLength;
}

SIZE_T
FormatHelper_ValueToStringU8(
	OUT CHAR8 *Buffer,
	IN SIZE_T BufferLength,
	IN SIZE_T MinimumWidth,
	IN SIZE_T MinimumPrecisionWidth,
	IN S64 Value,
	IN SIZE_T Radix,
	IN SIZE_T Flags)
{
	static CHAR8 DigitTable[] = "0123456789abcdefghijklmnopqrstuvwxyz";

	CHAR8 ValueString[64 + 1];     // Value + Null
	CHAR8 PrefixBuffer[2 + 1 + 1]; // Prefix (0x-like) + Sign + Null
	CHAR8 TempBuffer[64 + 1];      // Value + Null

	SIZE_T OutputLength = 0;
	SIZE_T ValueLength = 0;
	SIZE_T PrefixLength = 0;
	SIZE_T TempLength = 0;
	CHAR8 CaseBit = 0;
	SSIZE_T Sgn = 0;

	U64 Unsigned64;
	SIZE_T WriteCount;
	SIZE_T i;

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
		if (Value)
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
		CHAR8 SignChar[2] = { '+', 0 };
		switch (Sgn)
		{
		case 1: SignChar[0] = '+'; break;
		case 0: break;
		case -1: SignChar[0] = '-'; break;
		default: break;
		}

		FormatHelper_InternalStrCopyU8(PrefixBuffer, ARRAY_SIZE(PrefixBuffer), &PrefixLength, SignChar, ARRAY_SIZE(SignChar));
	}

	if (Flags & _FORMAT_FLAG_PRINT_PREFIX)
	{
		CHAR8 *Prefix = "";
		SIZE_T Length = 0;

		switch (Radix)
		{
		case 2: Prefix = "0b"; Length = 2; break;
		case 8: Prefix = "0";  Length = 1; break;
			// case 10: Prefix = "0n"; Length = 2; break;
		case 16: Prefix = "0x"; Length = 2; break;
		default: Prefix = ""; Length = 0;
		}

		FormatHelper_InternalStrCopyU8(PrefixBuffer, ARRAY_SIZE(PrefixBuffer), &PrefixLength, Prefix, Length);
	}

	for (i = 0; i < ValueLength; i++)
	{
		CHAR8 ch = ValueString[ValueLength - i - 1];

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
		FormatHelper_InternalStrFillU8(Buffer, BufferLength, &OutputLength, ' ', MinimumWidth - MinimumPrecisionWidth);


	WriteCount = ((Flags & _FORMAT_FLAG_PRECISION) ?
		MinimumPrecisionWidth : MinimumWidth) - (TempLength + PrefixLength);
	if (Flags & _FORMAT_FLAG_ZERO)
	{
		// [Prefix][Zero][Number]
		//       0x  0..0   12345
		FormatHelper_InternalStrCopyU8(Buffer, BufferLength, &OutputLength, PrefixBuffer, PrefixLength);
		FormatHelper_InternalStrFillU8(Buffer, BufferLength, &OutputLength, '0', WriteCount);
	}
	else
	{
		// [Space][Prefix][Number]
		//              0x   12345
		FormatHelper_InternalStrFillU8(Buffer, BufferLength, &OutputLength, ' ', WriteCount);
		FormatHelper_InternalStrCopyU8(Buffer, BufferLength, &OutputLength, PrefixBuffer, PrefixLength);
	}

	FormatHelper_InternalStrCopyU8(Buffer, BufferLength, &OutputLength, TempBuffer, TempLength);


	if (Flags & _FORMAT_FLAG_LEFTJUSTIFY)
		FormatHelper_InternalStrFillU8(Buffer, BufferLength, &OutputLength, ' ', MinimumWidth - MinimumPrecisionWidth);


	// ASSERT(OutputLength =< BufferLength);

	// Do not terminate with null

	return OutputLength;
}

VOID
ClDbgHex(
	IN U64 Value)
{
	CHAR8 Buffer[65], *p;
	FormatHelper_ValueToStringU8(Buffer, ARRAY_SIZE(Buffer) - 1, 0, 0, Value, 16, 0);

	for (p = Buffer; *p; *p++)
		__PseudoIntrin_OutPortByte(0xe9, *p);

	__PseudoIntrin_OutPortByte(0xe9, '\n');
}

SIZE_T
ClStrFormatU8V(
	OUT CHAR8 *Buffer,
	IN SIZE_T BufferLength,
	IN CHAR8 *Format,
	IN va_list VarList)
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

	SIZE_T OutputLength = 0;
	SIZE_T InputLength = 0;

	CHAR8 *s = Format;
	CHAR8 *d = Buffer;

	while (*s)
	{
		U64 Flags = 0;
		BOOLEAN ParseEnd = FALSE;

		SIZE_T Width = 0;
		SIZE_T PrecisionWidth = 0;
		SIZE_T VarSize = 0;
		SIZE_T i;

		BOOLEAN Integer = TRUE;
		SIZE_T Radix = 0;
		S64 Value = 0;

		SIZE_T RemainingLength = 0;

		static struct {
			CHAR8 *Prefix;
			SIZE_T Flag;
		} LengthTable[] = {
			{ "ll", _FORMAT_FLAG_LENGTH_LONG, },
			{ "hh", _FORMAT_FLAG_LENGTH_SHORT2, },
			{ "l", _FORMAT_FLAG_LENGTH_LONG, },
			{ "h", _FORMAT_FLAG_LENGTH_SHORT, },
		};

//		ClDbgHex(&LengthTable);

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
			Width = va_arg(VarList, SIZE_T);
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
				PrecisionWidth = va_arg(VarList, SIZE_T);
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
			CHAR8 *p1 = s;
			CHAR8 *p2 = LengthTable[i].Prefix;
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
			if (sizeof(void *) == sizeof(__int64))
				Flags |= _FORMAT_FLAG_LENGTH_LONG;

			Flags |= (_FORMAT_FLAG_TREAT_UNSIGNED | _FORMAT_FLAG_ZERO | _FORMAT_FLAG_PRECISION | _FORMAT_FLAG_UPCASE);
			Flags &= ~(_FORMAT_FLAG_PRINT_SIGN | _FORMAT_FLAG_PRINT_PREFIX);

			Radix = 16;
			PrecisionWidth = sizeof(void *) << 1;
			break;

		case 'c':
			if (Flags & _FORMAT_FLAG_LENGTH_LONG) // wide
				Value = va_arg(VarList, CHAR16) & 0xffff;
			else
				Value = va_arg(VarList, S8) & 0xff;

			WRITE_CHAR((CHAR8)Value);

			Integer = FALSE;
			break;

		case 's':
			if (1)
			{
				VOID *String = va_arg(VarList, VOID *);
				SIZE_T StringLength = 0;

				if (BufferLength > OutputLength)
					RemainingLength = BufferLength - OutputLength;

				if (Flags & _FORMAT_FLAG_LENGTH_LONG)
					StringLength = 0; // FormatHelper_StrCopy(d, RemainingLength, Width, PrecisionWidth, (CHAR16 *)String, Flags);
				else
					StringLength = FormatHelper_StrCopyU8(d, RemainingLength, Width, PrecisionWidth, (CHAR8 *)String, Flags);

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
			SIZE_T NumberWidth = 0;

			if (Flags & _FORMAT_FLAG_LENGTH_LONG) // long or long long
				Value = va_arg(VarList, S64);
			else if (Flags & _FORMAT_FLAG_LENGTH_SHORT) // short
				Value = (U16)va_arg(VarList, S16);
			else if (Flags & _FORMAT_FLAG_LENGTH_SHORT2) // char
				Value = (U8)va_arg(VarList, S8);
			else
				Value = (U32)va_arg(VarList, S32);

			if (BufferLength > OutputLength)
				RemainingLength = BufferLength - OutputLength;

			NumberWidth = FormatHelper_ValueToStringU8(d, RemainingLength, Width, PrecisionWidth, Value, Radix, Flags);

			OutputLength += NumberWidth;
			d += NumberWidth;
		}
	}

#undef WRITE_CHAR

_Return :

	return OutputLength;
}

SIZE_T
ClStrFormatU8(
	OUT CHAR8 *Buffer,
	IN SIZE_T BufferLength,
	IN CHAR8 *Format,
	...)
{
	va_list List;
	SIZE_T Result;

	va_start(List, Format);
	Result = ClStrFormatU8V(Buffer, BufferLength, Format, List);
	va_end(List);

	return Result;
}

SIZE_T
ClStrTerminateU8(
	IN OUT CHAR8 *Buffer,
	IN SIZE_T BufferLength,
	IN SIZE_T Position)
{
	if (!BufferLength || Position > BufferLength)
		return 0;

	if (Position == BufferLength)
		Position--;

	Buffer[Position] = 0;

	return 1;
}

SIZE_T
ClStrCopyU8(
	OUT CHAR8 *Buffer,
	IN SIZE_T BufferLength,
	IN CHAR8 *SourceBuffer)
{
	SIZE_T Count = 0;

	while (SourceBuffer[Count] && Count < BufferLength)
	{
		Buffer[Count] = SourceBuffer[Count];
		Count++;
	}

	ClStrTerminateU8(Buffer, BufferLength, Count);

	return Count;
}
