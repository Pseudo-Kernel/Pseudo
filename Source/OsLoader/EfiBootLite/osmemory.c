/** @file
	Memory-Related Routines.
**/

#include "OsLoader.h"

//static UINT8 OslGlobalMemoryMap[0x1000]; // dummy

/**
	Frees the memory map.

	@param[in] LoaderBlock    The loader block which contains OS loader information.

	@retval None.

**/
VOID
EFIAPI
OslMmFreeMemoryMap(
	IN OS_LOADER_BLOCK *LoaderBlock)
{
	VOID *Map = (VOID *)LoaderBlock->Memory.Map;

	gBS->FreePool(Map);

	LoaderBlock->Memory.Map = NULL;
	LoaderBlock->Memory.MapKey = 0;
	LoaderBlock->Memory.MapCount = 0;
	LoaderBlock->Memory.DescriptorSize = 0;
	LoaderBlock->Memory.DescriptorVersion = 0;
}

/**
	Allocates and querys the memory map.

	@param[in] LoaderBlock    The loader block which contains OS loader information.

	@retval EFI_SUCCESS       The operation is completed successfully.
	@retval other             An error occurred during the operation.

**/
EFI_STATUS
EFIAPI
OslMmQueryMemoryMap (
	IN OS_LOADER_BLOCK *LoaderBlock)
{
	EFI_MEMORY_DESCRIPTOR *Map;
	UINTN MapSize;
	UINTN MapKey;
	UINTN DescriptorSize;
	UINT32 DescriptorVersion;
	EFI_STATUS Status;
	UINTN TryCount;

	MapSize = 0;
	gBS->GetMemoryMap(&MapSize, NULL, &MapKey, &DescriptorSize, &DescriptorVersion);

	for (TryCount = 0; TryCount < 0x10; TryCount++)
	{
		Status = gBS->AllocatePool(EfiLoaderData, MapSize, (void **)&Map);
		if (Status != EFI_SUCCESS)
			return Status;

		Status = gBS->GetMemoryMap(&MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
		if (Status == EFI_SUCCESS)
			break;

		gBS->FreePool(Map);
	}

	if (Status != EFI_SUCCESS)
	{
		TRACEF(L"Failed to query the memory map, Status 0x%08x\r\n", Status);
		return Status;
	}

	LoaderBlock->Memory.Map = Map;
	LoaderBlock->Memory.MapKey = MapKey;
	LoaderBlock->Memory.MapCount = MapSize / DescriptorSize;
	LoaderBlock->Memory.DescriptorSize = DescriptorSize;
	LoaderBlock->Memory.DescriptorVersion = DescriptorVersion;

	return EFI_SUCCESS;
}

/**
	Prints the memory map.

	@param[in] Map            Pointer of memory descriptor list.
	@param[in] MemoryMapSize  Size of memory descriptors.
	@param[in] DescriptorSize Size of memory descriptor entry.

	@retval                   None.

**/
VOID
EFIAPI
OslMmDumpMemoryMap(
	IN EFI_MEMORY_DESCRIPTOR *Map, 
	IN UINTN MemoryMapSize, 
	IN UINTN DescriptorSize)
{
	UINTN MapCount;
	UINTN i;

	if (!IS_DEBUG_MODE(&OslLoaderBlock))
	{
		return;
	}

	MapCount = MemoryMapSize / DescriptorSize;

	DTRACE(
		&OslLoaderBlock, 
		L"\r\n"
		L"Firmware Memory Map (MapCount = %d)\r\n"
		L"  i         AddressStart          AddressEnd                        Type\r\n",
		MapCount);

	for (i = 0; i < MapCount; i++)
	{
		CHAR16 TypeString[26 + 1] = { 0 };
		UINTN Length;
		EFI_MEMORY_DESCRIPTOR *MapEntry = (EFI_MEMORY_DESCRIPTOR *)((INT8 *)Map + i * DescriptorSize);

		#define	CASE_COPY_STRING(_copy_to, _val_name)	\
			case _val_name:								\
				Length = StrFormat(TypeString, ARRAY_SIZE(TypeString), L"%s", ASCII(_val_name));\
				StrTerminate(TypeString, ARRAY_SIZE(TypeString), Length);\
				break;

		switch (MapEntry->Type)
		{
		CASE_COPY_STRING(TypeString, EfiReservedMemoryType)
		CASE_COPY_STRING(TypeString, EfiLoaderCode)
		CASE_COPY_STRING(TypeString, EfiLoaderData)
		CASE_COPY_STRING(TypeString, EfiBootServicesCode)
		CASE_COPY_STRING(TypeString, EfiBootServicesData)
		CASE_COPY_STRING(TypeString, EfiRuntimeServicesCode)
		CASE_COPY_STRING(TypeString, EfiRuntimeServicesData)
		CASE_COPY_STRING(TypeString, EfiConventionalMemory)
		CASE_COPY_STRING(TypeString, EfiUnusableMemory)
		CASE_COPY_STRING(TypeString, EfiACPIReclaimMemory)
		CASE_COPY_STRING(TypeString, EfiACPIMemoryNVS)
		CASE_COPY_STRING(TypeString, EfiMemoryMappedIO)
		CASE_COPY_STRING(TypeString, EfiMemoryMappedIOPortSpace)
		CASE_COPY_STRING(TypeString, EfiPalCode)
		CASE_COPY_STRING(TypeString, EfiPersistentMemory)
		CASE_COPY_STRING(TypeString, EfiMaxMemoryType)
		default:
			Length = StrFormat(TypeString, ARRAY_SIZE(TypeString) - 1, L"UnknownType (0x%08X)", MapEntry->Type);
			StrTerminate(TypeString, ARRAY_SIZE(TypeString), Length);
//			AsciiSPrint(TypeString, ARRAY_SIZE(TypeString), "UnknownType (0x%08X)", MapEntry->Type);
			break;
		}

		DTRACE(
			&OslLoaderBlock, 
			L" %2d:  0x%016lX  0x%016lX  %26ls\r\n",
			i,
			MapEntry->PhysicalStart,
			MapEntry->PhysicalStart + (MapEntry->NumberOfPages << EFI_PAGE_SHIFT) - 1,
			TypeString);

		if (!((i + 1) % 20))
		{
			OslDbgWaitEnterKey(&OslLoaderBlock, NULL);
		}

		#undef CASE_COPY_STRING
	}

	DTRACE(&OslLoaderBlock, L"End of Memory Map.\r\n");
	OslDbgWaitEnterKey(&OslLoaderBlock, NULL);

	TRACE(L"\r\n");
}

#if 0
/**
	Allocates 4-Level Paging Structures.

	@param[in] LoaderBlock    The loader block which contains OS loader information.

	@retval FALSE             An error occurred during the operation.
	@retval other             The operation is completed successfully.

**/
EFI_STATUS
EFIAPI
OslX64PreparePaging(
	IN OS_LOADER_BLOCK *LoaderBlock)
{
	EFI_BOOT_SERVICES *BootServices = LoaderBlock->Base.BootServices;


//	BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, )
}
#endif
