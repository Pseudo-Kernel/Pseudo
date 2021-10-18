
/**
 * @file apstub16.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements 16-bit stub for AP initialization.
 * @version 0.1
 * @date 2021-10-19
 * 
 * @copyright Copyright (c) 2021
 * 
 * @todo Not tested yet...
 * 
 */

#include <base/base.h>
#include <hal/apstub16.h>

/**
 * @brief 16-bit stub for AP initialization.\n
 *        This function performs mode switching and jumps to the 64-bit starting address.
 * 
 * @return This function never returns.
 * 
 * @note Kernel relocates this stub to 0xvv000 before call (where vv is reset vector).
 */
VOID
__attribute__((naked))
HalpApplicationProcessorInitStub(
    VOID)
{
    __asm__ __volatile__(
        ".code16\n\t"
        "_BASE:\n\t"

        "_i386ApInitPacket:\n\t"
        ".byte 0xe9\n\t"            // AP_INIT_PACKET::JumpOp
        ".word (_i386Init-_BASE)\n\t"
        "nop\n\t"
        ".int (_i386ApInitPacketHeaderEnd-_i386ApInitPacket)\n\t" // AP_INIT_PACKET::HeaderSize
        ".int (_i386ApStubEnd-_i386ApInitPacket)\n\t"             // AP_INIT_PACKET::PacketSize
        ".ascii \"AP16INIT\"\n\t"   // AP_INIT_PACKET::Signature
        ".int 0x0\n\t"              // AP_INIT_PACKET::PML4Base
        ".int 0x0\n\t"              // AP_INIT_PACKET::LM64StackSize
        ".int 0x0\n\t"              // AP_INIT_PACKET::LM64StackAddress
        ".int 0x0\n\t"
        ".int 0x0\n\t"              // AP_INIT_PACKET::LM64StartAddress
        ".int 0x0\n\t"
        "_i386ApInitPacketHeaderEnd:\n\t"

        "_i386Gdt:\n\t"
        ".int 0x0; .int 0x0\n\t"                // GDT Entry #0 - null selector
        ".int 0x0000ffff; .int 0x00af9a00\n\t"  // GDT Entry #1 - kernel code segment
        ".int 0x0000ffff; .int 0x00cf9200\n\t"  // GDT Entry #2 - kernel data segment

        "_i386Gdtr:\n\t"
        ".word 0x18 - 0x1\n\t"  // GDT has total three entries
        ".word 0x00\n\t"        // Lower base
        ".word 0x00\n\t"        // Higher base

        "_i386Init:\n\t"
        "cli\n\t"
        "xor ax, ax\n\t"
        "mov ss, ax\n\t"
        "mov ax, 0x200\n\t" // AP16_CODE_SEG
        "mov ds, ax\n\t"
        "mov es, ax\n\t"
        "mov fs, ax\n\t"
        "mov gs, ax\n\t"
        "push ax\n\t"
        "push (_i386PrepareTransfer-_BASE)\n\t"
        "retf\n\t"

        // Setup the stack.
        "_i386PrepareTransfer:\n\t"
        "mov sp, 0x7bfe\n\t" // AP16_TEMP_STACK_LIMIT

        // GDTR.Base must point physical address of GDT
        // Cs.BASE + i386Gdt(Offset) = PhysicalAddress of i386Gdt
        "push cs\n\t"
        "pop dx\n\t"
        "and edx, 0xffff\n\t"
        "shl edx, 4\n\t"
        "mov eax, (_i386Gdt-_BASE)\n\t"
        "add eax, edx\n\t"

        // Locate Initial GDT.
        "mov dword ptr [_i386Gdtr-_BASE+2], eax\n\t"
        "lgdt [_i386Gdtr-_BASE]\n\t"

        //
        // 1. Set CR4.PAE = 1
        // 2. Setup PML4 and Load to CR3
        // 3. Set MSR[IA32_EFER].LME = 1
        // 4. Set CR0.PG = 1, CR0.PE = 1
        // 5. Jump to the 64-bit segment
        //

        "mov eax, cr4\n\t"
        "or eax, 0x20\n\t"
        "mov cr4, eax\n\t"

        "mov eax, dword ptr [_i386ApInitPacket-_BASE+0x10]\n\t" // _i386ApInitPacket.AP_INIT_PACKET.PML4Base
        "mov cr3, eax\n\t"

        "mov ecx, 0xc0000080\n\t"   // IA32_EFER
        "rdmsr\n\t"
        "or eax, 0x100\n\t"         // EFER_LME

        // disable cache!
        "mov eax, cr0\n\t"
        "or eax, 0xe0000001\n\t"  // CR0_PE | CR0_NW | CR0_CD | CR0_PG
        "mov cr0, eax\n\t"

        // Jump to the 64-bit code.
        // edx = cs.BASE
        "push cs\n\t"
        "pop dx\n\t"
        "and edx, 0xffff\n\t"
        "shl edx, 4\n\t"

        "mov ax, 0x10\n\t" // AP_KERNEL_DS
        "mov ds, ax\n\t"
        "mov es, ax\n\t"
        "mov fs, ax\n\t"
        "mov gs, ax\n\t"
        "mov ss, ax\n\t"

        // jmp AP_KERNEL_CS:(i386LongMode + (AP16_CODE_SEG * 10h))
        ".byte 0x66\n\t"
        ".byte 0xea\n\t"
        ".int (_i386LongMode-_BASE + (0x200*0x10))\n\t"
        ".word 0x08\n\t" // AP_KERNEL_CS

        // Now we are in the long mode!!
        // edx = Previous CS.BASE
        // mov rsp, qword ptr [edx + i386ApInitPacket.AP_INIT_PACKET.LM64StackAddress]
        // sub rsp, 20h
        // call qword ptr [edx + i386ApInitPacket.AP_INIT_PACKET.LM64StartAddress]
        // add rsp, 20h
        "_i386LongMode:"
        ".byte 0x67; .byte 0x48; .byte 0x8b; .byte 0xa2;\n\t"
        ".int 0x18\n\t" // offset i386ApInitPacket.AP_INIT_PACKET.LM64StackAddress
        ".byte 0x48\n\t"
        "sub sp, 0x80\n\t"
        ".byte 0x67; .byte 0xff; .byte 0x92;\n\t"
        ".int 0x20\n\t" // offset i386ApInitPacket.AP_INIT_PACKET.LM64StartAddress

        "cli\n\t"
        "hlt\n\t"

        "_i386ApStubEnd:\n\t"
        
        :
        :
        : "memory"
    );
}
