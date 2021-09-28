#pragma once

#define	DBG_SPECIAL_IO_PORT				0xe9

typedef enum _DBG_TRACE_LEVEL
{
	TraceLevelAll,			// All
	TraceLevelDebug,		// Debug & Event & Warning & Error
	TraceLevelEvent,		// Event & Warning & Error
	TraceLevelWarning,		// Warning & Error
	TraceLevelError,		// Error
} DBG_TRACE_LEVEL;


#define COM1_IO_DEFAULT_BASE        0x3f8
#define COM2_IO_DEFAULT_BASE        0x2f8
#define COM3_IO_DEFAULT_BASE        0x3e8
#define COM4_IO_DEFAULT_BASE        0x2e8

#define COM_IO_DATA                 0
#define COM_IO_IER                  1
#define COM_IO_DIVISOR_LOW          COM_IO_DATA
#define COM_IO_DIVISOR_HIGH         COM_IO_IER
#define COM_IO_FIFO_CTL             2
#define COM_IO_LINE_CTL             3
#define COM_IO_MODEM_CTL            4
#define COM_IO_LINE_STATUS          5
#define COM_IO_MODEM_STATUS         6
#define COM_IO_STRATCH              7

#define COM_DEFAULT_BAUD            115200L




typedef
BOOLEAN
(KERNELAPI *PDBG_TRACE_ROUTINE)(
	IN DBG_TRACE_LEVEL TraceLevel,
	IN CHAR8 *TraceMessage,
	IN SIZE_T Length);


extern PDBG_TRACE_ROUTINE DbgpTrace;
extern DBG_TRACE_LEVEL DbgPrintTraceLevel;


BOOLEAN
KERNELAPI
DbgInitialize(
    IN OS_LOADER_BLOCK *LoaderBlock,
	IN DBG_TRACE_LEVEL DefaultTraceLevel);

BOOLEAN
KERNELAPI
DbgTraceN(
	IN DBG_TRACE_LEVEL TraceLevel,
	IN CHAR8 *TraceMessage,
	IN SIZE_T Length);

BOOLEAN
KERNELAPI
DbgTraceF(
	IN DBG_TRACE_LEVEL TraceLevel,
	IN CHAR8 *Format,
	...);

BOOLEAN
KERNELAPI
DbgTrace(
	IN DBG_TRACE_LEVEL TraceLevel,
	IN CHAR8 *TraceMessage);

#if 0
VOID
KERNELAPI
DbgHardwareBreak(
	VOID);
#endif

VOID
KERNELAPI
DbgFootprint(
    IN U32 Value, 
    IN U32 Pixel);

#define DFOOTPRN(_x)        DbgFootprint((_x), 0xff0000 >> (_x))



#define	DASSERT(_expr)	{	\
	if(!(_expr))	{	\
		DbgTraceF(TraceLevelDebug,	\
			"\n"	\
			" ========= Debug Assertion Failed ========= \n"	\
			"  Expr   : " _ASCII(_expr) "\n"	\
			"  Source : " __FILE__ "\n"	\
			"  Line   : %d\n"	\
			"\n",	\
			__LINE__);	\
		for(;;) __halt();	\
	}	\
}
