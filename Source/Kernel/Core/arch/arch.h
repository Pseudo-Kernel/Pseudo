#pragma once


#define _ARCH_TYPE_UNKNOWN_                     0
#define _ARCH_TYPE_X86_                         1
#define _ARCH_TYPE_X64_                         2


#ifdef __GNUC__
#ifdef __i386__
#define _ARCH_TYPE_                             _ARCH_TYPE_X86_
#elif defined __x86_64__
#define _ARCH_TYPE_                             _ARCH_TYPE_X64_
#else
#define _ARCH_TYPE_                             _ARCH_TYPE_UNKNOWN_
#endif
#else
#error gcc is required!
#endif // __GNUC__



#if _ARCH_TYPE_ == _ARCH_TYPE_X86_
#include "x86/Types.h"
#elif _ARCH_TYPE_ == _ARCH_TYPE_X64_
#include "x64/Types.h"
#include "x64/intrin64.h"
#else
#error Unknown architecture!
#endif
