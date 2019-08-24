#pragma once

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && !defined(_ARM_) && defined(_M_IX86)
#define _X86_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && !defined(_ARM_) && defined(_M_AMD64)
#define _AMD64_
#define	_X64_
#endif


#ifdef _X86_
#include "x86/Types.h"
#elif defined _X64_
#include "x64/Types.h"
#include "x64/intrin64.h"
#endif
