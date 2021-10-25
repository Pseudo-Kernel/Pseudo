
/**
 * @file halinit.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2021-10-16
 * 
 * @copyright Copyright (c) 2021
 * 
 * @todo Initialize local APIC and start processor.
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
#include <hal/halinit.h>
#include <hal/processor.h>
#include <hal/ptimer.h>


VIRTUAL_ADDRESS HalLowArea1MSpace;
AP_INIT_PACKET volatile *HalAPInitPacket;

VIRTUAL_ADDRESS HalApicBase;


VOID
KERNELAPI
HalPrepareAPStart(
    VOID)
{
    PTR ApicVirtualBase = 0;
    ESTATUS Status = MmAllocateVirtualMemory(NULL, &ApicVirtualBase, PAGE_SIZE, VadInUse);
    if (!E_IS_SUCCESS(Status))
    {
        FATAL("Failed to allocate memory for APIC\n");
    }

    PHYSICAL_ADDRESS LocalApicBase = (PHYSICAL_ADDRESS)(U32)HalAcpiMadt->LocalApicBase;

    Status = MmMapSinglePage(LocalApicBase, ApicVirtualBase, ARCH_X64_PXE_WRITABLE | ARCH_X64_PXE_CACHE_DISABLED);
    if (!E_IS_SUCCESS(Status))
    {
        FATAL("Failed to map memory for APIC\n");
    }

    //
    // Relocate 16-bit stub to low memory area.
    //

    PTR LowMemoryVirtual = 0;
    Status = MmAllocateVirtualMemory(NULL, &LowMemoryVirtual, 0x100000, VadInUse);
    if (!E_IS_SUCCESS(Status))
    {
        FATAL("Failed to allocate/map lower 1M area");
    }

    PHYSICAL_ADDRESSES PhysicalAddresses = 
    {
        .Mapped = FALSE,
        .PhysicalAddressCount = 1,
        .PhysicalAddresses[0].Range.Start = 0,
        .PhysicalAddresses[0].Range.End = 0x100000 - 1,
        .PhysicalAddressMaximumCount = 1,
    };

    Status = MmMapPages(&PhysicalAddresses, LowMemoryVirtual, ARCH_X64_PXE_WRITABLE, TRUE, 0);
    if (!E_IS_SUCCESS(Status))
    {
        FATAL("Failed to allocate/map lower 1M area");
    }

    AP_INIT_PACKET *Packet = (AP_INIT_PACKET *)(U64)&HalpApplicationProcessorInitStub;
    AP_INIT_PACKET *RelocatedPacket = (AP_INIT_PACKET *)(LowMemoryVirtual + HAL_PROCESSOR_RESET_ADDRESS);

    if (memcmp(Packet->Signature, "AP16INIT", 8))
    {
        FATAL("Corrupted AP16 stub");
    }

    memcpy(RelocatedPacket, Packet, Packet->PacketSize);

    BGXTRACE("Relocated AP init packet 0x%016llx\n", RelocatedPacket);

    HalLowArea1MSpace = LowMemoryVirtual;
    HalAPInitPacket = RelocatedPacket;
    HalApicBase = ApicVirtualBase;
}

VOID
KERNELAPI
HalPreInitialize(
    IN PVOID Rsdp)
{
    HalAcpiPreInitialize(Rsdp);
    HalPrepareAPStart();
}

VOID
KERNELAPI
HalInitialize(
    VOID)
{
    HalInitializeProcessor();

    HalStartProcessors();
}

