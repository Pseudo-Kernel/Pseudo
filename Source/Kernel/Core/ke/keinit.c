
/**
 * @file keinit.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements kernel initialization routines.
 * @version 0.1
 * @date 2021-09-26
 * 
 * @copyright Copyright (c) 2021
 * 
 * @todo Initialize GDT/IDT/TSS (per processor)
 */

#include <base/base.h>
#include <ke/lock.h>
#include <ke/keinit.h>
#include <mm/mm.h>
#include <mm/pool.h>
#include <init/bootgfx.h>

#pragma pack(push, 2)
typedef struct _ARCH_X64_XDTR
{
    U16 Limit;
    U64 Base;
} ARCH_X64_XDTR;
#pragma pack(pop)

typedef struct _KPROCESSOR
{
    struct _KPROCESSOR *Self;

    U64 *Gdt;
    ARCH_X64_IDTENTRY *Idt;
    ARCH_X64_TSS *Tss;

    U8 ProcessorId;

    //PVOID CurrentThread;
} KPROCESSOR;


__attribute__((naked))
VOID
KiInterruptNop(
    VOID)
{
    /*
        6.14 EXCEPTION AND INTERRUPT HANDLING IN 64-BIT MODE

        In 64-bit mode, interrupt and exception handling is similar to what has been described for non-64-bit modes. The
        following are the exceptions:
        - All interrupt handlers pointed by the IDT are in 64-bit code (this does not apply to the SMI handler).
        - The size of interrupt-stack pushes is fixed at 64 bits; and the processor uses 8-byte, zero extended stores.
        - The stack pointer (SS:RSP) is pushed unconditionally on interrupts. In legacy modes, this push is conditional
        and based on a change in current privilege level (CPL).
        - The new SS is set to NULL if there is a change in CPL.
        - IRET behavior changes.
        - There is a new interrupt stack-switch mechanism and a new interrupt shadow stack-switch mechanism.
        - The alignment of interrupt stack frame is different.    
    */

    __asm__ __volatile__ (
        "push fs\n\t"
        "push gs\n\t"
        "push rax\n\t"
        "push rcx\n\t"
        "push rdx\n\t"
        "push rbx\n\t"
        "push rbp\n\t"
        "push rsi\n\t"
        "push rdi\n\t"
        "push r9\n\t"
        "push r10\n\t"
        "push r11\n\t"
        "push r12\n\t"
        "push r13\n\t"
        "push r14\n\t"
        "push r15\n\t"
        "cli\n\t"
        "hlt\n\t" // Halts the system
        "pop r15\n\t"
        "pop r14\n\t"
        "pop r13\n\t"
        "pop r12\n\t"
        "pop r11\n\t"
        "pop r10\n\t"
        "pop r9\n\t"
        "pop rdi\n\t"
        "pop rsi\n\t"
        "pop rbp\n\t"
        "pop rbx\n\t"
        "pop rdx\n\t"
        "pop rcx\n\t"
        "pop rax\n\t"
        "pop gs\n\t"
        "pop fs\n\t"
        "iretq\n\t"
        :
        :
        : "memory"
    );
}

BOOLEAN
KERNELAPI
KiSetInterruptVector(
    IN ARCH_X64_IDTENTRY *Idt,
    IN U16 Vector,
    IN U64 BaseOffset,
    IN U16 Selector,
    IN U16 Attributes)
{
    UPTR Offset64 = (UPTR)BaseOffset;

    if (Vector >= 0x100)
        return FALSE;

    Idt[Vector] = (ARCH_X64_IDTENTRY) {
        .Reserved = 0, 
        .Selector = Selector, 
        .OffsetLow = (U16)((Offset64 & 0x000000000000ffffULL) >> 0x00),
        .OffsetMid = (U16)((Offset64 & 0x00000000ffff0000ULL) >> 0x10),
        .OffsetHigh = (U32)((Offset64 & 0xffffffff00000000ULL) >> 0x20),
        .Attributes = Attributes, 
    };

    return TRUE;
}

VOID
KERNELAPI
KiSetSegmentDescriptor(
    IN U64 *Gdt,
    IN U16 Selector,
    IN U64 Descriptor)
{
    Gdt[SELECTOR_TO_INDEX(Selector)] = Descriptor;
}

VOID
KERNELAPI
KiInitializeGdtIdt(
    IN U64 *Gdt,
    IN ARCH_X64_IDTENTRY *Idt)
{
    U64 DefaultDescriptor64 = 
        ARCH_X64_SEGMENT_PRESENT | ARCH_X64_SEGMENT_64BIT | 
        ARCH_X64_SEGMENT_DEFAULT | ARCH_X64_SEGMENT_GRANULARITY;

    KiSetSegmentDescriptor(Gdt, NULL_XS, 0);
    KiSetSegmentDescriptor(Gdt, KERNEL_CS32, 0);
    KiSetSegmentDescriptor(Gdt, KERNEL_DS32, 0);

    KiSetSegmentDescriptor(Gdt, KERNEL_CS, 
        DefaultDescriptor64 | ARCH_X64_SEGMENT_DPL(0) | 
        ARCH_X64_SEGMENT_TYPE(ARCH_X64_SEGMENT_TYPE_CODE_EXECUTE_READ));
    KiSetSegmentDescriptor(Gdt, KERNEL_DS,
        DefaultDescriptor64 | ARCH_X64_SEGMENT_DPL(0) |
        ARCH_X64_SEGMENT_TYPE(ARCH_X64_SEGMENT_TYPE_DATA_READWRITE));
    KiSetSegmentDescriptor(Gdt, KERNEL_GS,
        DefaultDescriptor64 | ARCH_X64_SEGMENT_DPL(0) |
        ARCH_X64_SEGMENT_TYPE(ARCH_X64_SEGMENT_TYPE_DATA_READWRITE));

    KiSetSegmentDescriptor(Gdt, USER_CS,
        DefaultDescriptor64 | ARCH_X64_SEGMENT_DPL(3) |
        ARCH_X64_SEGMENT_TYPE(ARCH_X64_SEGMENT_TYPE_CODE_EXECUTE_READ));
    KiSetSegmentDescriptor(Gdt, USER_DS,
        DefaultDescriptor64 | ARCH_X64_SEGMENT_DPL(3) |
        ARCH_X64_SEGMENT_TYPE(ARCH_X64_SEGMENT_TYPE_DATA_READWRITE));
    KiSetSegmentDescriptor(Gdt, USER_GS,
        DefaultDescriptor64 | ARCH_X64_SEGMENT_DPL(3) |
        ARCH_X64_SEGMENT_TYPE(ARCH_X64_SEGMENT_TYPE_DATA_READWRITE));

    for (U16 Vector = 0; Vector < IDTENTRY_MAXIMUM_COUNT; Vector++)
    {
        if (!KiSetInterruptVector(Idt, Vector, (U64)&KiInterruptNop, KERNEL_CS, 
            ARCH_X64_IDTENTRY_ATTRIBUTE_PRESENT |
            ARCH_X64_IDTENTRY_ATTRIBUTE_TYPE(ARCH_X64_IDTENTRY_ATTRIBUTE_TYPE_INTERRUPT) | 
            ARCH_X64_IDTENTRY_ATTRIBUTE_DPL(0) |
            ARCH_X64_IDTENTRY_ATTRIBUTE_IST(0)))
        {
            FATAL("Failed to set interrupt vector 0x%04hx. (bogus vector number?)", Vector);
        }
    }
}

VOID
KiLoadGdtIdt(
    IN U64 *Gdt,
    IN U16 GdtLimit,
    IN ARCH_X64_IDTENTRY *Idt,
    IN U16 IdtLimit)
{
    ARCH_X64_XDTR Gdtr = { .Base = (U64)Gdt, .Limit = GdtLimit, };
    ARCH_X64_XDTR Idtr = { .Base = (U64)Idt, .Limit = IdtLimit, };

    __asm__ __volatile__ (
        "lgdt [%0]\n\t"
        "lidt [%1]\n\t"
        :
        : "r"(&Gdtr), "r"(&Idtr)
        : "memory"
    );
}

VOID
KERNELAPI
KiInitializeProcessor(
    VOID)
{
    KPROCESSOR *Processor = MmAllocatePool(PoolTypeNonPaged, sizeof(*Processor), 0x10, 0);
    if (!Processor)
    {
        FATAL("Failed to allocate processor structure");
    }

    ARCH_X64_IDTENTRY *Idt = MmAllocatePool(PoolTypeNonPaged, IDT_LIMIT + 1, 0x10, 0);
    if (!Idt)
    {
        FATAL("Failed to allocate IDT");
    }

    U64 *Gdt = MmAllocatePool(PoolTypeNonPaged, GDT_LIMIT + 1, 0x10, 0);
    if (!Gdt)
    {
        FATAL("Failed to allocate GDT");
    }

//    ARCH_X64_TSS *Tss = MmAllocatePool(PoolTypeNonPaged, sizeof(ARCH_X64_TSS), 0x10, 0);
//    if (!Tss)
//    {
//        FATAL("Failed to allocate TSS");
//    }
//
//    Tss->IoMapBase = sizeof(Tss);


    KiInitializeGdtIdt(Gdt, Idt);
    KiLoadGdtIdt(Gdt, GDT_LIMIT, Idt, IDT_LIMIT);

    //
    // Load new segment registers.
    //

    __asm__ __volatile__ (
        "mov cs, %0\n\t"
        "mov ds, %1\n\t"
        "mov es, %2\n\t"
        "mov fs, %3\n\t"
        "mov ss, %4\n\t"
        :
        : "r"(KERNEL_CS), "r"(KERNEL_DS), "r"(KERNEL_DS), "r"(KERNEL_DS), "r"(KERNEL_SS)
        : "memory"
    );


    // 
    // The operating system must create at least one 64-bit TSS after activating IA-32e mode. 
    // It must execute the LTR instruction (in 64-bit mode) to load the TR register with a pointer
    // to the 64-bit TSS responsible for both 64-bit mode programs and compatibility-mode programs.
    //
    // [From: Intel SDM. 7.7 TASK MANAGEMENT IN 64-BIT MODE]
    // 
    

    Processor->Self = Processor;
    Processor->Idt = Idt;
    Processor->Gdt = Gdt;
    Processor->Tss = NULL;
    Processor->ProcessorId = -1;
}

VOID
KERNELAPI
KiInitialize(
    VOID)
{

}

