
/**
 * @file keinit.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements kernel initialization routines.
 * @version 0.1
 * @date 2021-09-26
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <base/base.h>
#include <ke/lock.h>
#include <ke/keinit.h>
#include <ke/interrupt.h>
#include <ke/kprocessor.h>
#include <mm/mm.h>
#include <mm/pool.h>
#include <init/bootgfx.h>
#include <hal/acpi.h>
#include <hal/apic.h>


VOID
KERNELAPI
KiInitialize(
    VOID)
{
    for (ULONG i = 0; i < 0x100; i++)
    {
        KiApicIdToProcessorId[i] = PROCESSOR_INVALID_MAPPING;
        KiProcessorIdToApicId[i] = PROCESSOR_INVALID_MAPPING;
    }

    KiInitializeProcessor();
    KiInitializeIrqGroups();
}

