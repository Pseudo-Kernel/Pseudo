
/**
 * @file acpi.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements ACPI table read for initialization.
 * @version 0.1
 * @date 2021-10-14
 * 
 * @copyright Copyright (c) 2021
 * 
 * @note This code assumes that ACPI tables are identity mapped.
 * 
 * @todo Implement helper routines for MADT structure.
 */


#include <base/base.h>
#include <ke/ke.h>
#include <mm/mm.h>
#include <mm/pool.h>
#include <init/bootgfx.h>
#include <init/preinit.h>
#include <init/acpi.h>


ACPI_XSDT *PiAcpiXsdt;
//U64 PiAcpiOffsetToVirtualAddress;

BOOLEAN
KERNELAPI
PiAcpiChecksumDescriptor(
    IN PVOID Descriptor, 
    IN ULONG DescriptorSize)
{
    U8 Checksum = 0;
    for (ULONG i = 0; i < DescriptorSize; i++)
    {
        Checksum += *((U8 *)Descriptor + i);
    }

    return !Checksum;
}

ACPI_XSDT *
KERNELAPI
PiAcpiValidateXSDT(
    IN ACPI_ROOT_POINTER *Rsdp)
{
    ACPI_ROOT_POINTER_EXT *RSDPExt = (ACPI_ROOT_POINTER_EXT *)Rsdp;

    if (!Rsdp->Revision)
    {
        // ACPI 2.0+ is required
        return NULL;
    }

    ACPI_XSDT *Xsdt = (ACPI_XSDT *)RSDPExt->XSDT;

    if (memcmp(Xsdt->Header.Signature, ACPI_XSDT_SIGNATURE, sizeof(Xsdt->Header.Signature)) || 
        !PiAcpiChecksumDescriptor(&Xsdt->Header, Xsdt->Header.Length))
    {
        // Checksum failed
        return NULL;
    }

    return Xsdt;
}

ACPI_DESCRIPTION_HEADER *
KERNELAPI
PiAcpiLookupDescriptionPointer(
    IN ACPI_XSDT *Xsdt,
    IN const CHAR *Signature)
{
    ULONG EntryCount = (Xsdt->Header.Length - sizeof(Xsdt->Header)) / sizeof(U64);
    for (ULONG i = 0; i < EntryCount; i++)
    {
        ACPI_DESCRIPTION_HEADER *DescriptionHeader = (ACPI_DESCRIPTION_HEADER *)Xsdt->Entry[i];

        if (!memcmp(DescriptionHeader->Signature, Signature, sizeof(DescriptionHeader->Signature)))
            return DescriptionHeader;
    }

    return NULL;
}

ESTATUS
KERNELAPI
PiAcpiPreInitialize(
    IN PVOID Rsdp)
{
    BGXTRACE("\nChecking ACPI info...\n");

    ACPI_XSDT *Xsdt = PiAcpiValidateXSDT(Rsdp);

    if (!Xsdt)
    {
        BGXTRACE("Failed to find XSDT\n");
        return E_FAILED;
    }

    BGXTRACE("Detected ACPI 2.0 or higher\n");

    ULONG EntryCount = (Xsdt->Header.Length - sizeof(Xsdt->Header)) / sizeof(U64);
    for (ULONG i = 0; i < EntryCount; i++)
    {
        ACPI_DESCRIPTION_HEADER *DescriptionHeader = (ACPI_DESCRIPTION_HEADER *)Xsdt->Entry[i];

        if (!PiAcpiChecksumDescriptor(DescriptionHeader, DescriptionHeader->Length))
        {
            BGXTRACE("Corrupted ACPI table (Checksum mismatch at description header 0x%016llx\n", DescriptionHeader);
            return E_FAILED;
        }

        BGXTRACE("Header 0x%016llx, signature `%c%c%c%c'\n", 
            DescriptionHeader,
            DescriptionHeader->Signature[0], 
            DescriptionHeader->Signature[1], 
            DescriptionHeader->Signature[2], 
            DescriptionHeader->Signature[3]);
    }

    ACPI_FADT *Fadt = (ACPI_FADT *)PiAcpiLookupDescriptionPointer(Xsdt, ACPI_FADT_SIGNATURE);

    if (!Fadt)
    {
        BGXTRACE("FADT not exists!\n");
        return E_FAILED;
    }

    BGXTRACE_C(BGX_COLOR_LIGHT_CYAN, "FADT version %d.%d\n", Fadt->Header.Revision, Fadt->FadtMinorVersion);

    ACPI_MADT *Madt = (ACPI_MADT *)PiAcpiLookupDescriptionPointer(Xsdt, ACPI_MADT_SIGNATURE);

    if (!Madt)
    {
        BGXTRACE("MADT not exists!\n");
        return E_FAILED;
    }

    BGXTRACE("\n");

    PiAcpiXsdt = Xsdt;

    return E_SUCCESS;
}
