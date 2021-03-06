
#pragma once

typedef struct _AP_INIT_PACKET
{
    U8 JumpOp[3];           // 0xe9 disp16
    U8 Status;              // Status code.
    U16 HeaderSize;         // Size of header.
    U16 PacketSize;         // Size of packet.
    U8 Signature[8];        // "AP16INIT"
    U32 PML4Base;           // (+0x10) 32-bit physical address of PML4 base.
    U32 LM64StackSize;      // (+0x14) Stack size in 64-bit context.
    U64 LM64StackAddress;   // (+0x18) Stack base in 64-bit context.
    U64 LM64StartAddress;   // (+0x20) Starting address in 64-bit context.
    
    // Initial GDT
    U64 SegmentDescriptors[3]; // null (0), code (0x00af9a000000ffff), data (0x00cf92000000ffff)
} AP_INIT_PACKET;


VOID
__attribute__((naked))
HalpApplicationProcessorInitStub(
    VOID);

