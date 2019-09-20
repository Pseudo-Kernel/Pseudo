#pragma once

typedef enum _DBG_TRACE_LEVEL {
	TraceLevelAll,			// All
	TraceLevelDebug,		// Debug & Event & Warning & Error
	TraceLevelEvent,		// Event & Warning & Error
	TraceLevelWarning,		// Warning & Error
	TraceLevelError,		// Error
} DBG_TRACE_LEVEL;

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
		for(;;) __PseudoIntrin_Halt();	\
	}	\
}
