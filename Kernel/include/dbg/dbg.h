#pragma once


typedef
BOOLEAN
(KERNELAPI *PDBG_TRACE_ROUTINE)(
	IN enum _DBG_TRACE_LEVEL TraceLevel,
	IN CHAR8 *TraceMessage,
	IN SIZE_T Length);

typedef enum _DBG_TRACE_LEVEL {
	TraceLevelAll,			// All
	TraceLevelDebug,		// Debug & Event & Warning & Error
	TraceLevelEvent,		// Event & Warning & Error
	TraceLevelWarning,		// Warning & Error
	TraceLevelError,		// Error
} DBG_TRACE_LEVEL;

extern PDBG_TRACE_ROUTINE DbgpTrace;
extern DBG_TRACE_LEVEL DbgPrintTraceLevel;


BOOLEAN
KERNELAPI
DbgInitialize(
	IN enum _DBG_TRACE_LEVEL DefaultTraceLevel);

BOOLEAN
KERNELAPI
DbgTraceN(
	IN enum _DBG_TRACE_LEVEL TraceLevel,
	IN CHAR8 *TraceMessage,
	IN SIZE_T Length);

BOOLEAN
KERNELAPI
DbgTraceF(
	IN enum _DBG_TRACE_LEVEL TraceLevel,
	IN CHAR8 *Format,
	...);

BOOLEAN
KERNELAPI
DbgTrace(
	IN enum _DBG_TRACE_LEVEL TraceLevel,
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
		for(;;) __halt();	\
	}	\
}
