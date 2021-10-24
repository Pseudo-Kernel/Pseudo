
#pragma once

#include <base/base.h>
#include <ke/lock.h>

typedef struct _IOAPIC
{
    U64 PhysicalBase;   // Physical base address
    U64 VirtualBase;    // Virtual base address

    U8 IoApicId;        // IOAPIC id
    U8 IoApicArbId;
    U8 Version;         // IOAPIC version
    BOOLEAN IsValid;    // Valid if TRUE

    // GSI range. [GSILimit:GSIBase].
    U32 GSIBase;        // Base GSI (Global System Interrupt).
    U32 GSILimit;       // GSI Limit.

    KSPIN_LOCK Lock;    // IOAPIC Lock.
} IOAPIC;




#define IOAPIC_BASE_FROM(_base, _x)         ((PTR)(_base) + (PTR)(_x))


#define IOREGSEL(_base)                     IOAPIC_BASE_FROM(_base, 0x00)
#define IOREGWIN(_base)                     IOAPIC_BASE_FROM(_base, 0x10)

#define IOAPICID                            0x00 // R/W
#define IOAPICVER                           0x01 // R
#define IOAPICARB                           0x02 // R

#define IOAPIC_ID_GET_ID(_v)                (((U32)(_v) >> 24) & 0xf)
#define IOAPIC_VER_GET_MAX_REDIRENT(_v)     (((U32)(_v) >> 16) & 0xff)
#define IOAPIC_VER_GET_VERSION(_v)          (((U32)(_v) >> 0) & 0xff)
#define IOAPIC_ARB_GET_ARB_ID(_v)           IOAPIC_ID_GET_ID(_v)


//
// I/O redirection table register (IOREDTBLn).
//

#define IOREDTBL_LOW(_vec)      ((_vec)*2 + IOREDTBL0L)
#define IOREDTBL_HIGH(_vec)     ((_vec)*2 + IOREDTBL0H)

#define IOREDTBL0L              0x10
#define IOREDTBL0H              0x11
#define IOREDTBL1L              0x12
#define IOREDTBL1H              0x13
#define IOREDTBL2L              0x14
#define IOREDTBL2H              0x15
#define IOREDTBL3L              0x16
#define IOREDTBL3H              0x17
#define IOREDTBL4L              0x18
#define IOREDTBL4H              0x19
#define IOREDTBL5L              0x1a
#define IOREDTBL5H              0x1b
#define IOREDTBL6L              0x1c
#define IOREDTBL6H              0x1d
#define IOREDTBL7L              0x1e
#define IOREDTBL7H              0x1f

#define IOREDTBL8L              0x20
#define IOREDTBL8H              0x21
#define IOREDTBL9L              0x22
#define IOREDTBL9H              0x23
#define IOREDTBL10L             0x24
#define IOREDTBL10H             0x25
#define IOREDTBL11L             0x26
#define IOREDTBL11H             0x27
#define IOREDTBL12L             0x28
#define IOREDTBL12H             0x29
#define IOREDTBL13L             0x2a
#define IOREDTBL13H             0x2b
#define IOREDTBL14L             0x2c
#define IOREDTBL14H             0x2d
#define IOREDTBL15L             0x2e
#define IOREDTBL15H             0x2f

#define IOREDTBL16L             0x30
#define IOREDTBL16H             0x31
#define IOREDTBL17L             0x32
#define IOREDTBL17H             0x33
#define IOREDTBL18L             0x34
#define IOREDTBL18H             0x35
#define IOREDTBL19L             0x36
#define IOREDTBL19H             0x37
#define IOREDTBL20L             0x38
#define IOREDTBL20H             0x39
#define IOREDTBL21L             0x3a
#define IOREDTBL21H             0x3b
#define IOREDTBL22L             0x3c
#define IOREDTBL22H             0x3d
#define IOREDTBL23L             0x3e
#define IOREDTBL23H             0x3f




//
// IOAPIC, REDTBL.
// Similar to ICR of Local APIC.
//

//
// Vector (REDTBL[7:0])
//

#define IOAPIC_RED_VECTOR(_v)               ((_v) & 0xff)

//
// Delivery mode (REDTBL[10:8])
//

#define IOAPIC_RED_DELIVERY_MODE(_v)        (((_v) & 7) << 8)
#define IOAPIC_RED_DELIVER_FIXED            0
#define IOAPIC_RED_DELIVER_LOWEST_PRI       1
#define IOAPIC_RED_DELIVER_SMI              2
#define IOAPIC_RED_DELIVER_NMI              4
#define IOAPIC_RED_DELIVER_INIT             5
#define IOAPIC_RED_DELIVER_EXTINT           7

//
// Destination mode (REDTBL[11])
//

#define IOAPIC_RED_DEST_MODE_LOGICAL        (1 << 11)
#define IOAPIC_RED_DEST_MODE_PHYSICAL       0

//
// Delivery status (Read only, REDTBL[12])
//

#define IOAPIC_RED_DELIVER_PENDING          (1 << 12)

//
// Interrupt input pin polarity (REDTBL[13])
//

#define IOAPIC_RED_POLARITY_LOW_ACTIVE      (1 << 13)
#define IOAPIC_RED_POLARITY_HIGH_ACTIVE     0

//
// Remote IRR (Read only, REDTBL[14])
// - undefined for edge triggered interrupts.
// - for level triggered, 1 means accept
//

#define IOAPIC_RED_REMOTE_IRR               (1 << 14)

//
// Trigger mode (REDTBL[15])
//

#define IOAPIC_RED_TRIGGERED_LEVEL          (1 << 15)
#define IOAPIC_RED_TRIGGERED_EDGE           0

//
// Interrupt mask (REDTBL[16])
//

#define IOAPIC_RED_INTERRUPT_MASKED         (1 << 16)

//
// Destination field (REDTBL[63:56] / REDTBL[59:56])
// - REDTBL[63:56] when logical mode.
// - REDTBL[59:56] when physical mode
//

#define IOAPIC_RED_HIGH32_DESTINATION_FIELD(_v) ((_v) << (56-32))
#define IOAPIC_RED_DESTINATION_FIELD(_v)        ((U64)(_v) << 56)

//
// REDirection.
//

#define IOAPIC_RED_SETBIT_VECTOR                0xff
#define IOAPIC_RED_SETBIT_MASK_INT              IOAPIC_RED_INTERRUPT_MASKED
#define IOAPIC_RED_SETBIT_DESTINATION           IOAPIC_RED_DESTINATION_FIELD(0xff)

#define IOAPIC_RED_SETBIT_DELIVERY_MODE         IOAPIC_RED_DELIVERY_MODE(7)
#define IOAPIC_RED_SETBIT_DESTINATION_MODE      IOAPIC_RED_DEST_MODE_LOGICAL
#define IOAPIC_RED_SETBIT_POLARITY              IOAPIC_RED_POLARITY_LOW_ACTIVE
#define IOAPIC_RED_SETBIT_TRIGGERED             IOAPIC_RED_TRIGGERED_LEVEL



IOAPIC *
KERNELAPI
HalIoApicAdd(
    IN U32 GSIBase,
    IN PHYSICAL_ADDRESS IoApicPhysicalBase);

U32
KERNELAPI
HalIoApicRead(
    IN PTR IoApicBase, 
    IN U32 Register);

U32
KERNELAPI
HalIoApicWrite(
    IN PTR IoApicBase, 
    IN U32 Register, 
    IN U32 Value);

VOID
KERNELAPI
HalIoApicMaskInterrupt(
    IN IOAPIC *IoApic, 
    IN U32 IntIn, 
    IN BOOLEAN Mask, 
    OUT BOOLEAN *PrevMaskedState);

U64
KERNELAPI
HalIoApicSetIoRedirectionByMask(
    IN IOAPIC *IoApic, 
    IN U32 IntIn, 
    IN U64 RedirectionEntry, 
    IN U64 SetMask);

U64
KERNELAPI
HalIoApicSetIoRedirection(
    IN IOAPIC *IoApic, 
    IN U32 IntIn, 
    IN U64 RedirectionEntry);

IOAPIC *
KERNELAPI
HalIoApicGetBlock(
    IN U32 IoApicId);

IOAPIC *
KERNELAPI
HalIoApicGetBlockByGSI(
    IN U32 GSI);

