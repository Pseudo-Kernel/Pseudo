
/**
 * @file kprocessor.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements routines for processor structure (GDT/IDT/KPROCESSOR).
 * @version 0.1
 * @date 2021-10-24
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <base/base.h>
#include <ke/lock.h>
#include <ke/keinit.h>
#include <ke/interrupt.h>
#include <ke/kprocessor.h>
#include <mm/mm.h>
#include <mm/pool.h>
#include <init/bootgfx.h>
#include <hal/acpi.h>
#include <hal/apic.h>

extern PTR KiInterruptHandlers[0x100];
extern VIRTUAL_ADDRESS HalApicBase;

U16 KiApicIdToProcessorId[0x100]; // valid if result is in 0..255
U16 KiProcessorIdToApicId[0x100]; // valid if result is in 0..255
KPROCESSOR *KiProcessorBlocks[0x100];
U64 KiProcessorMask;
U8 KiProcessorCount;

/**
 * @brief Returns 64-bit processor mask.
 *        Each bit represents existance of processor.\n
 *        For example, 0xf (01111b) means processor 0, 1, 2, 3 is existing.\n
 * 
 * @return 64-bit processor mask.
 */
U64
KERNELAPI
KeGetProcessorMask(
    VOID)
{
    return KiProcessorMask;
}

/**
 * @brief Returns number of processors.
 * 
 * @return Number of processors.
 */
U8
KERNELAPI
KeGetProcessorCount(
    VOID)
{
    return KiProcessorCount;
}

/**
 * @brief Sets the interrupt vector.
 * 
 * @param [in] Idt          Address of IDT.
 * @param [in] Vector       Vector number.
 * @param [in] BaseOffset   Address of interrupt handler (RIP).
 * @param [in] Selector     Selector (CS).
 * @param [in] Attributes   Attributes of vector.
 * 
 * @return TRUE if succeeds, FALS otherwise.
 */
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

/**
 * @brief Sets segment descriptor for given selector.
 * 
 * @param [in] Gdt          Address of GDT.
 * @param [in] Selector     Selector.
 * @param [in] Descriptor   New descriptor.
 * 
 * @return None.
 */
VOID
KERNELAPI
KiSetSegmentDescriptor(
    IN U64 *Gdt,
    IN U16 Selector,
    IN U64 Descriptor)
{
    Gdt[SELECTOR_TO_INDEX(Selector)] = Descriptor;
}

/**
 * @brief Initializes GDT and IDT.
 * 
 * @param [in] Gdt      Address of GDT.
 * @param [in] Idt      Address of IDT.
 * 
 * @return None.
 */
VOID
KERNELAPI
KiInitializeGdtIdt(
    IN U64 *Gdt,
    IN ARCH_X64_IDTENTRY *Idt)
{
    U64 DefaultDescriptor64 = 
        ARCH_X64_SEGMENT_PRESENT | ARCH_X64_SEGMENT_CODE_DATA | ARCH_X64_SEGMENT_GRANULARITY;
    U64 DefaultCodeDescriptor64 = DefaultDescriptor64 | ARCH_X64_SEGMENT_64BIT | 
        ARCH_X64_SEGMENT_TYPE(ARCH_X64_SEGMENT_TYPE_CODE_EXECUTE_READ);
    U64 DefaultDataDescriptor64 = DefaultDescriptor64 | ARCH_X64_SEGMENT_DEFAULT | 
        ARCH_X64_SEGMENT_TYPE(ARCH_X64_SEGMENT_TYPE_DATA_READWRITE);

    for (U16 i = 0; i < GDTENTRY_MAXIMUM_COUNT; i++)
    {
        Gdt[i] = 0;
    }

    KiSetSegmentDescriptor(Gdt, NULL_XS, 0);
    KiSetSegmentDescriptor(Gdt, KERNEL_CS32, 0); // currently not used
    KiSetSegmentDescriptor(Gdt, KERNEL_DS32, 0); // currently not used

    KiSetSegmentDescriptor(Gdt, KERNEL_CS, DefaultCodeDescriptor64 | ARCH_X64_SEGMENT_DPL(0));
    KiSetSegmentDescriptor(Gdt, KERNEL_DS, DefaultDataDescriptor64 | ARCH_X64_SEGMENT_DPL(0));
    KiSetSegmentDescriptor(Gdt, KERNEL_FS_RESERVED, DefaultDataDescriptor64 | ARCH_X64_SEGMENT_DPL(0));
    KiSetSegmentDescriptor(Gdt, KERNEL_GS_RESERVED, DefaultDataDescriptor64 | ARCH_X64_SEGMENT_DPL(0));

    KiSetSegmentDescriptor(Gdt, USER_CS, DefaultCodeDescriptor64 | ARCH_X64_SEGMENT_DPL(3));
    KiSetSegmentDescriptor(Gdt, USER_DS, DefaultDataDescriptor64 | ARCH_X64_SEGMENT_DPL(3));
    KiSetSegmentDescriptor(Gdt, USER_FS, DefaultDataDescriptor64 | ARCH_X64_SEGMENT_DPL(3));
    KiSetSegmentDescriptor(Gdt, USER_GS, DefaultDataDescriptor64 | ARCH_X64_SEGMENT_DPL(3));

    for (U16 Vector = 0; Vector < IDTENTRY_MAXIMUM_COUNT; Vector++)
    {
        // 0x00..0x1f -> Interrupt (RFLAGS.IF will be automatically cleared when handler is called)
        U16 Type = ARCH_X64_IDTENTRY_ATTRIBUTE_TYPE_INTERRUPT;

        if (Vector >= 0x20)
        {
            // For 0x20..0xff, RFLAGS.IF will not be changed when handler is called
            Type = ARCH_X64_IDTENTRY_ATTRIBUTE_TYPE_TRAP;
        }

        if (!KiSetInterruptVector(Idt, Vector, (U64)KiInterruptHandlers[Vector], KERNEL_CS, 
            ARCH_X64_IDTENTRY_ATTRIBUTE_PRESENT |
            ARCH_X64_IDTENTRY_ATTRIBUTE_TYPE(Type) | 
            ARCH_X64_IDTENTRY_ATTRIBUTE_DPL(0) |
            ARCH_X64_IDTENTRY_ATTRIBUTE_IST(0)))
        {
            FATAL("Failed to set interrupt vector 0x%04hx. (bogus vector number?)", Vector);
        }
    }
}

/**
 * @brief Loads GDT and IDT.
 * 
 * @param [in] Gdt          Address of GDT.
 * @param [in] GdtLimit     (Size of GDT) - 1.
 * @param [in] Idt          Address of IDT.
 * @param [in] IdtLimit     (Size of IDT) - 1.
 * 
 * @return None.
 */
VOID
KERNELAPI
KiLoadGdtIdt(
    IN U64 *Gdt,
    IN U16 GdtLimit,
    IN ARCH_X64_IDTENTRY *Idt,
    IN U16 IdtLimit)
{
    ARCH_X64_XDTR Gdtr = { .Base = (U64)Gdt, .Limit = GdtLimit, };
    ARCH_X64_XDTR Idtr = { .Base = (U64)Idt, .Limit = IdtLimit, };

    __lgdt(&Gdtr);
    __lidt(&Idtr);
}

/**
 * @brief Reloads all segment registers.\n
 *        CS, DS, ES, FS, GS, SS.
 */
VOID
__attribute__((naked, noinline))
KiReloadSegments(
    VOID)
{
    __asm__ __volatile__ (
        "mov eax, %0\n\t"
        "mov ds, ax\n\t"
        "mov es, ax\n\t"
        "mov eax, %1\n\t"
        "mov fs, ax\n\t"
        "mov eax, %1\n\t"
        "mov gs, ax\n\t"
        "mov eax, %2\n\t"
        "mov ss, ax\n\t"
        "pop rax\n\t"
        "push %3\n\t"
        "push rax\n\t"

        //
        // NOTE: default operation size of retf is 32-bit even in the long mode.
        //       use lretq (or rex.W retf) instead.
        //

        "lretq\n\t" // same as rex.W retf
        :
        : "i"(KERNEL_DS), "i"(KERNEL_FSGS), "i"(KERNEL_SS), 
          "i"(KERNEL_CS)
        : "rax", "memory"
    );
}

/**
 * @brief Initializes the processor.
 * 
 * @return None.
 */
VOID
KERNELAPI
KiInitializeProcessor(
    VOID)
{
    // CD=0 | NW=0 | WP=1 | NE=1 | EM=0 | MP=1 | TS=0
    U64 Value = __readcr0();
    Value |= (ARCH_X64_CR0_WP | ARCH_X64_CR0_NE | ARCH_X64_CR0_MP);
    Value &= ~(ARCH_X64_CR0_CD | ARCH_X64_CR0_NW | ARCH_X64_CR0_EM | ARCH_X64_CR0_TS);
    __writecr0(Value);

    // MCE=1 | PGE=1 | OSFXSR=1 | OSXMMEXCPT=1 | TSD=0
    Value = __readcr4();
    Value |= (ARCH_X64_CR4_MCE | ARCH_X64_CR4_PGE | ARCH_X64_CR4_OSFXSR | ARCH_X64_CR4_OSXMMEXCPT);
    Value &= ~ARCH_X64_CR4_TSD;
    __writecr4(Value);


    //
    // Initialize the PAT.
    //

    U64 OldPAT = __readmsr(IA32_PAT);
    U64 NewPAT = 
        PAT_MSR_ENTRY(PAT_INDEX_WRITE_BACK, PAT_MSR_MEMORY_TYPE_WRITE_BACK) |
        PAT_MSR_ENTRY(PAT_INDEX_WRITE_THROUGH, PAT_MSR_MEMORY_TYPE_WRITE_THROUGH) |
        PAT_MSR_ENTRY(PAT_INDEX_UNCACHED, PAT_MSR_MEMORY_TYPE_UNCACHED) |
        PAT_MSR_ENTRY(PAT_INDEX_UNCACHABLE, PAT_MSR_MEMORY_TYPE_UNCACHABLE) |
        PAT_MSR_ENTRY(PAT_INDEX_WRITE_PROTECTED, PAT_MSR_MEMORY_TYPE_WRITE_PROTECTED) |
        PAT_MSR_ENTRY(PAT_INDEX_WRITE_COMBINING, PAT_MSR_MEMORY_TYPE_WRITE_COMBINING);

    DbgTraceF(TraceLevelDebug, "Setting PAT 0x%016llx to 0x%016llx\n", OldPAT, NewPAT);
    __writemsr(IA32_PAT, NewPAT);



    KPROCESSOR *Processor = MmAllocatePool(PoolTypeNonPaged, sizeof(*Processor), 0x10, 0);
    if (!Processor)
    {
        FATAL("Failed to allocate processor structure");
    }

    //
    // Initialize GDT and IDT.
    //

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

    KiInitializeGdtIdt(Gdt, Idt);

    //
    // Initialize TSS with 7 interrupt stacks.
    //

    ARCH_X64_TSS *Tss = MmAllocatePool(PoolTypeNonPaged, sizeof(ARCH_X64_TSS), 0x10, 0);
    SIZE_T TssLimit = sizeof(*Tss) - 1;
    if (!Tss)
    {
        FATAL("Failed to allocate TSS");
    }

    PTR Ist[7] = { 0 }, Rsp0 = 0;
    SIZE_T StackSize = 0x20000;
    Rsp0 = (PTR)MmAllocatePool(PoolTypeNonPaged, StackSize, 0x10, 0);
    if (!Rsp0)
    {
        FATAL("Failed to initialize TSS (RSP0)");
    }

    for (INT i = 0; i < COUNTOF(Ist); i++)
    {
        Ist[i] = (PTR)MmAllocatePool(PoolTypeNonPaged, StackSize, 0x10, 0);
        if (!Ist[i])
        {
            FATAL("Failed to initialize TSS (IST)");
        }
    }

    memset(Tss, 0, sizeof(*Tss));
    Tss->Rsp0 = Rsp0 + StackSize - sizeof(U64) * 2;
    Tss->Ist1 = Ist[0] + StackSize - sizeof(U64) * 2;
    Tss->Ist2 = Ist[1] + StackSize - sizeof(U64) * 2;
    Tss->Ist3 = Ist[2] + StackSize - sizeof(U64) * 2;
    Tss->Ist4 = Ist[3] + StackSize - sizeof(U64) * 2;
    Tss->Ist5 = Ist[4] + StackSize - sizeof(U64) * 2;
    Tss->Ist6 = Ist[5] + StackSize - sizeof(U64) * 2;
    Tss->Ist7 = Ist[6] + StackSize - sizeof(U64) * 2;
    Tss->IoMapBase = sizeof(Tss); // disable I/O permission

    // 
    // Setup TSS to the GDT.
    // Note that TSS(or LDT) descriptor takes 16-byte.
    // 

    // TSS selector.
    KiSetSegmentDescriptor(Gdt, KERNEL_TSS, 
        ARCH_X64_SEGMENT_PRESENT | ARCH_X64_SEGMENT_TYPE(ARCH_X64_SEGMENT_TYPE_SYSTEM_TSS) | ARCH_X64_SEGMENT_DPL(0) | 
        (((U64)Tss & 0xffffff) << 0x10) | (((U64)Tss & 0xff0000) << 0x20) | 
        ((U64)TssLimit & 0xffff) | (((U64)TssLimit & 0xf0000) << 0x20));

    // Higher 32-bit offset.
    KiSetSegmentDescriptor(Gdt, KERNEL_HIGH_TSS, (((U64)Tss >> 0x20) & 0xffffffff));


    //
    // Load GDTR, IDTR, and TR.
    //

    // 
    // The operating system must create at least one 64-bit TSS after activating IA-32e mode. 
    // It must execute the LTR instruction (in 64-bit mode) to load the TR register with a pointer
    // to the 64-bit TSS responsible for both 64-bit mode programs and compatibility-mode programs.
    //
    // [From: Intel SDM. 7.7 TASK MANAGEMENT IN 64-BIT MODE]
    // 

    KiLoadGdtIdt(Gdt, GDT_LIMIT, Idt, IDT_LIMIT);
    __ltr(KERNEL_TSS);


    //
    // Load new segment registers.
    //

    KiReloadSegments();
    _writegsbase_u64((U64)Processor);

    U8 ProcessorId = KiProcessorCount;
    U8 ApicId = HalApicGetId(HalApicBase);

    KiApicIdToProcessorId[ApicId] = ProcessorId;
    KiProcessorIdToApicId[ProcessorId] = ApicId;

    Processor->Self = Processor;
    Processor->Idt = Idt;
    Processor->Gdt = Gdt;
    Processor->Tss = Tss;
    Processor->ProcessorId = ProcessorId;
    Processor->HalPrivateData = NULL;

    KiProcessorBlocks[ProcessorId] = Processor;
    KiProcessorMask |= (1 << ProcessorId);
    KiProcessorCount++;
}

/**
 * @brief Returns current processor id.
 * 
 * @return Current processor id.
 */
U8
KERNELAPI
KeGetCurrentProcessorId(
    VOID)
{
    U8 ApicId = HalApicGetId(HalApicBase);
    U16 ProcessorId = KiApicIdToProcessorId[ApicId];
    
    DASSERT(ProcessorId < 0x100);

    return (U8)ProcessorId;
}

/**
 * @brief Returns current processor block.
 * 
 * @return Current processor block.
 */
KPROCESSOR *
KERNELAPI
KeGetCurrentProcessor(
    VOID)
{
#if 1
    // Currently not use IA32_GS_BASE/IA32_KERNEL_GS_BASE
    U8 ProcessorId = KeGetCurrentProcessorId();

    return KiProcessorBlocks[ProcessorId];
#else
    //
    // Using IA32_GS_BASE is not simple because loading gs clears IA32_GS_BASE...
    // 1. Test whether the SWAPGS need to be executed in interrupt handler/syscall entry
    // 2. Take care of nested interrupts
    //

    KPROCESSOR *Processor = NULL;
    ULONG Offset = FIELD_OFFSET(KPROCESSOR, Self);

    __asm__ __volatile__ (
        "mov %0, qword ptr gs:[%1]\n\t"
        : "=a"(Processor)
        : "i"(Offset)
        :
    );

    return Processor;
#endif
}

