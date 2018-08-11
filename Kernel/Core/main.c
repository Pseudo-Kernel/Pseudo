
#include <Arch\Arch.h>
#include <Macros.h>

#pragma comment(linker, "/Entry:KiKernelStart")

#pragma intrinsic(strlen)
unsigned __int64 strlen(char *p);
#if 0
{
	char *first = p;
	while (*p++);

	return (int)(p - first);
}
#endif

U64
CALLCONV
KiKernelStart(
	IN PTR LoadedBase, 
	IN PTR LoaderBlock, 
	IN U32 SizeOfLoaderBlock, 
	IN PTR Reserved)
{
	CHAR8 s[10] = "456789";
	U64 l = strlen(s);

	*(volatile U64 *)0xb8000 = l;

	return l;
}

