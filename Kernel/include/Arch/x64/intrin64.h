
#ifndef _INTRIN64_H_
#define	_INTRIN64_H_

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
