
#pragma once


//
// Interrupt handlers.
//

typedef struct _KSTACK_FRAME_INTERRUPT
{
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

    U8 Reserved[128+8];

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
    VOID __attribute__((naked))    \
    INTERRUPT_HANDLER_NAME(_vector) (VOID)

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
    "sub rsp, 0x88\n\t" /* red zone size + 8 byte */ \
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
    "push gs\n\t" /* todo: make sure that rsp is 16-byte aligned after push */

#define ASM_INTERRUPT_FRAME_POP    \
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
    VOID __attribute__((naked))     \
    INTERRUPT_HANDLER_NAME(_vector) (VOID) {\
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


#define DECLARE_EXCEPTION_HANDLER(_name)    \
    VOID __attribute((naked)) _name(VOID)

#define EXCEPTION_HANDLER(_name, _id)       \
    DECLARE_EXCEPTION_HANDLER(_name) {      \
        __asm__ __volatile__ (              \
            "cli\n\t"                       \
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
            "cli\n\t"                       \
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


