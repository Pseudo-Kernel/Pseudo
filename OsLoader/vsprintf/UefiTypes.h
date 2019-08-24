#pragma once

typedef	char				CHAR8, *PCHAR8;
typedef	unsigned short		CHAR16, *PCHAR16;


#if 1
typedef	__int64				INTN, *PINTN;
typedef	unsigned __int64	UINTN, *PUINTN;
#else
typedef	__int32				INTN, *PINTN;
typedef	unsigned __int32	UINTN, *PUINTN;
#endif

#define	VA_START			va_start
#define	VA_ARG				va_arg
#define	VA_END				va_end
#define	VA_LIST				va_list

#define	ARRAY_SIZE			_countof



typedef	__int8				S8;
typedef	unsigned __int8		U8;
typedef	__int16				S16;
typedef	unsigned __int16	U16;
typedef	__int32				S32;
typedef	unsigned __int32	U32;
typedef	__int64				S64;
typedef	unsigned __int64	U64;
