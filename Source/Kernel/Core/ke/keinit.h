
#pragma once


//
// Bits for CR0.
//

#define ARCH_X64_CR0_MP                                     (1 << 1)
#define ARCH_X64_CR0_EM                                     (1 << 2)
#define ARCH_X64_CR0_TS                                     (1 << 3)
#define ARCH_X64_CR0_NE                                     (1 << 5)
#define ARCH_X64_CR0_WP                                     (1 << 16)
#define ARCH_X64_CR0_NW                                     (1 << 29)
#define ARCH_X64_CR0_CD                                     (1 << 30)

// CD=0 | NW=0 | WP=1 | NE=1 | EM=0 | MP=1 | TS=0
// MCE=1 | PGE=1 | OSFXSR=1 | OSXMMEXCPT=1 | TSD=0


//
// Bits for CR4.
//

#define ARCH_X64_CR4_TSD                                    (1 << 2)
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

//#define ARCH_X64_IDTENTRY_ATTRIBUTE_TYPE_TASK               5 // 0101b
#define ARCH_X64_IDTENTRY_ATTRIBUTE_TYPE_INTERRUPT          14 // 1110b
#define ARCH_X64_IDTENTRY_ATTRIBUTE_TYPE_TRAP               15 // 1111b

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
#define KERNEL_FS                                           ((5 << 3) | 0) // Index=5, RPL=0
#define KERNEL_GS                                           ((6 << 3) | 0) // Index=5, RPL=0
#define KERNEL_SS                                           KERNEL_DS
#define KERNEL_TSS                                          ((7 << 3) | 0) // Index=6, RPL=0
#define KERNEL_HIGH_TSS                                     ((8 << 3) | 0) // Index=6, RPL=0

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

typedef struct _KPROCESSOR
{
    struct _KPROCESSOR *Self;

    U64 *Gdt;
    ARCH_X64_IDTENTRY *Idt;
    ARCH_X64_TSS *Tss;

    U8 ProcessorId;

    //PVOID CurrentThread;
} KPROCESSOR;



//
// Interrupt handlers.
//

typedef struct _KSTACK_FRAME_INTERRUPT
{
    U64 R15;
    U64 R14;
    U64 R13;
    U64 R12;
    U64 R11;
    U64 R10;
    U64 R9;
    U64 R8;
    U64 Rdi;
    U64 Rsi;
    U64 Rbp;
    U64 Rbx;
    U64 Rdx;
    U64 Rcx;
    U64 Rax;
    U64 Gs;
    U64 Fs;
    U8 Redzone[128];

    U64 ErrorCode;
    U64 Rip;
    U64 Cs;
    U64 Rflags;
    U64 Rsp;
    U64 Ss;
} KSTACK_FRAME_INTERRUPT;

#define INTERRUPT_HANDLER_NAME(_vector)     \
    KiInterruptHandler_Vector ##_vector

#define DECLARE_INTERRUPT_HANDLER(_vector)  \
    __attribute__((naked)) VOID     \
    INTERRUPT_HANDLER_NAME(_vector) (VOID)

/* todo: make sure that rsp is 16-byte aligned after interrupt frame is pushed */
#define ASM_INTERRUPT_FRAME_PUSH_ERRCODE    \
    "push 0\n\t" /* error code placeholder */   \
    "push fs\n\t"   \
    "push gs\n\t"   \
    "push rax\n\t"  \
    "push rcx\n\t"  \
    "push rdx\n\t"  \
    "push rbx\n\t"  \
    "push rbp\n\t"  \
    "push rsi\n\t"  \
    "push rdi\n\t"  \
    "push r8\n\t"   \
    "push r9\n\t"   \
    "push r10\n\t"  \
    "push r11\n\t"  \
    "push r12\n\t"  \
    "push r13\n\t"  \
    "push r14\n\t"  \
    "push r15\n\t"  \
    "sub rsp, 0x88\n\t" /* red zone size + 8 byte */

#define ASM_INTERRUPT_FRAME_POP_ERRCODE    \
    "add rsp, 0x88\n\t"  /* red zone size + 8 byte */  \
    "pop r15\n\t"   \
    "pop r14\n\t"   \
    "pop r13\n\t"   \
    "pop r12\n\t"   \
    "pop r11\n\t"   \
    "pop r10\n\t"   \
    "pop r9\n\t"    \
    "pop r8\n\t"    \
    "pop rdi\n\t"   \
    "pop rsi\n\t"   \
    "pop rbp\n\t"   \
    "pop rbx\n\t"   \
    "pop rdx\n\t"   \
    "pop rcx\n\t"   \
    "pop rax\n\t"   \
    "pop gs\n\t"    \
    "pop fs\n\t"    \
    "add rsp, 0x08\n\t" /* pop error code */

#define ASM_INTERRUPT_FRAME_PUSH    \
    "sub rsp, 0x88\n\t" /* red zone size + 8 byte */ \
    "push fs\n\t"   \
    "push gs\n\t"   \
    "push rax\n\t"  \
    "push rcx\n\t"  \
    "push rdx\n\t"  \
    "push rbx\n\t"  \
    "push rbp\n\t"  \
    "push rsi\n\t"  \
    "push rdi\n\t"  \
    "push r8\n\t"   \
    "push r9\n\t"   \
    "push r10\n\t"  \
    "push r11\n\t"  \
    "push r12\n\t"  \
    "push r13\n\t"  \
    "push r14\n\t"  \
    "push r15\n\t" /* todo: make sure that rsp is 16-byte aligned after r15 push */

#define ASM_INTERRUPT_FRAME_POP    \
    "pop r15\n\t"   \
    "pop r14\n\t"   \
    "pop r13\n\t"   \
    "pop r12\n\t"   \
    "pop r11\n\t"   \
    "pop r10\n\t"   \
    "pop r9\n\t"    \
    "pop r8\n\t"    \
    "pop rdi\n\t"   \
    "pop rsi\n\t"   \
    "pop rbp\n\t"   \
    "pop rbx\n\t"   \
    "pop rdx\n\t"   \
    "pop rcx\n\t"   \
    "pop rax\n\t"   \
    "pop gs\n\t"    \
    "pop fs\n\t"    \
    "add rsp, 0x88\n\t"  /* red zone size + 8 byte */


/*
    In IA-32e mode, the RSP is aligned to a 16-byte boundary before pushing the stack frame. The stack
    frame itself is aligned on a 16-byte boundary when the interrupt handler is called.

    [FROM. Intel SDM, 6.14.2 64-Bit Mode Stack Frame]

    | ...     | (???) <- RSP before transfer to handler
    | ...     | +0x00 <- RSP before pushing stack frame (aligned to 16-byte boundary)
    | SS      | +0x08
    | RSP     | +0x10
    | RFLAGS  | +0x18
    | CS      | +0x20
    | RIP     | +0x28
    | ERRCODE | +0x30 <- RSP after interrupt
*/

#define INTERRUPT_HANDLER(_vector)  \
    __attribute__((naked)) VOID     \
    INTERRUPT_HANDLER_NAME(_vector) (VOID) {\
    __asm__ __volatile__ (  \
        ASM_INTERRUPT_FRAME_PUSH_ERRCODE    \
        "mov rdi, %0\n\t" /* sysvabi uses rdi for 1st argument */   \
        "mov rcx, %0\n\t" /* msabi uses rcx for 1st argument */     \
        "call KiCallInterruptChain\n\t"     \
        ASM_INTERRUPT_FRAME_POP_ERRCODE     \
        "iretq\n\t"         \
        :                   \
        : "i"((_vector))    \
        : "memory"          \
    );                      \
}



DECLARE_INTERRUPT_HANDLER(32);
DECLARE_INTERRUPT_HANDLER(33);
DECLARE_INTERRUPT_HANDLER(34);
DECLARE_INTERRUPT_HANDLER(35);
DECLARE_INTERRUPT_HANDLER(36);
DECLARE_INTERRUPT_HANDLER(37);
DECLARE_INTERRUPT_HANDLER(38);
DECLARE_INTERRUPT_HANDLER(39);
DECLARE_INTERRUPT_HANDLER(40);
DECLARE_INTERRUPT_HANDLER(41);
DECLARE_INTERRUPT_HANDLER(42);
DECLARE_INTERRUPT_HANDLER(43);
DECLARE_INTERRUPT_HANDLER(44);
DECLARE_INTERRUPT_HANDLER(45);
DECLARE_INTERRUPT_HANDLER(46);
DECLARE_INTERRUPT_HANDLER(47);
DECLARE_INTERRUPT_HANDLER(48);
DECLARE_INTERRUPT_HANDLER(49);
DECLARE_INTERRUPT_HANDLER(50);
DECLARE_INTERRUPT_HANDLER(51);
DECLARE_INTERRUPT_HANDLER(52);
DECLARE_INTERRUPT_HANDLER(53);
DECLARE_INTERRUPT_HANDLER(54);
DECLARE_INTERRUPT_HANDLER(55);
DECLARE_INTERRUPT_HANDLER(56);
DECLARE_INTERRUPT_HANDLER(57);
DECLARE_INTERRUPT_HANDLER(58);
DECLARE_INTERRUPT_HANDLER(59);
DECLARE_INTERRUPT_HANDLER(60);
DECLARE_INTERRUPT_HANDLER(61);
DECLARE_INTERRUPT_HANDLER(62);
DECLARE_INTERRUPT_HANDLER(63);
DECLARE_INTERRUPT_HANDLER(64);
DECLARE_INTERRUPT_HANDLER(65);
DECLARE_INTERRUPT_HANDLER(66);
DECLARE_INTERRUPT_HANDLER(67);
DECLARE_INTERRUPT_HANDLER(68);
DECLARE_INTERRUPT_HANDLER(69);
DECLARE_INTERRUPT_HANDLER(70);
DECLARE_INTERRUPT_HANDLER(71);
DECLARE_INTERRUPT_HANDLER(72);
DECLARE_INTERRUPT_HANDLER(73);
DECLARE_INTERRUPT_HANDLER(74);
DECLARE_INTERRUPT_HANDLER(75);
DECLARE_INTERRUPT_HANDLER(76);
DECLARE_INTERRUPT_HANDLER(77);
DECLARE_INTERRUPT_HANDLER(78);
DECLARE_INTERRUPT_HANDLER(79);
DECLARE_INTERRUPT_HANDLER(80);
DECLARE_INTERRUPT_HANDLER(81);
DECLARE_INTERRUPT_HANDLER(82);
DECLARE_INTERRUPT_HANDLER(83);
DECLARE_INTERRUPT_HANDLER(84);
DECLARE_INTERRUPT_HANDLER(85);
DECLARE_INTERRUPT_HANDLER(86);
DECLARE_INTERRUPT_HANDLER(87);
DECLARE_INTERRUPT_HANDLER(88);
DECLARE_INTERRUPT_HANDLER(89);
DECLARE_INTERRUPT_HANDLER(90);
DECLARE_INTERRUPT_HANDLER(91);
DECLARE_INTERRUPT_HANDLER(92);
DECLARE_INTERRUPT_HANDLER(93);
DECLARE_INTERRUPT_HANDLER(94);
DECLARE_INTERRUPT_HANDLER(95);
DECLARE_INTERRUPT_HANDLER(96);
DECLARE_INTERRUPT_HANDLER(97);
DECLARE_INTERRUPT_HANDLER(98);
DECLARE_INTERRUPT_HANDLER(99);
DECLARE_INTERRUPT_HANDLER(100);
DECLARE_INTERRUPT_HANDLER(101);
DECLARE_INTERRUPT_HANDLER(102);
DECLARE_INTERRUPT_HANDLER(103);
DECLARE_INTERRUPT_HANDLER(104);
DECLARE_INTERRUPT_HANDLER(105);
DECLARE_INTERRUPT_HANDLER(106);
DECLARE_INTERRUPT_HANDLER(107);
DECLARE_INTERRUPT_HANDLER(108);
DECLARE_INTERRUPT_HANDLER(109);
DECLARE_INTERRUPT_HANDLER(110);
DECLARE_INTERRUPT_HANDLER(111);
DECLARE_INTERRUPT_HANDLER(112);
DECLARE_INTERRUPT_HANDLER(113);
DECLARE_INTERRUPT_HANDLER(114);
DECLARE_INTERRUPT_HANDLER(115);
DECLARE_INTERRUPT_HANDLER(116);
DECLARE_INTERRUPT_HANDLER(117);
DECLARE_INTERRUPT_HANDLER(118);
DECLARE_INTERRUPT_HANDLER(119);
DECLARE_INTERRUPT_HANDLER(120);
DECLARE_INTERRUPT_HANDLER(121);
DECLARE_INTERRUPT_HANDLER(122);
DECLARE_INTERRUPT_HANDLER(123);
DECLARE_INTERRUPT_HANDLER(124);
DECLARE_INTERRUPT_HANDLER(125);
DECLARE_INTERRUPT_HANDLER(126);
DECLARE_INTERRUPT_HANDLER(127);
DECLARE_INTERRUPT_HANDLER(128);
DECLARE_INTERRUPT_HANDLER(129);
DECLARE_INTERRUPT_HANDLER(130);
DECLARE_INTERRUPT_HANDLER(131);
DECLARE_INTERRUPT_HANDLER(132);
DECLARE_INTERRUPT_HANDLER(133);
DECLARE_INTERRUPT_HANDLER(134);
DECLARE_INTERRUPT_HANDLER(135);
DECLARE_INTERRUPT_HANDLER(136);
DECLARE_INTERRUPT_HANDLER(137);
DECLARE_INTERRUPT_HANDLER(138);
DECLARE_INTERRUPT_HANDLER(139);
DECLARE_INTERRUPT_HANDLER(140);
DECLARE_INTERRUPT_HANDLER(141);
DECLARE_INTERRUPT_HANDLER(142);
DECLARE_INTERRUPT_HANDLER(143);
DECLARE_INTERRUPT_HANDLER(144);
DECLARE_INTERRUPT_HANDLER(145);
DECLARE_INTERRUPT_HANDLER(146);
DECLARE_INTERRUPT_HANDLER(147);
DECLARE_INTERRUPT_HANDLER(148);
DECLARE_INTERRUPT_HANDLER(149);
DECLARE_INTERRUPT_HANDLER(150);
DECLARE_INTERRUPT_HANDLER(151);
DECLARE_INTERRUPT_HANDLER(152);
DECLARE_INTERRUPT_HANDLER(153);
DECLARE_INTERRUPT_HANDLER(154);
DECLARE_INTERRUPT_HANDLER(155);
DECLARE_INTERRUPT_HANDLER(156);
DECLARE_INTERRUPT_HANDLER(157);
DECLARE_INTERRUPT_HANDLER(158);
DECLARE_INTERRUPT_HANDLER(159);
DECLARE_INTERRUPT_HANDLER(160);
DECLARE_INTERRUPT_HANDLER(161);
DECLARE_INTERRUPT_HANDLER(162);
DECLARE_INTERRUPT_HANDLER(163);
DECLARE_INTERRUPT_HANDLER(164);
DECLARE_INTERRUPT_HANDLER(165);
DECLARE_INTERRUPT_HANDLER(166);
DECLARE_INTERRUPT_HANDLER(167);
DECLARE_INTERRUPT_HANDLER(168);
DECLARE_INTERRUPT_HANDLER(169);
DECLARE_INTERRUPT_HANDLER(170);
DECLARE_INTERRUPT_HANDLER(171);
DECLARE_INTERRUPT_HANDLER(172);
DECLARE_INTERRUPT_HANDLER(173);
DECLARE_INTERRUPT_HANDLER(174);
DECLARE_INTERRUPT_HANDLER(175);
DECLARE_INTERRUPT_HANDLER(176);
DECLARE_INTERRUPT_HANDLER(177);
DECLARE_INTERRUPT_HANDLER(178);
DECLARE_INTERRUPT_HANDLER(179);
DECLARE_INTERRUPT_HANDLER(180);
DECLARE_INTERRUPT_HANDLER(181);
DECLARE_INTERRUPT_HANDLER(182);
DECLARE_INTERRUPT_HANDLER(183);
DECLARE_INTERRUPT_HANDLER(184);
DECLARE_INTERRUPT_HANDLER(185);
DECLARE_INTERRUPT_HANDLER(186);
DECLARE_INTERRUPT_HANDLER(187);
DECLARE_INTERRUPT_HANDLER(188);
DECLARE_INTERRUPT_HANDLER(189);
DECLARE_INTERRUPT_HANDLER(190);
DECLARE_INTERRUPT_HANDLER(191);
DECLARE_INTERRUPT_HANDLER(192);
DECLARE_INTERRUPT_HANDLER(193);
DECLARE_INTERRUPT_HANDLER(194);
DECLARE_INTERRUPT_HANDLER(195);
DECLARE_INTERRUPT_HANDLER(196);
DECLARE_INTERRUPT_HANDLER(197);
DECLARE_INTERRUPT_HANDLER(198);
DECLARE_INTERRUPT_HANDLER(199);
DECLARE_INTERRUPT_HANDLER(200);
DECLARE_INTERRUPT_HANDLER(201);
DECLARE_INTERRUPT_HANDLER(202);
DECLARE_INTERRUPT_HANDLER(203);
DECLARE_INTERRUPT_HANDLER(204);
DECLARE_INTERRUPT_HANDLER(205);
DECLARE_INTERRUPT_HANDLER(206);
DECLARE_INTERRUPT_HANDLER(207);
DECLARE_INTERRUPT_HANDLER(208);
DECLARE_INTERRUPT_HANDLER(209);
DECLARE_INTERRUPT_HANDLER(210);
DECLARE_INTERRUPT_HANDLER(211);
DECLARE_INTERRUPT_HANDLER(212);
DECLARE_INTERRUPT_HANDLER(213);
DECLARE_INTERRUPT_HANDLER(214);
DECLARE_INTERRUPT_HANDLER(215);
DECLARE_INTERRUPT_HANDLER(216);
DECLARE_INTERRUPT_HANDLER(217);
DECLARE_INTERRUPT_HANDLER(218);
DECLARE_INTERRUPT_HANDLER(219);
DECLARE_INTERRUPT_HANDLER(220);
DECLARE_INTERRUPT_HANDLER(221);
DECLARE_INTERRUPT_HANDLER(222);
DECLARE_INTERRUPT_HANDLER(223);
DECLARE_INTERRUPT_HANDLER(224);
DECLARE_INTERRUPT_HANDLER(225);
DECLARE_INTERRUPT_HANDLER(226);
DECLARE_INTERRUPT_HANDLER(227);
DECLARE_INTERRUPT_HANDLER(228);
DECLARE_INTERRUPT_HANDLER(229);
DECLARE_INTERRUPT_HANDLER(230);
DECLARE_INTERRUPT_HANDLER(231);
DECLARE_INTERRUPT_HANDLER(232);
DECLARE_INTERRUPT_HANDLER(233);
DECLARE_INTERRUPT_HANDLER(234);
DECLARE_INTERRUPT_HANDLER(235);
DECLARE_INTERRUPT_HANDLER(236);
DECLARE_INTERRUPT_HANDLER(237);
DECLARE_INTERRUPT_HANDLER(238);
DECLARE_INTERRUPT_HANDLER(239);
DECLARE_INTERRUPT_HANDLER(240);
DECLARE_INTERRUPT_HANDLER(241);
DECLARE_INTERRUPT_HANDLER(242);
DECLARE_INTERRUPT_HANDLER(243);
DECLARE_INTERRUPT_HANDLER(244);
DECLARE_INTERRUPT_HANDLER(245);
DECLARE_INTERRUPT_HANDLER(246);
DECLARE_INTERRUPT_HANDLER(247);
DECLARE_INTERRUPT_HANDLER(248);
DECLARE_INTERRUPT_HANDLER(249);
DECLARE_INTERRUPT_HANDLER(250);
DECLARE_INTERRUPT_HANDLER(251);
DECLARE_INTERRUPT_HANDLER(252);
DECLARE_INTERRUPT_HANDLER(253);
DECLARE_INTERRUPT_HANDLER(254);
DECLARE_INTERRUPT_HANDLER(255);


__attribute__((naked))
VOID
KiInterruptNop(
    VOID);


VOID
KERNELAPI
KiInitializeProcessor(
    VOID);

VOID
KERNELAPI
KiInitialize(
    VOID);




