#pragma once


#define _ARCH_TYPE_UNKNOWN_                     0
#define _ARCH_TYPE_X86_                         1
#define _ARCH_TYPE_X64_                         2


#define __STDC_VERSION_C11__                    201112L // ISO/IEC 9899:2011
#define __STDC_VERSION_C18__                    201710L // ISO/IEC 9899:2018

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < __STDC_VERSION_C11__)
#error Compiler must support C11 or higher!
#endif


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
#include "x86/types.h"
#elif _ARCH_TYPE_ == _ARCH_TYPE_X64_
#include "x64/types.h"
#include "x64/processor.h"
#include "x64/intrin64.h"
#else
#error Unknown architecture!
#endif
