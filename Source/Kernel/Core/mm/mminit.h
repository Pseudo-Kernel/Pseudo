
#pragma once

#define	REQUIREMENT_MEMORY_SIZE			0x2000000


//
// Defines initial virtual memory map.
//

#define KERNEL_VA_SIZE_LOADER_SPACE                     0x0000001000000000ULL // 64G
#define KERNEL_VA_SIZE_LOADER_SPACE_ASLR_GAP            0x0000008000000000ULL // 512G

#define KERNEL_VA_START_LOADER_SPACE                    0xffff8f0000000000ULL // Used in initialization
#define KERNEL_VA_END_LOADER_SPACE                      (KERNEL_VA_START_LOADER_SPACE + KERNEL_VA_SIZE_LOADER_SPACE + KERNEL_VA_SIZE_LOADER_SPACE_ASLR_GAP)


#define KERNEL_VA_SIZE_ASLR_GAP                         0x0000008000000000ULL // 512G
#define KENREL_VA_SIZE_PAD_LIST                         0x0000080000000000ULL // 8T (128 byte per each entry)
#define KENREL_VA_SIZE_VAD_LIST                         0x0000080000000000ULL // 8T (128 byte per each entry)
#define KERNEL_VA_SIZE_KERNEL_POOL                      0x0000080000000000ULL // 8T (Paged + NonPaged)
#define KERNEL_VA_SIZE_PXE_AREA                         0x0000010000000000ULL // 1T (PT + PD + PDPT + PML4T + Gap)

#define KERNEL_VA_START_KERNEL_POOL                     0xffffa00000000000ULL
#define KERNEL_VA_END_KERNEL_POOL                       (KERNEL_VA_START_KERNEL_POOL + KERNEL_VA_SIZE_KERNEL_POOL)
#define KERNEL_VA_START_PAD_LIST                        0xffffb00000000000ULL
#define KERNEL_VA_END_PAD_LIST                          (KERNEL_VA_START_PAD_LIST + KENREL_VA_PAD_LIST_SIZE)
#define KERNEL_VA_START_VAD_LIST                        0xffffc00000000000ULL
#define KERNEL_VA_END_VAD_LIST                          (KERNEL_VA_START_VAD_LIST + KENREL_VA_VAD_LIST_SIZE)
#define KERNEL_VA_START_PXE_AREA                        0xffffd00000000000ULL
#define KERNEL_VA_END_PXE_AREA                          (KERNEL_VA_START_PXE_AREA + KERNEL_VA_SIZE_PXE_AREA)


#define KERNEL_VA_SIZE_PT                               (8 * (1ULL << 36))
#define KERNEL_VA_SIZE_PD                               (8 * (1ULL << 27))
#define KERNEL_VA_SIZE_PDPT                             (8 * (1ULL << 18))
#define KERNEL_VA_SIZE_PML4                             (8 * (1ULL << 9))

#define KERNEL_VA_PT_BASE                               (KERNEL_VA_START_PXE_AREA + 0)
#define KERNEL_VA_PD_BASE                               (KERNEL_VA_PT_BASE + KERNEL_VA_SIZE_PT)
#define KERNEL_VA_PDPT_BASE                             (KERNEL_VA_PD_BASE + KERNEL_VA_SIZE_PD)
#define KERNEL_VA_PML4_BASE                             (KERNEL_VA_PDPT_BASE + KERNEL_VA_SIZE_PDPT)


BOOLEAN
KERNELAPI
MiDiscardFirmwareMemory(
	VOID);

BOOLEAN
KERNELAPI
MiPrePoolInitialize(
	IN OS_LOADER_BLOCK *LoaderBlock);

ESTATUS
KERNELAPI
MiPreInitialize(
    IN OS_LOADER_BLOCK *LoaderBlock);

VOID
KERNELAPI
MiPreDumpXad(
    VOID);


