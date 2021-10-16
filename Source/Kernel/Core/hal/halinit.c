

#include <base/base.h>
#include <ke/ke.h>
#include <mm/mm.h>
#include <mm/pool.h>
#include <init/bootgfx.h>
#include <init/preinit.h>
#include <hal/acpi.h>
#include <hal/ioapic.h>
#include <hal/halinit.h>


extern U8 HalLegacyIrqToGSIMappings[16];
extern U8 HalGSIToLegacyIrqMappings[256];


VOID
KERNELAPI
HalPreInitialize(
    IN PVOID Rsdp)
{
    ESTATUS Status = HalAcpiPreInitialize(Rsdp);

    if (!E_IS_SUCCESS(Status))
    {
        FATAL("ACPI failure");
    }


}
