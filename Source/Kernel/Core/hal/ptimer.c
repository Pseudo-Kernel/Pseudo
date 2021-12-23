
/**
 * @file ptimer.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements platform timer initialization.
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
#include <hal/hpet.h>


volatile U64 HalTickCount;

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
 * @brief Returns timer frequency.
 * 
 * @param [out] Frequency       Counter per second.
 * 
 * @return ESTATUS code.
 */
ESTATUS
KERNELAPI
HalTimerGetFrequency(
    OUT U64 *Frequency)
{
    return HalHpetGetFrequency(Frequency);
}

/**
 * @brief Returns 64-bit counter.
 * 
 * @param [out] Counter     Current counter.
 * 
 * @return ESTATUS code.
 */
ESTATUS
KERNELAPI
HalTimerReadCounter(
    OUT U64 *Counter)
{
    return HalHpetReadCounter(Counter);
}

/**
 * @brief Initializes the system timer.\n
 * 
 * @return ESTATUS code.
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

    ESTATUS Status = HalHpetInitialize();

    if (InterruptState)
    {
        _enable();
    }

    return Status;
}
