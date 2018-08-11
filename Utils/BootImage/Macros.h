#pragma once

//
// Calling Conventions.
//

#define	CALLCONV					__stdcall


//
// Tag4, Tag8.
//

#define	TAG4(_a, _b, _c, _d) ( \
	( (((U32)(_a)) & 0xff) << 0x00 ) | \
	( (((U32)(_b)) & 0xff) << 0x08 ) | \
	( (((U32)(_c)) & 0xff) << 0x10 ) | \
	( (((U32)(_d)) & 0xff) << 0x18 ) \
)

#if 0
#define	TAG8(_a, _b, _c, _d, _e, _f, _g, _h) ( \
	( ((unsigned __int64)TAG4((_a), (_b), (_c), (_d))) << 0x00 ) | \ 
	( ((unsigned __int64)TAG4((_e), (_f), (_g), (_h))) << 0x20 ) \
)
#endif

//
// Misc.
//

#define	NULL				((void *)0)
#ifndef TRUE
#define	TRUE				(1 == 1)
#endif // !TRUE
#ifndef FALSE
#define	FALSE				(0 == 1)
#endif // !FALSE


//
// IN, OUT, OPTIONAL
//

#ifndef IN
#define	IN
#endif // !IN
#ifndef OUT
#define	OUT
#endif // !OUT
#ifndef OPTIONAL
#define	OPTIONAL
#endif // !OPTIONAL


//
// Character Macros.
//

#define	IsLowerAlphabet(_s)			( 'a' <= ((U8)(_s)) && ((U8)(_s)) <= 'z' )
#define	IsUpperAlphabet(_s)			( 'A' <= ((U8)(_s)) && ((U8)(_s)) <= 'Z' )
#define	IsAlphabet(_s)				( IsLowerAlphabet(_s) || IsUpperAlphabet(_s) )
#define	IsNumberic(_s)				( '0' <= ((U8)(_s)) && ((U8)(_s)) <= '9' )

#define	ToLowerAlphabet(_s)			( ((U8)(_s)) | 0x20 )
#define	ToUpperAlphabet(_s)			( ((U8)(_s)) & ~0x20 )
