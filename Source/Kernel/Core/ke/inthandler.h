
#pragma once


//
// Interrupt handlers.
//

typedef struct _KSTACK_FRAME_INTERRUPT
{
    U8 Reserved[128+8];

    U64 Gs;
    U64 Fs;
    U64 Es;
    U64 Ds;

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

    U64 ErrorCode;
    U64 Rip;
    U64 Cs;
    U64 Rflags;
    U64 Rsp;
    U64 Ss;
} KSTACK_FRAME_INTERRUPT;

//
// Interrupt vector number.
//

#define SOFTWARE_INTERRUPT_YIELD_THREAD             0x20


//
// Exception/Interrupt handler macros.
//

#define INTERRUPT_HANDLER_NAME_BY_VECTOR(_vector)     \
    KiInterruptHandler_Vector ##_vector

#define DECLARE_INTERRUPT_HANDLER(_name)  \
    VOID __attribute__((naked))    \
    _name (VOID)

#define DECLARE_INTERRUPT_HANDLER_BY_VECTOR(_vector)  \
    VOID __attribute__((naked))    \
    INTERRUPT_HANDLER_NAME_BY_VECTOR(_vector) (VOID)

/* todo: make sure that rsp is 16-byte aligned after interrupt frame is pushed */
#define ASM_INTERRUPT_FRAME_PUSH_ERRCODE    \
    "push 0\n\t" /* error code placeholder */   \
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
    "mov eax, ds\n\t"\
    "push rax\n\t"   \
    "mov eax, es\n\t"\
    "push rax\n\t"   \
    "push fs\n\t"   \
    "push gs\n\t"   \
    "sub rsp, 0x88\n\t" /* red zone size + 8 byte */
 
#define ASM_INTERRUPT_FRAME_POP_ERRCODE    \
    "add rsp, 0x88\n\t"  /* red zone size + 8 byte */  \
    "pop gs\n\t"    \
    "pop fs\n\t"    \
    "pop rax\n\t"    \
    "mov es, eax\n\t"\
    "pop rax\n\t"    \
    "mov ds, eax\n\t"\
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
    "add rsp, 0x08\n\t" /* pop error code */

#define ASM_INTERRUPT_FRAME_PUSH    \
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
    "mov eax, ds\n\t"\
    "push rax\n\t"   \
    "mov eax, es\n\t"\
    "push rax\n\t"   \
    "push fs\n\t"   \
    "push gs\n\t"   \
    /* todo: make sure that rsp is 16-byte aligned after push */ \
    "sub rsp, 0x88\n\t" /* red zone size + 8 byte */ \

#define ASM_INTERRUPT_FRAME_POP    \
    "add rsp, 0x88\n\t"  /* red zone size + 8 byte */ \
    "pop gs\n\t"    \
    "pop fs\n\t"    \
    "pop rax\n\t"    \
    "mov es, eax\n\t"\
    "pop rax\n\t"    \
    "mov ds, eax\n\t"\
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
    VOID __attribute__((naked))     \
    INTERRUPT_HANDLER_NAME_BY_VECTOR(_vector) (VOID) {\
    __asm__ __volatile__ (  \
        ASM_INTERRUPT_FRAME_PUSH_ERRCODE    \
        "mov eax, %0\n\t"   \
        "mov ds, ax\n\t"    \
        "mov es, ax\n\t"    \
        "mov eax, %1\n\t"   \
        "mov fs, ax\n\t"    \
        "mov eax, %1\n\t"   \
        "mov gs, ax\n\t"    \
        "mov rdi, %2\n\t" /* sysvabi uses rdi for 1st argument */   \
        "mov rsi, rsp\n\t" /* sysvabi uses rsi for 2nd argument */  \
        "mov rcx, %2\n\t" /* msabi uses rcx for 1st argument */     \
        "mov rdx, rsp\n\t" /* msabi uses rdx for 2nd argument */    \
        "call KiCallInterruptChain\n\t"     \
        ASM_INTERRUPT_FRAME_POP_ERRCODE     \
        "iretq\n\t"         \
        :                   \
        : "i"(KERNEL_DS), "i"(KERNEL_FSGS), "i"((_vector))    \
        : "memory"          \
    );                      \
}

#define INTERRUPT_HANDLER_SOFTWARE(_name, _real_handler)  \
    VOID __attribute__((naked))     \
    _name (VOID) {\
    __asm__ __volatile__ (  \
        ASM_INTERRUPT_FRAME_PUSH_ERRCODE    \
        "mov eax, %0\n\t"   \
        "mov ds, ax\n\t"    \
        "mov es, ax\n\t"    \
        "mov eax, %1\n\t"   \
        "mov fs, ax\n\t"    \
        "mov eax, %1\n\t"   \
        "mov gs, ax\n\t"    \
        "mov rdi, rsp\n\t" /* sysvabi uses rdi for 1st argument */   \
        "mov rcx, rsp\n\t" /* msabi uses rcx for 1st argument */     \
        "call %2\n\t"     \
        ASM_INTERRUPT_FRAME_POP_ERRCODE     \
        "iretq\n\t"         \
        :                   \
        : "i"(KERNEL_DS), "i"(KERNEL_FSGS), "Ts"((_real_handler))  \
        : "memory"          \
    );                      \
}


#define DECLARE_EXCEPTION_HANDLER(_name)    \
    VOID __attribute((naked)) _name(VOID)

#define EXCEPTION_HANDLER(_name, _id)       \
    DECLARE_EXCEPTION_HANDLER(_name) {      \
        __asm__ __volatile__ (              \
            /*"cli\n\t"*/                   \
            ASM_INTERRUPT_FRAME_PUSH        \
            "mov eax, %0\n\t"   \
            "mov ds, ax\n\t"    \
            "mov es, ax\n\t"    \
            "mov eax, %1\n\t"   \
            "mov fs, ax\n\t"    \
            "mov eax, %1\n\t"   \
            "mov gs, ax\n\t"    \
            "mov rdi, %2\n\t"   \
            "mov rsi, rsp\n\t"  \
            "call KiDispatchException\n\t"  \
            ASM_INTERRUPT_FRAME_POP         \
            "iretq\n\t"                     \
            :                               \
            : "i"(KERNEL_DS), "i"(KERNEL_FSGS), "i"(_id)  \
            : "memory"                      \
        );                                  \
    }

#define EXCEPTION_HANDLER_PUSH_ERRCODE(_name, _id)  \
    DECLARE_EXCEPTION_HANDLER(_name) {      \
        __asm__ __volatile__ (              \
            /*"cli\n\t"*/                   \
            ASM_INTERRUPT_FRAME_PUSH_ERRCODE\
            "mov eax, %0\n\t"   \
            "mov ds, ax\n\t"    \
            "mov es, ax\n\t"    \
            "mov eax, %1\n\t"   \
            "mov fs, ax\n\t"    \
            "mov eax, %1\n\t"   \
            "mov gs, ax\n\t"    \
            "mov rdi, %2\n\t"   \
            "mov rsi, rsp\n\t"  \
            "call KiDispatchException\n\t"  \
            ASM_INTERRUPT_FRAME_POP_ERRCODE \
            "iretq\n\t"                     \
            :                               \
            : "i"(KERNEL_DS), "i"(KERNEL_FSGS), "i"(_id)  \
            : "memory"                      \
        );                                  \
    }


//
// Exception/Interrupt handlers.
//

DECLARE_EXCEPTION_HANDLER(KiDivideFault);
DECLARE_EXCEPTION_HANDLER(KiDebugTrap);
DECLARE_EXCEPTION_HANDLER(KiNMIInterrupt);
DECLARE_EXCEPTION_HANDLER(KiBreakpointTrap);
DECLARE_EXCEPTION_HANDLER(KiOverflowTrap);
DECLARE_EXCEPTION_HANDLER(KiBoundFault);
DECLARE_EXCEPTION_HANDLER(KiInvalidOpcode);
DECLARE_EXCEPTION_HANDLER(KiCoprocessorNotAvailable);
DECLARE_EXCEPTION_HANDLER(KiDoubleFault);
DECLARE_EXCEPTION_HANDLER(KiCoprocessorSegmentOverrun);
DECLARE_EXCEPTION_HANDLER(KiInvalidTss);
DECLARE_EXCEPTION_HANDLER(KiSegmentNotPresent);
DECLARE_EXCEPTION_HANDLER(KiStackSegmentFault);
DECLARE_EXCEPTION_HANDLER(KiGeneralProtectionFault);
DECLARE_EXCEPTION_HANDLER(KiPageFault);
DECLARE_EXCEPTION_HANDLER(KiUnexpectedException15); //Reserved by intel
DECLARE_EXCEPTION_HANDLER(KiX87FloatingPointFault);
DECLARE_EXCEPTION_HANDLER(KiAlignmentFault);
DECLARE_EXCEPTION_HANDLER(KiMachineCheck);
DECLARE_EXCEPTION_HANDLER(KiSIMDFloatingPointFault);
DECLARE_EXCEPTION_HANDLER(KiVirtualizationFault);
DECLARE_EXCEPTION_HANDLER(KiControlProtectionFault);

DECLARE_EXCEPTION_HANDLER(KiUnexpectedException22);
DECLARE_EXCEPTION_HANDLER(KiUnexpectedException23);
DECLARE_EXCEPTION_HANDLER(KiUnexpectedException24);
DECLARE_EXCEPTION_HANDLER(KiUnexpectedException25);
DECLARE_EXCEPTION_HANDLER(KiUnexpectedException26);
DECLARE_EXCEPTION_HANDLER(KiUnexpectedException27);
DECLARE_EXCEPTION_HANDLER(KiUnexpectedException28);
DECLARE_EXCEPTION_HANDLER(KiUnexpectedException29);
DECLARE_EXCEPTION_HANDLER(KiUnexpectedException30);
DECLARE_EXCEPTION_HANDLER(KiUnexpectedException31);

DECLARE_INTERRUPT_HANDLER(KiYieldThreadInterrupt);

DECLARE_INTERRUPT_HANDLER_BY_VECTOR(33);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(34);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(35);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(36);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(37);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(38);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(39);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(40);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(41);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(42);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(43);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(44);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(45);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(46);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(47);

DECLARE_INTERRUPT_HANDLER_BY_VECTOR(48);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(49);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(50);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(51);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(52);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(53);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(54);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(55);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(56);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(57);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(58);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(59);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(60);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(61);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(62);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(63);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(64);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(65);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(66);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(67);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(68);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(69);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(70);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(71);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(72);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(73);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(74);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(75);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(76);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(77);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(78);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(79);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(80);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(81);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(82);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(83);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(84);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(85);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(86);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(87);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(88);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(89);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(90);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(91);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(92);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(93);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(94);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(95);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(96);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(97);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(98);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(99);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(100);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(101);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(102);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(103);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(104);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(105);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(106);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(107);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(108);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(109);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(110);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(111);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(112);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(113);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(114);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(115);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(116);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(117);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(118);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(119);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(120);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(121);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(122);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(123);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(124);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(125);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(126);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(127);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(128);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(129);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(130);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(131);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(132);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(133);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(134);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(135);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(136);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(137);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(138);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(139);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(140);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(141);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(142);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(143);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(144);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(145);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(146);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(147);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(148);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(149);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(150);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(151);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(152);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(153);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(154);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(155);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(156);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(157);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(158);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(159);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(160);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(161);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(162);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(163);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(164);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(165);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(166);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(167);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(168);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(169);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(170);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(171);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(172);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(173);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(174);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(175);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(176);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(177);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(178);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(179);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(180);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(181);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(182);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(183);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(184);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(185);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(186);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(187);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(188);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(189);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(190);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(191);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(192);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(193);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(194);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(195);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(196);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(197);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(198);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(199);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(200);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(201);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(202);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(203);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(204);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(205);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(206);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(207);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(208);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(209);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(210);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(211);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(212);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(213);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(214);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(215);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(216);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(217);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(218);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(219);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(220);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(221);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(222);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(223);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(224);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(225);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(226);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(227);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(228);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(229);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(230);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(231);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(232);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(233);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(234);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(235);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(236);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(237);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(238);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(239);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(240);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(241);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(242);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(243);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(244);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(245);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(246);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(247);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(248);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(249);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(250);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(251);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(252);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(253);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(254);
DECLARE_INTERRUPT_HANDLER_BY_VECTOR(255);


