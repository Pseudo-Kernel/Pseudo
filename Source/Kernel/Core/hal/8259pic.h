
#pragma once

// PIC Control (Master-PIC0, Slave-PIC1)

#define PIC0_COMMAND        0x20
#define PIC0_DATA           0x21
#define PIC1_COMMAND        0xa0
#define PIC1_DATA           0xa1


//
// Edge/Level trigerred register.
// (Each bit represents trigger mode; 0 = edge-triggered, 1 = level-triggered.)
//

#define PIC0_ELCR1          0x4d0 // [7:3] trigger mode for IRQ3..7 / [2:0] zero.
#define PIC1_ELCR2          0x4d1 // [7:6, 4:1] trigger mode for IRQ15..14, IRQ12..9 / [5, 0] zero.


//
// Initialization control word (ICW).
//

#define PIC_ICW1_DEFAULT    0x10 // default bit
#define PIC_ICW1_ICW4       0x01 // 1 - ICW4 needed, 0 - not needed
#define PIC_ICW1_SNGL       0x02 // 1 - single mode, 0 - cascade mode
#define PIC_ICW1_ADI        0x04 // call address interval, 1 - interval of 8, 0 - interval of 4
#define PIC_ICW1_LTIM       0x08 // 1 - level triggered mode, 0 - edge triggered mode

#define PIC_ICW4_UPM        0x01 // 1 - 8086/8088 mode, 0 - MCS-80/85 mode
#define PIC_ICW4_AEOI       0x02 // 1 - auto eoi, 0 - normal eoi
#define PIC_ICW4_MS         0x04 // BUF:MS 0:x - non-buffered-mode
#define PIC_ICW4_BUF        0x08 //        1:0 - buffered mode (slave), 1:1 - buffered mode (master)
#define PIC_ICW4_SFNM       0x10 // 1 - special fully nested mode, 0 - not special


//
// Operational control word (OCW).
//

#define PIC_OCW2_EOI        0x20
#define PIC_OCW3_READ_IRR   0x0a // read irr
#define PIC_OCW3_READ_ISR   0x0b // read isr

/*
    PIC_ICW1
    [0 | A7 | A6 | A5 | 1 | LTIM | ADI | SNGL | IC4]
*/



VOID
KERNELAPI
HalPicMaskInterrupt(
    IN U16 InterruptMask);

VOID
KERNELAPI
HalPicEnableInterrupt(
    IN U16 InterruptMask, 
    IN U8 VectorBase);

VOID
KERNELAPI
HalPicSendEoi(
    IN U8 ServicingVector);

U16
KERNELAPI
HalPicReadRegister(
    IN U8 Value);

BOOLEAN
KERNELAPI
HalPicGetInServicingVector(
    OUT U8 *ServicingVector);

BOOLEAN
KERNELAPI
HalPicInServicing(
    IN U8 ServicingVector);

U16
KERNELAPI
HalPicReadTriggerModeRegister(
    VOID);
