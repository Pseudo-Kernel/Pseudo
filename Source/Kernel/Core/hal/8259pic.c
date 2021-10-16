
/**
 * @file 8259pic.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements built-in driver (8259 PIC).
 * @version 0.1
 * @date 2021-10-10 (original date 201?-??-??)
 * 
 * @copyright Copyright (c) 2021
 * 
 */


#include <base/base.h>
#include <hal/8259pic.h>

/**
 * @brief Masks the interrupt for master/slave PIC.
 * 
 * @param [in] InterruptMask    16-bit interrupt mask for IRQ0 to IRQ15.\n
 *                              Each bit represents the interrupt mask state for corresponding IRQ.\n
 *                              (IRQ is disabled if bit is set.)\n
 *                              0xffff means all IRQs are disabled (IRQ0 to IRQ15).\n
 *                              0x0000 means all IRQs are enabled (IRQ0 to IRQ15).\n
 *                              0x9001 means IRQ0, IRQ12, IRQ15 are enabled.
 * 
 * @return None.
 */
VOID
KERNELAPI
HalPicMaskInterrupt(
    IN U16 InterruptMask)
{
    // 1 for interrupt disabled, 0 for enabled
    __outbyte(PIC0_DATA, (U8)(InterruptMask & 0xff));
    __outbyte(PIC1_DATA, (U8)(InterruptMask >> 0x08));
}

/**
 * @brief Initializes the master/slave PICs and masks interrupts.
 * 
 * @param [in] InterruptMask    16-bit interrupt mask for IRQ0 to IRQ15.\n
 *                              See HalPicMaskInterrupt().
 * @param [in] VectorBase       IDT base index for IRQ0.
 * 
 * @return None.
 */
VOID
KERNELAPI
HalPicEnableInterrupt(
    IN U16 InterruptMask, 
    IN U8 VectorBase)
{
    U64 Rflags = __readeflags();
    _disable();

    //
    // Disable all IRQs. (Master, Slave)
    //

    __outbyte(PIC0_DATA, 0xff);
    __outbyte(PIC1_DATA, 0xff);

    //
    // Send initialize command (code 0x11, ICW1) to PICs.
    // This command makes the PIC wait for 3 extra "initialization words" on the data port.
    // ICW2 - vector offset, ICW3 - master/slave state, ICW4 - additional modes
    //

    //
    // Initialize master PIC.
    // ICW2: Starting vector = VectorBase
    // ICW3: IR input 2 (mask 00000100 = 1 << 2) has a slave
    // ICW4: 8086/8088 mode
    //

    __outbyte(PIC0_COMMAND, PIC_ICW1_DEFAULT | PIC_ICW1_ICW4);
    __outbyte(PIC0_DATA, VectorBase); // ICW2
    __outbyte(PIC0_DATA, 0x04); // ICW3
    __outbyte(PIC0_DATA, PIC_ICW4_UPM); // ICW4

    //
    // Initialize slave PIC.
    // ICW2: Starting vector = VectorBase+8
    // ICW3: SlaveID is equal to the corrensponding master IR input (2 = 00000010)
    // ICW4: 8086/8088 mode
    //

    __outbyte(PIC1_COMMAND, PIC_ICW1_DEFAULT | PIC_ICW1_ICW4);
    __outbyte(PIC1_DATA, VectorBase + 8);
    __outbyte(PIC1_DATA, 0x02);
    __outbyte(PIC1_DATA, PIC_ICW4_UPM);


    //
    // Mask interrupt as specified
    //

    __outbyte(PIC0_DATA, (U8)(InterruptMask & 0xff));
    __outbyte(PIC1_DATA, (U8)(InterruptMask >> 0x08));

    __writeeflags(Rflags);
}

/**
 * @brief Sends the EOI message to PIC.
 * 
 * @param [in] ServicingVector  Currently servicing IRQ number (0 to 15).
 * 
 * @return None.
 */
VOID
KERNELAPI
HalPicSendEoi(
    IN U8 ServicingVector)
{
    if (ServicingVector >= 0x08)
    {
        __outbyte(PIC1_COMMAND, PIC_OCW2_EOI);
    }

    // We also need to send EOI to master PIC if it is in slave mode
    __outbyte(PIC0_COMMAND, PIC_OCW2_EOI);
}

/**
 * @brief Reads the 8259 master/slave PIC registers.
 * 
 * @param [in] Value    OCW3 command word.
 * 
 * @return Returns 16-bit result.\n
 *         Lower 8-bit is master result, Higher 8-bit is slave result.
 */
U16
KERNELAPI
HalPicReadRegister(
    IN U8 Value)
{
    //
    // Write OCW3 to CMD port.
    //

    __outbyte(PIC0_COMMAND, Value);
    __outbyte(PIC1_COMMAND, Value);

    //
    // Read it back from CMD, not DATA.
    //

    return (__inbyte(PIC1_COMMAND) << 8) | __inbyte(PIC0_COMMAND);
}

/**
 * @brief Gets in-servicing IRQ number.
 * 
 * @param [out] ServicingVector     Currently servicing IRQ number (0 to 15).
 * 
 * @return TRUE if in-servicing IRQ exists, FALSE otherwise (no IRQ is in servicing).
 */
BOOLEAN
KERNELAPI
HalPicGetInServicingVector(
    OUT U8 *ServicingVector)
{
    U16 InServiceRegister = HalPicReadRegister(PIC_OCW3_READ_ISR);

    for (U8 Vector = 0; Vector < 16; Vector++)
    {
        if (InServiceRegister & (1 << Vector))
        {
            *ServicingVector = Vector;
        }
    }

    return (InServiceRegister != 0);
}

/**
 * @brief Tests whether the given IRQ is in servicing.
 * 
 * @param [in] ServicingVector      IRQ number (0 to 15) to test in-servicing state.
 * 
 * @return TRUE if IRQ is in servicing, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
HalPicInServicing(
    IN U8 ServicingVector)
{
    U16 InServiceRegister = HalPicReadRegister(PIC_OCW3_READ_ISR);

    DASSERT(0 <= ServicingVector && ServicingVector <= 0x0f);

    return (InServiceRegister & (1 << ServicingVector)) != 0;
}


/**
 * @brief Reads edge/level triggered register (ELCR).
 * 
 * @return 16-bit trigger mode for IRQ0..15.\n
 *         Each bit represents trigger mode for corresponding IRQ.\n
 *         For example, IRQ0 is level-triggered if LSB is set.\n
 *                      IRQ15 is edge-triggered if MSB is cleared.\n
 */
U16
KERNELAPI
HalPicReadTriggerModeRegister(
    VOID)
{
    return (__inbyte(PIC1_ELCR2) << 8) | __inbyte(PIC0_ELCR1);
}
