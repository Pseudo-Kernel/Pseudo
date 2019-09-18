
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

	__outbytestring(DBG_SPECIAL_IO_PORT, TraceMessage, (U32)Length);

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

