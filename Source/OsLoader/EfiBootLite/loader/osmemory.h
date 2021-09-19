
#pragma once

typedef enum _OS_MEMORY_TYPE
{
    // 0x80000000 to 0xffffffff are reserved for use by UEFI OS loaders.
    OsSpecificMemTypeStart = (int)0x80000000,
    OsTemporaryData = OsSpecificMemTypeStart,
    OsLoaderData,
    OsLowMemory1M,
    OsKernelImage,
    OsKernelStack,
    OsBootImage,
    OsPreInitPool,
    OsPagingPxePool,
    OsFramebufferCopy,

    OsSpecificMemTypeEnd,
} OS_MEMORY_TYPE;



//
// X64 Paging Structures.
//

#define ARCH_X64_PA_TO_4K_PFN(_va)          (((UINT64)(_va)) >> EFI_PAGE_SHIFT)

#define ARCH_X64_VFN_TO_PML4EI(_va)         ((((UINT64)(_va)) >> 27) & 0x1ff)
#define ARCH_X64_VFN_TO_PDPTEI(_va)         ((((UINT64)(_va)) >> 18) & 0x1ff)
#define ARCH_X64_VFN_TO_PDEI(_va)           ((((UINT64)(_va)) >> 9) & 0x1ff)
#define ARCH_X64_VFN_TO_PTEI(_va)           ((((UINT64)(_va)) >> 0) & 0x1ff)

#define ARCH_X64_VA_TO_4K_VFN(_va)          (((UINT64)(_va)) >> EFI_PAGE_SHIFT)

#define ARCH_X64_VA_TO_PML4EI(_va)          ARCH_X64_VFN_TO_PML4EI(ARCH_X64_VA_TO_4K_VFN(_va))
#define ARCH_X64_VA_TO_PDPTEI(_va)          ARCH_X64_VFN_TO_PDPTEI(ARCH_X64_VA_TO_4K_VFN(_va))
#define ARCH_X64_VA_TO_PDEI(_va)            ARCH_X64_VFN_TO_PDEI(ARCH_X64_VA_TO_4K_VFN(_va))
#define ARCH_X64_VA_TO_PTEI(_va)            ARCH_X64_VFN_TO_PTEI(ARCH_X64_VA_TO_4K_VFN(_va))
#define ARCH_X64_VA_TO_OFFSET(_va)          (((UINT64)(_va)) & ((1 << EFI_PAGE_SHIFT) - 1))


#define ARCH_X64_PXE_PRESENT                (1ULL << 0)
#define ARCH_X64_PXE_WRITABLE               (1ULL << 1)
#define ARCH_X64_PXE_USER                   (1ULL << 2)
#define ARCH_X64_PXE_WRITE_THROUGH          (1ULL << 3)
#define ARCH_X64_PXE_CACHE_DISABLED         (1ULL << 4)
#define ARCH_X64_PXE_LARGE_SIZE             (1ULL << 7)
#define ARCH_X64_PXE_EXECUTE_DISABLED       (1ULL << 63)

#define ARCH_X64_PXE_4K_BASE_MASK           0x000ffffffffff000ULL // [51:12], 4K
#define ARCH_X64_PXE_2M_BASE_MASK           0x000fffffffe00000ULL // [51:21], 2M

#define ARCH_X64_CR3_PML4_BASE_MASK         0xfffffffffffff000ULL // [M-1:12], 4K

#if 0
typedef union _ARCH_X64_VIRTUAL_ADDRESS
{
    struct
    {
        UINT64 NotUsed : 16;
        UINT64 PML4Ei : 9;
        UINT64 PDPTEi : 9;
        UINT64 PDEi : 9;
        UINT64 PTEi : 9;
        UINT64 Offset : 12;
    } b;
    
    UINT64 Address;
} ARCH_X64_VIRTUAL_ADDRESS;

typedef union _ARCH_X64_PML4E
{
    struct
    {
        UINT64 Present : 1;
        UINT64 Writable : 1;
        UINT64 User : 1;
        UINT64 WriteThru : 1;
        UINT64 CacheDisable : 1;
        UINT64 Accessed : 1;
        UINT64 Ignored1 : 1;
        UINT64 ReservedZero : 1;
        UINT64 Ignored2 : 4;
        UINT64 PDPTBase : 40;
        UINT64 Ignored3 : 11;
        UINT64 ExecuteDisable : 1;
    } b;
    UINT64 Value;
} ARCH_X64_PML4E;

typedef union _ARCH_X64_PDPTE
{
    struct
    {
        UINT64 Present : 1;
        UINT64 Writable : 1;
        UINT64 User : 1;
        UINT64 WriteThru : 1;
        UINT64 CacheDisable : 1;
        UINT64 Accessed : 1;
        UINT64 Ignored1 : 1;
        UINT64 ReservedZero : 1;
        UINT64 Ignored2 : 4;
        UINT64 PDBase : 40;
        UINT64 Ignored3 : 11;
        UINT64 ExecuteDisable : 1;
    } b;
    UINT64 Value;
} ARCH_X64_PDPTE;

typedef union _ARCH_X64_PDE
{
    struct
    {
        UINT64 Present : 1;
        UINT64 Writable : 1;
        UINT64 User : 1;
        UINT64 WriteThru : 1;
        UINT64 CacheDisable : 1;
        UINT64 Accessed : 1;
        UINT64 Ignored1 : 1;
        UINT64 ReservedZero : 1;
        UINT64 Ignored2 : 4;
        UINT64 PTBase : 40;
        UINT64 Ignored3 : 11;
        UINT64 ExecuteDisable : 1;
    } b;
    UINT64 Value;
} ARCH_X64_PDE;

typedef union _ARCH_X64_PDE_2MB
{
    struct
    {
        UINT64 Present : 1;
        UINT64 Writable : 1;
        UINT64 User : 1;
        UINT64 WriteThru : 1;
        UINT64 CacheDisable : 1;
        UINT64 Accessed : 1;
        UINT64 Dirty : 1;
        UINT64 PageSize2M : 1; // must be 1
        UINT32 Global : 1;
        UINT64 Ignored1 : 3;
        UINT64 PAT : 1;
        UINT64 Reserved : 8;
        UINT64 Base : 31;
        UINT64 Ignored3 : 7;
        UINT64 ProtectionKey : 4;
        UINT64 ExecuteDisable : 1;
    } b;
    UINT64 Value;
} ARCH_X64_PDE_2MB;

typedef union _ARCH_X64_PTE
{
    struct
    {
        UINT64 Present : 1;
        UINT64 Writable : 1;
        UINT64 User : 1;
        UINT64 WriteThru : 1;
        UINT64 CacheDisable : 1;
        UINT64 Accessed : 1;
        UINT64 Dirty : 1;
        UINT64 PAT : 1;
        UINT64 Global : 1;
        UINT64 Ignored2 : 3;
        UINT64 Base : 40;
        UINT64 Ignored3 : 7;
        UINT64 ProtectionKey : 4;
        UINT64 ExecuteDisable : 1;
    } b;
    UINT64 Value;
} ARCH_X64_PTE;
#endif


//
// Memory routines.
//

EFI_STATUS
EFIAPI
OslAllocatePages(
    IN UINTN Size, 
    IN OUT EFI_PHYSICAL_ADDRESS *Address, 
    IN BOOLEAN AddressSpecified,
    IN OS_MEMORY_TYPE MemoryType);

EFI_STATUS
EFIAPI
OslFreePages(
    IN EFI_PHYSICAL_ADDRESS Address,
    IN UINTN Size);

EFI_STATUS
EFIAPI
OslAllocatePagesPreserve(
    IN OS_LOADER_BLOCK *LoaderBlock,
    IN UINTN Size, 
    IN OUT EFI_PHYSICAL_ADDRESS *Address, 
    IN BOOLEAN AddressSpecified,
    IN OS_MEMORY_TYPE MemoryType);

EFI_STATUS
EFIAPI
OslFreePagesPreserve(
    IN OS_LOADER_BLOCK *LoaderBlock,
    IN EFI_PHYSICAL_ADDRESS Address, 
    IN UINTN Size);

EFI_STATUS
EFIAPI
OslQueryMemoryMap (
    IN OS_LOADER_BLOCK *LoaderBlock);

VOID
EFIAPI
OslFreeMemoryMap(
    IN OS_LOADER_BLOCK *LoaderBlock);

VOID
EFIAPI
OslDumpMemoryMap(
    IN EFI_MEMORY_DESCRIPTOR *Map,
    IN UINTN MemoryMapSize,
    IN UINTN DescriptorSize);

BOOLEAN
EFIAPI
OslSetupPaging(
    IN OS_LOADER_BLOCK *LoaderBlock);



UINT64
EFIAPI
OslArchX64GetCr3(
    VOID);

VOID
EFIAPI
OslArchX64SetCr3(
    IN UINT64 Cr3);

