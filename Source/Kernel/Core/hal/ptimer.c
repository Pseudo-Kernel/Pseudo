
/**
 * @file ptimer.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements platform timer initialization and ISR.
 * @version 0.1
 * @date 2021-10-25
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <base/base.h>
#include <ke/ke.h>
#include <ke/kprocessor.h>
#include <mm/mm.h>
#include <mm/pool.h>
#include <init/bootgfx.h>
#include <init/preinit.h>
#include <hal/acpi.h>
#include <hal/apic.h>
#include <hal/ioapic.h>
#include <hal/halinit.h>
#include <hal/processor.h>
#include <hal/ptimer.h>
#include <hal/8254pit.h>


volatile U64 HalTickCount;

/**
 * @brief ISR for platform timer.
 * 
 * @param [in] Interrupt            Interrupt object.
 * @param [in] InterruptContext     Interrupt context.
 * @param [in] InterruptStackFrame  Interrupt stack frame.
 * 
 * @return Always InterruptAccepted.
 */
KINTERRUPT_RESULT
KERNELAPI
HalIsrPlatformTimer(
    IN PKINTERRUPT Interrupt,
    IN PVOID InterruptContext,
    IN PVOID InterruptStackFrame)
{
    HalTickCount++;
    HalApicSendEoi(HalApicBase);
    return InterruptAccepted;
}

/**
 * @brief Returns tick count.
 * 
 * @return 64-bit tick count.
 */
U64
KERNELAPI
HalGetTickCount(
    VOID)
{
    return HalTickCount;
}

/**
 * @brief Initializes the system timer.\n
 * 
 * @return ESTATUS code.
 * 
 * @todo If exists, use HPET instead.
 */
ESTATUS
KERNELAPI
HalInitializePlatformTimer(
    VOID)
{
    BGXTRACE_C(BGX_COLOR_LIGHT_YELLOW, "Setting system timer...\n");

    HAL_PRIVATE_DATA *PrivateData = HalGetPrivateData();

    BOOLEAN InterruptState = !!(__readeflags() & RFLAG_IF);
    _disable();

    // todo: Use HPET if exists
    ESTATUS Status = HalRegisterInterrupt(
        &PrivateData->InterruptObjects.PlatformTimer, 
        HalIsrPlatformTimer, NULL, VECTOR_PLATFORM_TIMER);

    if (E_IS_SUCCESS(Status))
    {
        U8 GSI = 0;
        DASSERT(HalLegacyIrqToGSI(ISA_IRQ_TIMER, &GSI));

        IOAPIC *IoApic = HalIoApicGetBlockByGSI(GSI);
        DASSERT(IoApic);

        U8 ApicId = HalApicGetId(HalApicBase);
        HalIoApicSetIoRedirectionByMask(
            IoApic, 
            GSI, 
            IOAPIC_RED_VECTOR(VECTOR_PLATFORM_TIMER) | 
            IOAPIC_RED_DESTINATION_FIELD(ApicId) | IOAPIC_RED_DEST_MODE_PHYSICAL | 
            IOAPIC_RED_DELIVERY_MODE(IOAPIC_RED_DELIVER_FIXED), 
            IOAPIC_RED_SETBIT_VECTOR | IOAPIC_RED_SETBIT_MASK_INT |
            IOAPIC_RED_SETBIT_DESTINATION | IOAPIC_RED_SETBIT_DESTINATION_MODE |
            IOAPIC_RED_SETBIT_DELIVERY_MODE);

        Hal_8254SetupTimer(1000);
    }

    if (InterruptState)
    {
        _enable();
    }

    return Status;
}
