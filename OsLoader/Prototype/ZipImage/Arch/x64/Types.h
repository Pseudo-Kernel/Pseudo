#pragma once

// Character Types.
typedef __int8						CHAR8, *PCHAR8;
typedef __int16						CHAR16, *PCHAR16;

// Integer Types.
typedef __int8						S8, *PS8;
typedef __int16						S16, *PS16;
typedef __int32						S32, *PS32;
typedef __int64						S64, *PS64;

typedef unsigned __int8				U8, *PU8;
typedef unsigned __int16			U16, *PU16;
typedef unsigned __int32			U32, *PU32;
typedef unsigned __int64			U64, *PU64;

// Pointer Types.
#ifndef __WINDOWS__
#ifndef VOID
typedef	void						VOID;
#endif
typedef	VOID						*PVOID, **PPVOID;
#endif

typedef __int32						PTR32, *PPTR32;
typedef __int32						SPTR32, *PSPTR32;
typedef unsigned __int32			UPTR32, *PUPTR32;
typedef __int64						PTR64, *PPTR64;
typedef __int64						SPTR64, *PSPTR64;
typedef unsigned __int64			UPTR64, *PUPTR64;

#if (defined _X64_) || (defined _AMD64_)
typedef PTR64						PTR, *PPTR, **PPPTR;
typedef SPTR64						SPTR, *PSPTR, **PPSPTR;
typedef UPTR64						UPTR, *PUPTR, **PPUPTR;
#elif defined _X86_
typedef PTR32						PTR, *PPTR, **PPPTR;
typedef SPTR32						SPTR, *PSPTR, **PPSPTR;
typedef UPTR32						UPTR, *PUPTR, **PPUPTR;
#else
#error Unknown architecture!
#endif


// Other types.
#ifndef __WINDOWS__
typedef	U8							BOOLEAN, *PBOOLEAN;
#endif

