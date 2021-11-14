
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

#ifdef _X86_
typedef PTR32                       PTR, *PPTR;
typedef UPTR32                      UPTR, *PUPTR;
#elif (defined _AMD64_)
typedef PTR64                       PTR, *PPTR;
typedef UPTR64                      UPTR, *PUPTR;
#else
#error Requires x86 or amd64!
#endif


#define KASSERT(_x)                 if (!(_x)) { __debugbreak(); }

#include "gerror.h"

