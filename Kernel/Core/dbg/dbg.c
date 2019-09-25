
#include <Base.h>

#define	DBG_SPECIAL_IO_PORT				0xe9

PDBG_TRACE_ROUTINE DbgpTrace = NULL;
DBG_TRACE_LEVEL DbgPrintTraceLevel = TraceLevelDebug;


BOOLEAN
KERNELAPI
DbgpNormalTraceN(
	IN DBG_TRACE_LEVEL TraceLevel,
	IN CHAR8 *TraceMessage, 
	IN SIZE_T Length)
{
	CHAR8 *p = TraceMessage;

	if (TraceLevel < DbgPrintTraceLevel)
		return FALSE;

	__PseudoIntrin_OutPortBuffer8(DBG_SPECIAL_IO_PORT, (U8 *)TraceMessage, Length);
//	__outbytestring(DBG_SPECIAL_IO_PORT, TraceMessage, (U32)Length);

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
KERNELAPI
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

BOOLEAN
KERNELAPI
DbgInitialize(
	IN DBG_TRACE_LEVEL DefaultTraceLevel)
{
	DbgpTrace = DbgpNormalTraceN;
	DbgPrintTraceLevel = DefaultTraceLevel;

	return TRUE;
}

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
