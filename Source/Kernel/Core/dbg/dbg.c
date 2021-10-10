
#include <base/base.h>
#include <init/bootgfx.h>

PDBG_TRACE_ROUTINE DbgpTrace = NULL;
DBG_TRACE_LEVEL DbgPrintTraceLevel = TraceLevelDebug;

U16 DbgSerialPortBase[] =
{
    COM1_IO_DEFAULT_BASE,
    COM2_IO_DEFAULT_BASE,
    COM3_IO_DEFAULT_BASE,
    COM4_IO_DEFAULT_BASE
};

volatile PVOID DbgpFramebufferBase;
SIZE_T DbgpFramebufferSize;

BOOLEAN
KERNELAPI
DbgpNormalTraceN(
    IN DBG_TRACE_LEVEL TraceLevel,
    IN CHAR8 *TraceMessage, 
    IN SIZE_T Length)
{
    if (TraceLevel < DbgPrintTraceLevel)
        return FALSE;

#if KERNEL_BUILD_TARGET_EMULATOR
    __outbytestring(DBG_SPECIAL_IO_PORT, (U8 *)TraceMessage, Length);
#else

#endif

    return TRUE;
}

BOOLEAN
KERNELAPI
DbgpNormalTraceSerialN(
    IN DBG_TRACE_LEVEL TraceLevel,
    IN CHAR8 *TraceMessage, 
    IN SIZE_T Length)
{
    if (TraceLevel < DbgPrintTraceLevel)
        return FALSE;

    U16 Base = DbgSerialPortBase[0]; // COM1
    CHAR8 *s = TraceMessage, c = 0;

    while ((c = *s++))
    {
        // while(!LINE_STATUS.TransmitEmpty)
        while(!(__inbyte(Base + COM_IO_LINE_STATUS) & 0x20));

        __outbyte(Base + COM_IO_DATA, c);
    }

    return TRUE;
}

BOOLEAN
KERNELAPI
DbgTraceN(
    IN DBG_TRACE_LEVEL TraceLevel,
    IN CHAR8 *TraceMessage, 
    IN SIZE_T Length)
{
    if (!DbgpTrace)
        return FALSE;

    return DbgpTrace(TraceLevel, TraceMessage, Length);
}

BOOLEAN
VARCALL
DbgTraceF(
    IN DBG_TRACE_LEVEL TraceLevel,
    IN CHAR8 *Format,
    ...)
{
    va_list Args;
    CHAR8 Buffer[512];
    SIZE_T Length;

    if (!DbgpTrace)
        return FALSE;

    va_start(Args, Format);
    Length = ClStrFormatU8V(Buffer, ARRAY_SIZE(Buffer), Format, Args);
    va_end(Args);

    return DbgpTrace(TraceLevel, Buffer, Length);
}

BOOLEAN
KERNELAPI
DbgTrace(
    IN DBG_TRACE_LEVEL TraceLevel,
    IN CHAR8 *TraceMessage)
{
    return DbgTraceN(TraceLevel, TraceMessage, strlen(TraceMessage));
}


VOID
KERNELAPI
DbgInitializeSerial(
    VOID)
{
    U32 BaudRate = COM_DEFAULT_BAUD;
    U16 Divisor = (U16)(COM_DEFAULT_BAUD / BaudRate);

    for (UINT i = 0; i < COUNTOF(DbgSerialPortBase); i++)
    {
        // Interrupt disabled
        U16 Base = DbgSerialPortBase[i];

        __outbyte(Base + COM_IO_IER, 0x00);

        // DLAB=1 (MSB of LINECTL)
        __outbyte(Base + COM_IO_LINE_CTL, 0x80);
        __outbyte(Base + COM_IO_DIVISOR_LOW, (U8)(Divisor & 0xff));
        __outbyte(Base + COM_IO_DIVISOR_HIGH, (U8)(Divisor >> 0x08));

        // LINECTL -> Parity none(??xx0), 1 stop bit(0), 8-bit(11) -> ??xx0011
        __outbyte(Base + COM_IO_LINE_CTL, 0x03);

        // DTR/RTS set
        __outbyte(Base + COM_IO_MODEM_CTL, 0x03);
    }
}

BOOLEAN
KERNELAPI
DbgInitialize(
    IN OS_LOADER_BLOCK *LoaderBlock,
    IN DBG_TRACE_LEVEL DefaultTraceLevel)
{
#if KERNEL_BUILD_TARGET_EMULATOR
    DbgpTrace = DbgpNormalTraceN;
#else
    DbgpTrace = DbgpNormalTraceSerialN;
#endif

    DbgPrintTraceLevel = DefaultTraceLevel;

    DbgInitializeSerial();

    DbgpFramebufferBase = (PVOID)LoaderBlock->LoaderData.VideoFramebufferBase;
    DbgpFramebufferSize = LoaderBlock->LoaderData.VideoFramebufferSize;

    return TRUE;
}

VOID
KERNELAPI
DbgFootprint(
    IN U32 Value, 
    IN U32 Pixel)
{
    for (INT X = Value * 100; X < (Value + 1) * 100 + 50; X++)
    {
        *((U32 volatile *)DbgpFramebufferBase + X) = Pixel;
    }
}

#if 0
__attribute__((naked))
VOID
KERNELAPI
DbgHardwareBreak(
    VOID)
{
    // 
    // DR0 -> RW0=00 L0=1 G0=1 LEN0=00
    //     -> DR7[17:16]=00 DR7[0]=1 DR7[1]=1 DR7[19:18]=00
    //     -> Mask 1111 0000 0000 0000 0011 = 0xf0003
    //     -> Value 0000 0000 0000 0000 0011 = 0x00003
    // 

    // We'll use DR0 for QEMU break
    __asm__ __volatile__ (
        "lea rax, qword ptr [__DbgBreakOn]\n\t"
        "mov dr0, rax\n\t"
        "mov rax, dr7\n\t"
        "and rax, not 0xf0003\n\t"
        "or rax, 0x03\n\t"
        "mov dr7, rax\n\t"
        "__DbgBreakOn:\n\t"
        "ret\n\t"
    );
}
#endif
