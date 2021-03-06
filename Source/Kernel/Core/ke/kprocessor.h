#pragma once

#include <base/base.h>
#include <ke/irql.h>


//
// Bits for CR0.
//

#define ARCH_X64_CR0_PE                                     (1 << 0)
#define ARCH_X64_CR0_MP                                     (1 << 1)
#define ARCH_X64_CR0_EM                                     (1 << 2)
#define ARCH_X64_CR0_TS                                     (1 << 3)
#define ARCH_X64_CR0_NE                                     (1 << 5)
#define ARCH_X64_CR0_WP                                     (1 << 16)
#define ARCH_X64_CR0_NW                                     (1 << 29)
#define ARCH_X64_CR0_CD                                     (1 << 30)
#define ARCH_X64_CR0_PG                                     (1 << 31)

// CD=0 | NW=0 | WP=1 | NE=1 | EM=0 | MP=1 | TS=0
// MCE=1 | PGE=1 | OSFXSR=1 | OSXMMEXCPT=1 | TSD=0


//
// Bits for CR4.
//

#define ARCH_X64_CR4_TSD                                    (1 << 2)
#define ARCH_X64_CR4_PAE                                    (1 << 5)
#define ARCH_X64_CR4_MCE                                    (1 << 6)
#define ARCH_X64_CR4_PGE                                    (1 << 7)
#define ARCH_X64_CR4_OSFXSR                                 (1 << 9)
#define ARCH_X64_CR4_OSXMMEXCPT                             (1 << 10)
#define ARCH_X64_CR4_FSGSBASE                               (1 << 16)
#define ARCH_X64_CR4_PCIDE                                  (1 << 17)
#define ARCH_X64_CR4_OSXSAVE                                (1 << 18)


//
// Flags for x64 GDT entries.
//

#define ARCH_X64_SEGMENT_TYPE_DATA_READONLY                 0
#define ARCH_X64_SEGMENT_TYPE_DATA_READWRITE                2
#define ARCH_X64_SEGMENT_TYPE_DATA_READONLY_EXPAND          4
#define ARCH_X64_SEGMENT_TYPE_DATA_READWRITE_EXPAND         6
#define ARCH_X64_SEGMENT_TYPE_CODE_EXECUTEONLY              8
#define ARCH_X64_SEGMENT_TYPE_CODE_EXECUTE_READ             10
#define ARCH_X64_SEGMENT_TYPE_CODE_EXECUTEONLY_CONFORMING   12
#define ARCH_X64_SEGMENT_TYPE_CODE_EXECUTE_READ_CONFORMING  14

#define ARCH_X64_SEGMENT_TYPE_SYSTEM_TSS                    9
#define ARCH_X64_SEGMENT_TYPE_SYSTEM_TSS_BUSY               11
#define ARCH_X64_SEGMENT_TYPE_SYSTEM_CALLGATE               12
#define ARCH_X64_SEGMENT_TYPE_SYSTEM_INTGATE                14
#define ARCH_X64_SEGMENT_TYPE_SYSTEM_TRAPGATE               15
#define ARCH_X64_SEGMENT_TYPE_MASK                          0x0f

#define ARCH_X64_SEGMENT_DPL_MASK                           0x03

#define ARCH_X64_SEGMENT_TYPE(_type)                        (((U64)(_type) & ARCH_X64_SEGMENT_TYPE_MASK) << (8 + 32))
#define ARCH_X64_SEGMENT_CODE_DATA                          (1ULL << (12 + 32)) // treated as code/data seg if set
#define ARCH_X64_SEGMENT_DPL(_dpl)                          (((U64)(_dpl) & ARCH_X64_SEGMENT_DPL_MASK) << (13 + 32))

#define ARCH_X64_SEGMENT_PRESENT                            (1ULL << (15 + 32))
#define ARCH_X64_SEGMENT_USER_DEFINED_BIT                   (1ULL << (20 + 32)) // free to use
#define ARCH_X64_SEGMENT_64BIT                              (1ULL << (21 + 32)) // treated as 64-bit segment if set
#define ARCH_X64_SEGMENT_DEFAULT                            (1ULL << (22 + 32)) // treated as 32-bit segment if set
#define ARCH_X64_SEGMENT_GRANULARITY                        (1ULL << (23 + 32)) // when flag is set, the segment limit is interpreted in 4-KByte units


//
// Flags for x64 IDT entries.
//

// 
// When an interrupt is handled through an interrupt gate, the IF flag is automatically cleared, which disables
// maskable hardware interrupts. (If an interrupt is handled through a trap gate, the IF flag is not cleared.)
// [From. Intel SDM, 6.8.1 Masking Maskable Hardware Interrupts]
// 

//#define ARCH_X64_IDTENTRY_ATTRIBUTE_TYPE_TASK               5 // 0101b
#define ARCH_X64_IDTENTRY_ATTRIBUTE_TYPE_INTERRUPT          14 // 1110b, Clears RFLAGS.IF
#define ARCH_X64_IDTENTRY_ATTRIBUTE_TYPE_TRAP               15 // 1111b, RFLAGS.IF not changed

#define ARCH_X64_IDTENTRY_ATTRIBUTE_IST(_ist)               (((_ist) & 0x07) << 0)
#define ARCH_X64_IDTENTRY_ATTRIBUTE_TYPE(_type)             (((_type) & 0x0f) << 8)
#define ARCH_X64_IDTENTRY_ATTRIBUTE_DPL(_dpl)               (((_dpl) & 0x03) << 13)
#define ARCH_X64_IDTENTRY_ATTRIBUTE_PRESENT                 (1 << 15)

//
// x64 IDT entry.
//

typedef struct _ARCH_X64_IDTENTRY
{
    U16 OffsetLow;
    U16 Selector;
    U16 Attributes;
    U16 OffsetMid;
    U32 OffsetHigh;
    U32 Reserved;
} ARCH_X64_IDTENTRY;


// 
// x64 TSS.
// 

#pragma pack(push, 4)
typedef struct _ARCH_X64_TSS
{
    U32 Reserved1;
    U64 Rsp0;
    U64 Rsp1;
    U64 Rsp2;
    U64 Reserved2;
    U64 Ist1;
    U64 Ist2;
    U64 Ist3;
    U64 Ist4;
    U64 Ist5;
    U64 Ist6;
    U64 Ist7;
    U64 Reserved3;
    U16 Reserved4;

    //
    // If the I/O bit map base address is greater than or equal to the TSS segment limit, there is no I/O permission map,
    // and all I/O instructions generate exceptions when the CPL is greater than the current IOPL.
    // 
    // [From: Intel SDM, Volume 1, 19.5.2 I/O Permission Bit Map]
    //

    U16 IoMapBase;
} ARCH_X64_TSS;
#pragma pack(pop)


//
// Limits for IDT/GDT.
//

#define IDTENTRY_MAXIMUM_COUNT                              256 // IDT[0..(IDTENTRY_MAXIMUM_COUNT-1)]
#define GDTENTRY_MAXIMUM_COUNT                              64 // GDT[0..(GDTENTRY_MAXIMUM_COUNT-1)]

#define IDT_LIMIT                                           (IDTENTRY_MAXIMUM_COUNT * sizeof(ARCH_X64_IDTENTRY) - 1)
#define GDT_LIMIT                                           (GDTENTRY_MAXIMUM_COUNT * sizeof(U64) - 1)


//
// Kernel stack size.
//

#define KERNEL_STACK_SIZE_DEFAULT                           0x100000 // 1M


//
// Defined segments for kernel and application.
//

// 
// GDT[0] = Null selector.
// GDT[1] = 32-bit code selector for kernel.
// GDT[2] = 32-bit data selector for kernel.
// GDT[3] = 64-bit code selector for kernel.
// GDT[4] = 64-bit data selector for kernel.
// GDT[5] = 64-bit data selector for kernel (special purpose).
// GDT[6] = 64-bit data selector for kernel (special purpose).
// GDT[7] = 64-bit tss selector for kernel.
// GDT[8] = 64-bit tss base address (cannot be used).
// GDT[9] = reserved.
// GDT[10] = reserved.
// GDT[11] = reserved.
// GDT[12] = 64-bit code selector for application.
// GDT[13] = 64-bit data selector for application.
// GDT[14] = 64-bit data selector for application (special purpose).
// GDT[15] = 64-bit data selector for application (special purpose).
// GDT[16..??] = Free to use.
// 

#define NULL_XS                                             ((0 << 3) | 0) // Index=0, RPL=0
#define KERNEL_CS32                                         ((1 << 3) | 0) // Index=1, RPL=0
#define KERNEL_DS32                                         ((2 << 3) | 0) // Index=2, RPL=0
#define KERNEL_CS                                           ((3 << 3) | 0) // Index=3, RPL=0
#define KERNEL_DS                                           ((4 << 3) | 0) // Index=4, RPL=0
#define KERNEL_FS_RESERVED                                  ((5 << 3) | 0) // Index=5, RPL=0
#define KERNEL_GS_RESERVED                                  ((6 << 3) | 0) // Index=5, RPL=0
#define KERNEL_SS                                           KERNEL_DS
#define KERNEL_TSS                                          ((7 << 3) | 0) // Index=6, RPL=0
#define KERNEL_HIGH_TSS                                     ((8 << 3) | 0) // Index=6, RPL=0
#define KERNEL_FSGS                                         NULL_XS

#define USER_CS                                             ((12 << 3) | 3) // Index=7, RPL=3
#define USER_DS                                             ((13 << 3) | 3) // Index=8, RPL=3
#define USER_FS                                             ((14 << 3) | 0) // Index=5, RPL=0
#define USER_GS                                             ((15 << 3) | 3) // Index=9, RPL=3
#define USER_SS                                             USER_DS

#define SELECTOR_TO_INDEX(_selector)                        ((_selector) >> 3)


#pragma pack(push, 2)
typedef struct _ARCH_X64_XDTR
{
    U16 Limit;
    U64 Base;
} ARCH_X64_XDTR;
#pragma pack(pop)

typedef struct _KTHREAD                     KTHREAD;
typedef struct _KSCHED_CLASS                KSCHED_CLASS;

typedef struct _KPROCESSOR
{
    struct _KPROCESSOR *Self;
    U8 ProcessorId;
    U64 *Gdt;
    ARCH_X64_IDTENTRY *Idt;
    ARCH_X64_TSS *Tss;

    PVOID HalPrivateData;
    KIRQ_GROUP IrqGroups[IRQ_GROUPS_MAX];

    KTHREAD *CurrentThread;
    KSCHED_CLASS *SchedNormalClass;
} KPROCESSOR;


#define PROCESSOR_INVALID_MAPPING               0xffff


extern U16 KiApicIdToProcessorId[0x100];
extern U16 KiProcessorIdToApicId[0x100];
extern KPROCESSOR *KiProcessorBlocks[0x100];
extern U64 KiProcessorMask;
extern U8 KiProcessorCount;



U64
KERNELAPI
KeGetProcessorMask(
    VOID);

U8
KERNELAPI
KeGetProcessorCount(
    VOID);

VOID
KERNELAPI
KiInitializeProcessor(
    VOID);

U8
KERNELAPI
KeGetCurrentProcessorId(
    VOID);

KPROCESSOR *
KERNELAPI
KeGetCurrentProcessor(
    VOID);
