
//#pragma message("Must be compiled without: /GL option")

#include <base/base.h>

//#pragma function(memset, memcmp, memcpy)

//
// standard runtime functions.
//

void *memset(void *dest, int v, size_t size)
{
	unsigned char *p = (unsigned char *)dest;

	while (size-- > 0)
		*p++ = (unsigned char)v;

	return dest;
}

int memcmp(const void *s1, const void *s2, size_t size)
{
	unsigned char *p1 = (unsigned char *)s1;
	unsigned char *p2 = (unsigned char *)s2;

	while (size-- > 0)
	{
		int diff = *p1 - *p2;
		if (diff != 0)
			return diff;
		p1++, p2++;
	}

	return 0;
}

void *memcpy(void *dest, const void *src, size_t size)
{
	unsigned char *s = (unsigned char *)src;
	unsigned char *d = (unsigned char *)dest;

	while (size-- > 0)
		*d++ = *s++;

	return dest;
}

size_t strlen(const char *s)
{
	size_t len = 0;
	while (*s++)
		len++;

	return len;
}

#if 0

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

#endif
