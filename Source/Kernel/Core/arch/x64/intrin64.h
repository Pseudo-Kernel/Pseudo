
#ifndef _INTRIN64_H_
#define	_INTRIN64_H_

// http://svn.assembla.com/svn/iLog/intrin_x86.h

#define __PSEUDO_INTRINSIC_HEADER	\
	static __inline__ __attribute__((always_inline))
#define __PSEUDO_INTRINSIC(_type_ret, _fn)    \
    __PSEUDO_INTRINSIC_HEADER _type_ret __PseudoIntrin_##_fn



#define	PSEUDO_INTRINSIC_INPORT(_fn_postfix, _type_val)	\
	__PSEUDO_INTRINSIC(_type_val, InPort##_fn_postfix)	\
	(unsigned __int16 Port)	{							\
		_type_val Value = 0;					   		\
	    __asm__ __volatile__ (							\
	        "in %0, %1\n\t"								\
	        : "=a"(Value)								\
	        : "d"(Port)									\
    	    : /*"memory"*/								\
    	);											      	\
		return Value;									   \
	}

#define	PSEUDO_INTRINSIC_OUTPORT(_fn_postfix, _type_val)	\
	__PSEUDO_INTRINSIC(void, OutPort##_fn_postfix)  	\
	(unsigned __int16 Port, _type_val Value) {			\
	    __asm__ __volatile__ (								\
	        "out %0, %1\n\t"								\
	        :											   	\
	        : "d"(Port), "a"(Value)						\
    	    : /*"memory"*/									\
    	);													      \
	}

#define	PSEUDO_INTRINSIC_INPORT_BUFFER(_fn_postfix, _type_val, _inst_postfix)	\
	__PSEUDO_INTRINSIC(void, InPortBuffer##_fn_postfix)	\
	(unsigned __int16 Port, _type_val *Buffer, __int64 Count) {	\
	    __asm__ __volatile__ (							\
	        "rep ins" _inst_postfix "\n\t"				\
	        : "=D"(Buffer), "=c"(Count)					\
	        : "d"(Port), "c"(Count)						\
    	    : "memory"/* should we need compiler-level barrier? */	\
    	);												\
	}

#define	PSEUDO_INTRINSIC_OUTPORT_BUFFER(_fn_postfix, _type_val, _inst_postfix)	\
	__PSEUDO_INTRINSIC(void, OutPortBuffer##_fn_postfix)	\
	(unsigned __int16 Port, _type_val *Buffer, __int64 Count) {	\
	    __asm__ __volatile__ (								\
	        "rep outs" _inst_postfix "\n\t"					\
	        : "=c"(Count)									\
	        : "d"(Port), "S"(Buffer), "c"(Count)			\
    	    : "memory"/* should we need compiler-level barrier? */	\
    	);													\
	}

#define	PSEUDO_INTRINSIC_INTERLOCKEDEXCHANGE(_fn_postfix, _type_val)	\
	__PSEUDO_INTRINSIC(_type_val, InterlockedExchange##_fn_postfix)		\
	(_type_val volatile *Target, _type_val Value) {						\
	    return __atomic_exchange_n(Target, Value, __ATOMIC_SEQ_CST);	\
	}
//    __asm__ __volatile__ (
//        "lock xchg %0, %1\n\t"
//        : "+m"(Target)
//        : "r"(Value)
//        : "memory"
//    );




// __PseudoIntrin_InPortXxx
PSEUDO_INTRINSIC_INPORT(8, unsigned __int8)
PSEUDO_INTRINSIC_INPORT(16, unsigned __int16)
PSEUDO_INTRINSIC_INPORT(32, unsigned __int32)

// __PseudoIntrin_OutPortXxx
PSEUDO_INTRINSIC_OUTPORT(8, unsigned __int8)
PSEUDO_INTRINSIC_OUTPORT(16, unsigned __int16)
PSEUDO_INTRINSIC_OUTPORT(32, unsigned __int32)

// __PseudoIntrin_InPortBufferXxx
PSEUDO_INTRINSIC_INPORT_BUFFER(8, unsigned __int8, "b")
PSEUDO_INTRINSIC_INPORT_BUFFER(16, unsigned __int16, "w")
PSEUDO_INTRINSIC_INPORT_BUFFER(32, unsigned __int32, "d")

// __PseudoIntrin_OutPortBufferXxx
PSEUDO_INTRINSIC_OUTPORT_BUFFER(8, unsigned __int8, "b")
PSEUDO_INTRINSIC_OUTPORT_BUFFER(16, unsigned __int16, "w")
PSEUDO_INTRINSIC_OUTPORT_BUFFER(32, unsigned __int32, "d")

// __PseudoIntrin_InterlockedExchangeXxx
PSEUDO_INTRINSIC_INTERLOCKEDEXCHANGE(8, __int8)
PSEUDO_INTRINSIC_INTERLOCKEDEXCHANGE(16, __int16)
PSEUDO_INTRINSIC_INTERLOCKEDEXCHANGE(32, __int32)
PSEUDO_INTRINSIC_INTERLOCKEDEXCHANGE(64, __int64)


__PSEUDO_INTRINSIC(void, DisableInterrupt) ()
{
    __asm__ __volatile__ ("cli\n\t" ::: "memory");
}

__PSEUDO_INTRINSIC(void, EnableInterrupt) ()
{
    __asm__ __volatile__ ("sti\n\t" ::: "memory");
}

__PSEUDO_INTRINSIC(void, Halt) ()
{
    __asm__ __volatile__ ("hlt\n\t" ::: "memory");
}

__PSEUDO_INTRINSIC(void, Pause) ()
{
    __asm__ __volatile__ ("pause\n\t" ::: "memory");
}

__PSEUDO_INTRINSIC(__int64, ReadTimestampCounter) ()
{
	__int64 Value = 0;
	__asm__ __volatile__ ("rdtsc\n\t" : "=A"(Value));
	return Value;
}





#if 0

//
// x64 Assembly Instructions.
//

void *_ReturnAddress();				// MS-specific

void __cpuid(
   int CPUInfo[4],
   int InfoType
);

void __cpuidex(
   int CPUInfo[4],
   int InfoType,
   int ECXValue
);

unsigned __int64 __readeflags();				// returns RFLAGS
void __writeeflags(unsigned __int64 Value);		// writes to RFLAGS

void _enable();			// sets XFLAGS.IF
void _disable();		// clears XFLAGS.IF

void __debugbreak();	// trap into the debugger (int 03h)
void __halt();			// halt

void _lgdt(void *Source);		// loads the GDTR.
void _sgdt(void *Destination);	// stores the GDTR.

void __lidt(void *Source);		// loads the IDTR.
void __sidt(void *Destination);	// stores the IDTR.

void __invlpg(void* Address);


void _mm_pause();	// pause

unsigned char _interlockedbittestandset(long *a, long b);
unsigned char _interlockedbittestandset64(__int64 *a, __int64 b);

long _InterlockedExchange(
   long volatile * Target,
   long Value
);

char _InterlockedExchange8(
   char volatile * Target,
   char Value
);

short _InterlockedExchange16(
   short volatile * Target,
   short Value
);

__int64 _InterlockedExchange64(
   __int64 volatile * Target,
   __int64 Value
);

#define	_InterlockedExchangePointer		_InterlockedExchange64

long _InterlockedCompareExchange(
   long volatile * Destination,
   long Exchange,
   long Comparand
);

short _InterlockedCompareExchange16(
   short volatile * Destination,
   short Exchange,
   short Comparand
);

__int64 _InterlockedCompareExchange64(
   __int64 volatile * Destination,
   __int64 Exchange,
   __int64 Comparand
);

unsigned char _InterlockedCompareExchange128(
   __int64 volatile * Destination,
   __int64 ExchangeHigh,
   __int64 ExchangeLow,
   __int64 * Comparand
);


long _InterlockedExchangeAdd(
	long volatile *Addend,
	long Value
);

__int64 _InterlockedExchangeAdd64(
	__int64 volatile *Addend,
	__int64 Value
);


long _InterlockedOr(
   long volatile * Value,
   long Mask
);

char _InterlockedOr8(
   char volatile * Value,
   long Mask
);

short _InterlockedOr16(
   short volatile * Value,
   short Mask
);

__int64 _InterlockedOr64(
   __int64 volatile * Value,
   __int64 Mask
);



unsigned __int64 __rdtsc();

unsigned __int64 __readcr0(void);
unsigned __int64 __readcr2(void);
unsigned __int64 __readcr3(void);
unsigned __int64 __readcr4(void);
unsigned __int64 __readcr8(void);

unsigned __int64 __readdr(unsigned int DebugRegister);


void __writecr0(unsigned __int64 Data);
void __writecr3(unsigned __int64 Data);
void __writecr4(unsigned __int64 Data);
void __writecr8(unsigned __int64 Data);

void __writedr(unsigned DebugRegister, unsigned __int64 DebugValue);


unsigned __int64 _xgetbv(unsigned int);
void _xsetbv(unsigned int,unsigned __int64);

unsigned __int64 __readmsr(unsigned long msr_addr);
void __writemsr(unsigned long msr_addr, unsigned __int64 value);

void __movsb( 
   unsigned char* Destination, 
   unsigned const char* Source, 
   unsigned __int64 Count 
);

void __movsw( 
   unsigned short* Dest, 
   unsigned short* Source, 
	unsigned __int64 Count
);

void __movsd( 
   unsigned long* Dest, 
   unsigned long* Source, 
   unsigned __int64 Count
);

void __movsq( 
   unsigned char* Dest, 
   unsigned char* Source, 
   unsigned __int64 Count
);

void __stosb( 
   unsigned char* Dest, 
   unsigned char Data, 
   unsigned __int64 Count
);

void __stosw( 
   unsigned short* Dest, 
   unsigned short Data, 
   unsigned __int64 Count
);

void __stosd( 
   unsigned long* Dest, 
   unsigned long Data, 
   unsigned __int64 Count
);

void __stosq( 
   unsigned __int64* Dest, 
   unsigned __int64 Data, 
   unsigned __int64 Count
);


unsigned char __inbyte(
   unsigned short Port
);

void __inbytestring(
   unsigned short Port,
   unsigned char* Buffer,
   unsigned long Count
);

unsigned short __inword(
   unsigned short Port
);

void __inwordstring(
   unsigned short Port,
   unsigned short* Buffer,
   unsigned long Count
);

unsigned long __indword(
   unsigned short Port
);

void __indwordstring(
   unsigned short Port,
   unsigned long* Buffer,
   unsigned long Count
);


void __outbyte( 
   unsigned short Port, 
   unsigned char Data 
);

void __outbytestring( 
   unsigned short Port, 
   unsigned char* Buffer, 
   unsigned long Count 
);

void __outword( 
   unsigned short Port, 
   unsigned short Data 
);

void __outwordstring( 
   unsigned short Port, 
   unsigned short* Buffer, 
   unsigned long Count 
);

void __outdword( 
   unsigned short Port, 
   unsigned long Data 
);

void __outdwordstring( 
   unsigned short Port, 
   unsigned long* Buffer, 
   unsigned long Count 
);

//
// InterlockedIncrement, InterlockedDecrement
//

long _InterlockedIncrement(
   long * lpAddend
);

short _InterlockedIncrement16(
   short * lpAddend
);

__int64 _InterlockedIncrement64(
   __int64 * lpAddend
);

long _InterlockedDecrement(
   long * lpAddend
);

short _InterlockedDecrement16(
   short * lpAddend
);

__int64 _InterlockedDecrement64(
   __int64 * lpAddend
);


//
// InterlockedBitTestAndSet, InterlockedBitTestAndReset
//

unsigned char _interlockedbittestandset(
   long *a,
   long b
);

unsigned char _interlockedbittestandset64(
   __int64 *a,
   __int64 b
);

unsigned char _interlockedbittestandreset(
   long *a,
   long b
);

unsigned char _interlockedbittestandreset64(
   __int64 *a,
   __int64 b
);

//
// readfsbase, writefsbase
//

unsigned int _readfsbase_u32(void );
unsigned __int64 _readfsbase_u64(void );
unsigned int _readgsbase_u32(void );
unsigned __int64 _readgsbase_u64(void );

void _writefsbase_u32( unsigned int );
void _writefsbase_u64( unsigned __int64 );
void _writegsbase_u32( unsigned int );
void _writegsbase_u64( unsigned __int64 );


//
// Memory fences.
//

extern void _mm_lfence(void);
extern void _mm_mfence(void);

//
// Bit Scan.
//

unsigned char _BitScanReverse(
   unsigned long * Index,
   unsigned long Mask
);

unsigned char _BitScanReverse64(
   unsigned long * Index,
   unsigned __int64 Mask
);

unsigned char _BitScanForward(
   unsigned long * Index,
   unsigned long Mask
);

unsigned char _BitScanForward64(
   unsigned long * Index,
   unsigned __int64 Mask
);

unsigned char _bittest64(__int64 const *, __int64);

//
// 64-to-128 multiply.
//

__int64 _mul128(
	__int64 Multiplier, 
	__int64 Multiplicand, 
	__int64 *HighProduct );

unsigned __int64 _umul128(
	unsigned __int64 Multiplier, 
	unsigned __int64 Multiplicand, 
	unsigned __int64 *HighProduct );

#endif

#endif
