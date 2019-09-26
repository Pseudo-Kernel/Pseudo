#pragma once

typedef U8					*va_list;

#define _INTSIZEOF(n)		( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )

#define va_start(ap,p)		ap = (va_list)((U8 *)(&(p)) + sizeof(U8 *))
#define va_arg(ap,t)		( *(t *)((ap += sizeof(U8 *)) - sizeof(U8 *)) )
#define va_end(ap)			ap = (va_list)0

