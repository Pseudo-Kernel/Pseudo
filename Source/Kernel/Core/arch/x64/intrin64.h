
#ifndef _INTRIN64_H_
#define	_INTRIN64_H_

// http://svn.assembla.com/svn/iLog/intrin_x86.h


#if 0

void __cpuid(
   int CPUInfo[4],
   int InfoType
);

void __cpuidex(
   int CPUInfo[4],
   int InfoType,
   int ECXValue
);



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





void __writedr(unsigned DebugRegister, unsigned __int64 DebugValue);


unsigned __int64 _xgetbv(unsigned int);
void _xsetbv(unsigned int,unsigned __int64);


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

#define _INTRINSIC_ATTRIBUTES           __attribute__((__gnu_inline__, __always_inline__, __artificial__))
#define _DEFINE_INTRINSIC(_return)      extern inline _return _INTRINSIC_ATTRIBUTES



_DEFINE_INTRINSIC(unsigned __int64)
__readeflags(void)
{
    unsigned __int64 Rflags = 0;
    __asm__ __volatile__ (
        "pushfq\n\t"
        "pop %0\n\t"
        : "=r"(Rflags)
        :
        : "memory"
    );

    return Rflags;
}

_DEFINE_INTRINSIC(void)
__writeeflags(
    unsigned __int64 Value)
{
    __asm__ __volatile__ (
        "push %0\n\t"
        "popfq\n\t"
        :
        : "r"(Value)
        : "memory", "cc" /* conditional flags will be overwritten */
    );
}

_DEFINE_INTRINSIC(void)
_enable(
    void)
{
    __asm__ __volatile__ (
        "sti\n\t"
        :
        :
        :
    );
}

_DEFINE_INTRINSIC(void)
_disable(
    void)
{
    __asm__ __volatile__ (
        "cli\n\t"
        :
        :
        :
    );
}

_DEFINE_INTRINSIC(void)
__debugbreak(
    void)
{
    __asm__ __volatile__ (
        "int 3h\n\t"
        :
        :
        :
    );
}

_DEFINE_INTRINSIC(void)
__halt(
    void)
{
    __asm__ __volatile__ (
        "hlt\n\t"
        :
        :
        :
    );
}

_DEFINE_INTRINSIC(void)
__lgdt(
    void *Source)
{
    __asm__ __volatile__ (
        "lgdt [%0]\n\t"
        :
        : "r"(Source)
        : "memory" /* prevent optimize out */
    );
}

_DEFINE_INTRINSIC(void)
__sgdt(
    void *Destination)
{
    __asm__ __volatile__ (
        "sgdt [%0]\n\t"
        : "=m"(Destination)
        :
        : "memory"
    );
}

_DEFINE_INTRINSIC(void)
__lidt(
    void *Source)
{
    __asm__ __volatile__ (
        "lidt [%0]\n\t"
        :
        : "r"(Source)
        : "memory" /* prevent optimize out */
    );
}

_DEFINE_INTRINSIC(void)
__sidt(
    void *Destination)
{
    __asm__ __volatile__ (
        "sidt [%0]\n\t"
        : "=m"(Destination)
        :
        : "memory"
    );
}

_DEFINE_INTRINSIC(void)
__invlpg(
    void* Address)
{
    __asm__ __volatile__ (
        "invlpg [%0]\n\t"
        :
        : "r"(Address)
        : "memory"
    );
}

_DEFINE_INTRINSIC(void)
__ltr(
    unsigned short Selector)
{
    __asm__ __volatile__ (
        "ltr %0\n\t"
        :
        : "r"(Selector)
        : "memory"
    );
}

_DEFINE_INTRINSIC(unsigned short)
__str(
    void)
{
    unsigned short Value = 0;
    __asm__ __volatile__ (
        "str %0\n\t"
        : "=r"(Value)
        :
        :
    );

    return Value;
}

_DEFINE_INTRINSIC(void)
_mm_pause(
    void)
{
    __builtin_ia32_pause();
}


_DEFINE_INTRINSIC(unsigned __int64)
__rdtsc(
    void)
{
    unsigned long high = 0, low = 0;
    __asm__ __volatile__ (
        "rdtsc\n\t"
        : "=d"(high), "=a"(low)
        :
        :
    );

    return low | ((unsigned long long)high << 0x20);
}

_DEFINE_INTRINSIC(unsigned __int64)
__readcr0(void)
{
    unsigned long long value = 0;
    __asm__ __volatile__ (
        "mov %0, cr0\n\t"
        : "=r"(value)
        :
        :
    );

    return value;
}

_DEFINE_INTRINSIC(unsigned __int64)
__readcr2(void)
{
    unsigned long long value = 0;
    __asm__ __volatile__ (
        "mov %0, cr2\n\t"
        : "=r"(value)
        :
        :
    );

    return value;
}

_DEFINE_INTRINSIC(unsigned __int64)
__readcr3(void)
{
    unsigned long long value = 0;
    __asm__ __volatile__ (
        "mov %0, cr3\n\t"
        : "=r"(value)
        :
        :
    );

    return value;
}

_DEFINE_INTRINSIC(unsigned __int64)
__readcr4(void)
{
    unsigned long long value = 0;
    __asm__ __volatile__ (
        "mov %0, cr4\n\t"
        : "=r"(value)
        :
        :
    );

    return value;
}

_DEFINE_INTRINSIC(unsigned __int64)
__readcr8(void)
{
    unsigned long long value = 0;
    __asm__ __volatile__ (
        "mov %0, cr8\n\t"
        : "=r"(value)
        :
        :
    );

    return value;
}

//unsigned __int64 __readdr(unsigned int DebugRegister);


_DEFINE_INTRINSIC(void)
__writecr0(
    unsigned __int64 Data)
{
    __asm__ __volatile__ (
        "mov cr0, %0\n\t"
        :
        : "r"(Data)
        :
    );
}

_DEFINE_INTRINSIC(void)
__writecr3(
    unsigned __int64 Data)
{
    __asm__ __volatile__ (
        "mov cr3, %0\n\t"
        :
        : "r"(Data)
        :
    );
}

_DEFINE_INTRINSIC(void)
__writecr4(
    unsigned __int64 Data)
{
    __asm__ __volatile__ (
        "mov cr4, %0\n\t"
        :
        : "r"(Data)
        :
    );
}

_DEFINE_INTRINSIC(void)
__writecr8(
    unsigned __int64 Data)
{
    __asm__ __volatile__ (
        "mov cr8, %0\n\t"
        :
        : "r"(Data)
        :
    );
}

_DEFINE_INTRINSIC(unsigned __int64)
__readmsr(
    unsigned long msr_addr)
{
    unsigned long high = 0, low = 0;

    __asm__ __volatile__ (
        "rdmsr\n\t"
        : "=a"(low), "=d"(high)
        : "c"(msr_addr)
        :
    );

    return low | ((unsigned long long)high << 0x20);
}

_DEFINE_INTRINSIC(void)
__writemsr(
    unsigned long msr_addr, 
    unsigned __int64 value)
{
    unsigned long high = (unsigned long)(value >> 0x20);
    unsigned long low = (unsigned long)(value & 0xffffffff);

    __asm__ __volatile__ (
        "wrmsr\n\t"
        : 
        : "c"(msr_addr), "a"(low), "d"(high)
        :
    );
}


_DEFINE_INTRINSIC(unsigned char)
__inbyte(
    unsigned short Port)
{
    unsigned char value = 0;
    __asm__ __volatile__ (
        "in %0, %1\n\t"
        : "=a"(value)
        : "d"(Port)
        :
    );

    return value;
}

_DEFINE_INTRINSIC(void)
__inbytestring(
    unsigned short Port,
    unsigned char* Buffer,
    unsigned long Count)
{
    __asm__ __volatile__ (
        "rep insb\n\t"
        : "=D"(Buffer), "=c"(Count)
        : "d"(Port), "c"(Count)
        :
    );
}

_DEFINE_INTRINSIC(unsigned short)
__inword(
    unsigned short Port)
{
    unsigned short value = 0;
    __asm__ __volatile__ (
        "in %0, %1\n\t"
        : "=a"(value)
        : "d"(Port)
        :
    );

    return value;
}

_DEFINE_INTRINSIC(void)
__inwordstring(
    unsigned short Port,
    unsigned short* Buffer,
    unsigned long Count)
{
    __asm__ __volatile__ (
        "rep insw\n\t"
        : "=D"(Buffer), "=c"(Count)
        : "d"(Port), "c"(Count)
        :
    );
}

_DEFINE_INTRINSIC(unsigned long)
__indword(
    unsigned short Port)
{
    unsigned long value = 0;
    __asm__ __volatile__ (
        "in %0, %1\n\t"
        : "=a"(value)
        : "d"(Port)
        :
    );

    return value;
}

_DEFINE_INTRINSIC(void)
__indwordstring(
    unsigned short Port,
    unsigned long* Buffer,
    unsigned long Count)
{
    __asm__ __volatile__ (
        "rep insd\n\t"
        : "=D"(Buffer), "=c"(Count)
        : "d"(Port), "c"(Count)
        :
    );
}


_DEFINE_INTRINSIC(void)
__outbyte( 
    unsigned short Port, 
    unsigned char Data)
{
    __asm__ __volatile__ (
        "out %0, %1\n\t"
        :
        : "d"(Port), "a"(Data)
        :
    );
}

_DEFINE_INTRINSIC(void)
__outbytestring( 
    unsigned short Port, 
    unsigned char* Buffer, 
    unsigned long Count)
{
    __asm__ __volatile__ (
        "rep outsb\n\t"
        : "=D"(Buffer), "=c"(Count)
        : "d"(Port), "S"(Buffer), "c"(Count)
        :
    );
}

_DEFINE_INTRINSIC(void)
__outword( 
    unsigned short Port, 
    unsigned short Data)
{
    __asm__ __volatile__ (
        "out %0, %1\n\t"
        :
        : "d"(Port), "a"(Data)
        :
    );
}

_DEFINE_INTRINSIC(void)
__outwordstring( 
    unsigned short Port, 
    unsigned short* Buffer, 
    unsigned long Count)
{
    __asm__ __volatile__ (
        "rep outsw\n\t"
        : "=D"(Buffer), "=c"(Count)
        : "d"(Port), "S"(Buffer), "c"(Count)
        :
    );
}

_DEFINE_INTRINSIC(void)
__outdword( 
    unsigned short Port, 
    unsigned long Data)
{
    __asm__ __volatile__ (
        "out %0, %1\n\t"
        :
        : "d"(Port), "a"(Data)
        : 
    );
}

_DEFINE_INTRINSIC(void)
__outdwordstring( 
    unsigned short Port, 
    unsigned long* Buffer, 
    unsigned long Count)
{
    __asm__ __volatile__ (
        "rep outsd\n\t"
        : "=D"(Buffer), "=c"(Count)
        : "d"(Port), "S"(Buffer), "c"(Count)
        :
    );
}


_DEFINE_INTRINSIC(long)
_InterlockedExchange(
    long volatile * Target,
    long Value)
{
    return __atomic_exchange_n(Target, Value, __ATOMIC_SEQ_CST);
    /*
    long Prev = 0;
    __asm__ __volatile__ (
        "xchg [%1], %2\n\t"
        : "=a"(Prev)
        : "r"(Target), "a"(Value)
        : "memory"
    );

    return Prev;
    */
}

_DEFINE_INTRINSIC(char)
_InterlockedExchange8(
    char volatile * Target,
    char Value)
{
    return __atomic_exchange_n(Target, Value, __ATOMIC_SEQ_CST);
    /*
    char Prev = 0;
    __asm__ __volatile__ (
        "xchg [%1], %2\n\t"
        : "=a"(Prev)
        : "r"(Target), "a"(Value)
        : "memory"
    );

    return Prev;
    */
}

_DEFINE_INTRINSIC(short)
_InterlockedExchange16(
    short volatile * Target,
    short Value)
{
    return __atomic_exchange_n(Target, Value, __ATOMIC_SEQ_CST);
    /*
    short Prev = 0;
    __asm__ __volatile__ (
        "xchg [%1], %2\n\t"
        : "=a"(Prev)
        : "r"(Target), "a"(Value)
        : "memory"
    );

    return Prev;
    */
}

_DEFINE_INTRINSIC(__int64)
_InterlockedExchange64(
    __int64 volatile * Target,
    __int64 Value)
{
    return __atomic_exchange_n(Target, Value, __ATOMIC_SEQ_CST);
    /*
    __int64 Prev = 0;
    __asm__ __volatile__ (
        "xchg [%1], %2\n\t"
        : "=a"(Prev)
        : "r"(Target), "a"(Value)
        : "memory"
    );

    return Prev;
    */
}

#define	_InterlockedExchangePointer		_InterlockedExchange64

_DEFINE_INTRINSIC(void)
_mm_lfence(
    void)
{
    __asm__ __volatile__ (
        "lfence\n\t"
        :
        :
        : "memory"
    );
}

_DEFINE_INTRINSIC(void)
_mm_sfence(
    void)
{
    __asm__ __volatile__ (
        "sfence\n\t"
        :
        :
        : "memory"
    );
}

_DEFINE_INTRINSIC(void)
_mm_mfence(
    void)
{
    __asm__ __volatile__ (
        "mfence\n\t"
        :
        :
        : "memory"
    );
}


_DEFINE_INTRINSIC(unsigned long long)
_readfsbase_u64(
    void)
{
    unsigned long long value = 0;
    __asm__ __volatile__ (
        "rdfsbase %0\n\t"
        : "=r"(value)
        :
        :
    );

    return value;
//    return __builtin_ia32_rdfsbase64();
}

_DEFINE_INTRINSIC(unsigned long long)
_readgsbase_u64(
    void)
{
    unsigned long long value = 0;
    __asm__ __volatile__ (
        "rdgsbase %0\n\t"
        : "=r"(value)
        :
        :
    );

    return value;

//    return __builtin_ia32_rdgsbase64();
}

_DEFINE_INTRINSIC(void)
_writefsbase_u64(
    unsigned long long value)
{
    __asm__ __volatile__ (
        "wrfsbase %0\n\t"
        : 
        : "r"(value)
        :
    );

//    __builtin_ia32_wrfsbase64(value);
}

_DEFINE_INTRINSIC(void)
_writegsbase_u64(
    unsigned long long value)
{
    __asm__ __volatile__ (
        "wrgsbase %0\n\t"
        : 
        : "r"(value)
        :
    );

//    __builtin_ia32_wrgsbase64(value);
}


/*

unsigned __int64 _readfsbase_u64(void );
unsigned __int64 _readgsbase_u64(void );

unsigned long long __builtin_ia32_rdfsbase64 (void);
unsigned long long __builtin_ia32_rdgsbase64 (void);
void _writefsbase_u64 (unsigned long long);
void _writegsbase_u64 (unsigned long long);
*/

#endif
