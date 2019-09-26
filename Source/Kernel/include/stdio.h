#pragma once

#include <stddef.h>
#include <stdarg.h>

//#pragma intrinsic(memset, memcmp, memcpy)
#pragma intrinsic(strcpy, strlen, strcmp, strcat)
#pragma intrinsic(wcscpy, wcslen, wcscmp, wcscat)

#define is_digit(c) ((c) >= '0' && (c) <= '9')


//
// standard runtime functions.
//

extern void *memset(void *dest, int v, size_t size);
extern int memcmp(const void *s1, const void *s2, size_t size);
extern void *memcpy(void *dest, const void *src, size_t size);


extern int vsprintf(char *buffer, const char *format, va_list argptr);
extern int vsnprintf(char *dest, unsigned int buflen, const char *format, va_list args);


//
// string.
//

int tolower(int ch);
int toupper(int ch);

char *strcpy(char *__restrict dest, const char *__restrict src);
char *strncpy(char *__restrict dest, const char *__restrict src, size_t count);
char *strcat(char *__restrict dest, const char *__restrict src);
char *strncat(char *__restrict dest, const char *__restrict src, size_t count);
size_t strlen(const char *str);

int strcmp(const char *lhs, const char *rhs);
int strncmp(const char *lhs, const char *rhs, size_t count);
int stricmp(const char *lhs, const char *rhs);
int strnicmp(const char *lhs, const char *rhs, size_t count);

char *strchr(const char *str, int ch);
char *strrchr(const char *str, int ch);

size_t strspn(const char *dest, const char *src);
size_t strcspn(const char *dest, const char *src);
char *strpbrk(const char *dest, const char *breakset);
char *strstr(const char *str, const char *substr);


//
// string, wide-char version.
//

wchar_t *wcscpy(wchar_t *__restrict dest, const wchar_t *__restrict src);
wchar_t *wcsncpy(wchar_t *__restrict dest, const wchar_t *__restrict src, size_t count);
wchar_t *wcscat(wchar_t *__restrict dest, const wchar_t *__restrict src);
wchar_t *wcsncat(wchar_t *__restrict dest, const wchar_t *__restrict src, size_t count);
size_t wcslen(const wchar_t *str);

int wcscmp(const wchar_t *lhs, const wchar_t *rhs);
int wcsncmp(const wchar_t *lhs, const wchar_t *rhs, size_t count);
int wcsicmp(const wchar_t *lhs, const wchar_t *rhs);
int wcsnicmp(const wchar_t *lhs, const wchar_t *rhs, size_t count);

wchar_t *wcschr(const wchar_t *str, int ch);
wchar_t *wcsrchr(const wchar_t *str, int ch);

size_t wcsspn(const wchar_t *dest, const wchar_t *src);
size_t wcscspn(const wchar_t *dest, const wchar_t *src);
wchar_t *wcspbrk(const wchar_t *dest, const wchar_t *breakset);
wchar_t *wcsstr(const wchar_t *str, const wchar_t *substr);

