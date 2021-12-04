
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
#include <ke/sched.h>
#include <mm/mm.h>
#include <mm/pool.h>
#include <init/bootgfx.h>
#include <hal/acpi.h>
#include <hal/apic.h>


VOID
KERNELAPI
KiTestProcessorFeature(
    VOID)
{
    int Info[4];

    __cpuid(Info, 0x80000001);
    if (!(Info[3] & (1 << 27)))
    {
        FATAL("Processor does not support RDTSCP");
    }

    __cpuid(Info, 0x80000007);
    if (!(Info[3] & (1 << 8)))
    {
        FATAL("Processor does not support invariant TSC");
    }
}

VOID
KERNELAPI
KiInitialize(
    VOID)
{
//    KiTestProcessorFeature();

    for (ULONG i = 0; i < 0x100; i++)
    {
        KiApicIdToProcessorId[i] = PROCESSOR_INVALID_MAPPING;
        KiProcessorIdToApicId[i] = PROCESSOR_INVALID_MAPPING;
    }

    KiInitializeProcessor();
    KiInitializeIrqGroups();
    KiProcessorSchedInitialize();
}

