
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
 *       Initialize local APIC and start processor.
 */


#include <base/base.h>
#include <ke/ke.h>
#include <mm/mm.h>
#include <mm/pool.h>
#include <init/bootgfx.h>
#include <init/preinit.h>
#include <hal/acpi.h>
#include <hal/8259pic.h>
#include <hal/ioapic.h>
#include <hal/apic.h>
#include <hal/apstub16.h>


ACPI_XSDT *HalAcpiXsdt;
ACPI_MADT *HalAcpiMadt;

U8 HalLegacyIrqToGSIMappings[16];
U8 HalGSIToLegacyIrqMappings[256];


/**
 * @brief Converts the PIC IRQ (also known as ISA IRQ) to ACPI GSI (Global System Interrupt).
 * 
 * @param [in] IsaIrq       PIC IRQ.
 * @param [out] GSI         Converted GSI number.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
HalLegacyIrqToGSI(
    IN U8 IsaIrq,
    OUT U8 *GSI)
{
    if (0 <= IsaIrq && IsaIrq < COUNTOF(HalLegacyIrqToGSIMappings))
    {
        *GSI = HalLegacyIrqToGSIMappings[IsaIrq];
        return TRUE;
    }
    
    // ISA IRQ must be in range of 0..15
    return FALSE;
}

/**
 * @brief Converts the ACPI GSI (Global System Interrupt) to PIC IRQ.
 * 
 * @param [in] GSI          GSI number.
 * @param [out] IsaIrq      Converted PIC IRQ.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
HalGSIToLegacyIrq(
    IN U8 GSI,
    OUT U8 *IsaIrq)
{
    if (0 <= GSI && GSI < COUNTOF(HalGSIToLegacyIrqMappings))
    {
        *IsaIrq = HalGSIToLegacyIrqMappings[GSI];
        return TRUE;
    }
    
    return FALSE;
}

/**
 * @brief Validates the descriptor checksum.
 * 
 * @param [in] Descriptor       Pointer to the descriptor.
 * @param [in] DescriptorSize   Size of descriptor.
 * 
 * @return TRUE if checksum is valid, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
HalAcpiChecksumDescriptor(
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

/**
 * @brief Validates the XSDT.
 * 
 * @param [in] Rsdp         Pointer to ACPI_ROOT_POINTER structure.
 * 
 * @return Pointer to ACPI_XSDT if succeeds.\n
 *         If validation fails, this function returns NULL.
 */
ACPI_XSDT *
KERNELAPI
HalAcpiValidateXSDT(
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
        !HalAcpiChecksumDescriptor(&Xsdt->Header, Xsdt->Header.Length))
    {
        // Checksum failed
        return NULL;
    }

    return Xsdt;
}

/**
 * @brief Finds the description pointer by signature.
 * 
 * @param [in] Xsdt         Pointer to ACPI_XSDT.
 * @param [in] Signature    4-byte ascii string.
 * 
 * @return Pointer to ACPI_DESCRIPTION_HEADER if succeeds.\n
 *         If validation fails, this function returns NULL.
 */
ACPI_DESCRIPTION_HEADER *
KERNELAPI
HalAcpiLookupDescriptionPointer(
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

/**
 * @brief Do ACPI pre-initialization.\n
 * 
 * @param [in] Rsdp     Pointer to ACPI_ROOT_POINTER.
 * 
 * @return None.
 */
VOID
KERNELAPI
HalAcpiPreInitialize(
    IN PVOID Rsdp)
{
    // Mask ISA IRQs before initialize IOAPIC.
    HalPicMaskInterrupt(~0);

    BGXTRACE("\nChecking ACPI info...\n");

    ACPI_XSDT *Xsdt = HalAcpiValidateXSDT(Rsdp);

    if (!Xsdt)
    {
        FATAL("Failed to find XSDT\n");
    }

    BGXTRACE("Detected ACPI 2.0 or higher\n");

    ULONG EntryCount = (Xsdt->Header.Length - sizeof(Xsdt->Header)) / sizeof(U64);
    for (ULONG i = 0; i < EntryCount; i++)
    {
        ACPI_DESCRIPTION_HEADER *DescriptionHeader = (ACPI_DESCRIPTION_HEADER *)Xsdt->Entry[i];

        if (!HalAcpiChecksumDescriptor(DescriptionHeader, DescriptionHeader->Length))
        {
            FATAL("Corrupted ACPI table (Checksum mismatch at description header 0x%016llx\n", DescriptionHeader);
        }

        BGXTRACE("Header 0x%016llx, signature `%c%c%c%c'\n", 
            DescriptionHeader,
            DescriptionHeader->Signature[0], 
            DescriptionHeader->Signature[1], 
            DescriptionHeader->Signature[2], 
            DescriptionHeader->Signature[3]);
    }

    ACPI_FADT *Fadt = (ACPI_FADT *)HalAcpiLookupDescriptionPointer(Xsdt, ACPI_FADT_SIGNATURE);

    if (!Fadt)
    {
        FATAL("FADT not exists!\n");
    }

    BGXTRACE_C(BGX_COLOR_LIGHT_CYAN, "FADT version %d.%d\n", Fadt->Header.Revision, Fadt->FadtMinorVersion);

    ACPI_MADT *Madt = (ACPI_MADT *)HalAcpiLookupDescriptionPointer(Xsdt, ACPI_MADT_SIGNATURE);

    if (!Madt)
    {
        FATAL("MADT not exists!\n");
    }

    if (!Madt->PCAT_COMPAT)
    {
        FATAL("8259 PIC not exists!\n");
    }

    ACPI_MADT_RECORD_HEADER *FirstRecord = (ACPI_MADT_RECORD_HEADER *)(Madt + 1);
    PTR MadtEnd = (PTR)Madt + Madt->Header.Length;

    //
    // Initialize ISA IRQ <-> GSI mappings (Identity mapping).
    //

    for (ULONG i = 0; i < COUNTOF(HalLegacyIrqToGSIMappings); i++)
        HalLegacyIrqToGSIMappings[i] = i;

    for (ULONG i = 0; i < COUNTOF(HalGSIToLegacyIrqMappings); i++)
    {
        if (i < COUNTOF(HalLegacyIrqToGSIMappings))
            HalGSIToLegacyIrqMappings[i] = i;
        else
            HalGSIToLegacyIrqMappings[i] = -1;
    }

    //
    // Initialize IOAPIC with default settings.
    //

    ACPI_MADT_RECORD_HEADER *Record = FirstRecord;
    while ((PTR)Record < MadtEnd)
    {
        if (Record->EntryType == ACPI_MADT_RECORD_IOAPIC)
        {
            ACPI_IOAPIC *IoApicRecord = (ACPI_IOAPIC *)Record;
            IOAPIC *IoApic = HalIoApicAdd(IoApicRecord->GSI_Base, IoApicRecord->IoApicAddress);

            DASSERT(IoApic != NULL);

            BGXTRACE("IOAPIC Id %d, Version 0x%02x, GSI %d..%d, PhysicalBase 0x%016llx, VirtualBase 0x%016llx\n", 
                IoApic->IoApicId, IoApic->Version, IoApic->GSIBase, IoApic->GSILimit, 
                IoApic->PhysicalBase, IoApic->VirtualBase);

            if (IoApicRecord->GSI_Base == 0)
            {
                // Map ISA IRQ to GSI (Identity mapping).
                DASSERT(IoApic->GSILimit >= 15);

                U16 TriggerMode = HalPicReadTriggerModeRegister();

                for (ULONG i = 0; i <= 15; i++)
                {
                    // default: active high, edge-triggered

                    // Fixed delivery mode, physical mode, masked
                    U64 RedirectionEntry = 
                        IOAPIC_RED_DELIVERY_MODE(IOAPIC_RED_DELIVER_FIXED) |
                        IOAPIC_RED_DEST_MODE_PHYSICAL |
                        IOAPIC_RED_INTERRUPT_MASKED;
                    U64 Mask = 
                        IOAPIC_RED_DELIVERY_MODE_MASK | 
                        IOAPIC_RED_POLARITY_MASK | 
                        IOAPIC_RED_TRIGGERED_MASK | 
                        IOAPIC_RED_DEST_MODE_MASK |
                        IOAPIC_RED_INTERRUPT_MASKED;

                    if (TriggerMode & (1 << i))
                    {
                        // level-triggered, active low
                        RedirectionEntry |= IOAPIC_RED_POLARITY_LOW_ACTIVE | IOAPIC_RED_TRIGGERED_LEVEL;
                    }
                    else
                    {
                        // edge-triggered, active high
                        RedirectionEntry |= IOAPIC_RED_POLARITY_HIGH_ACTIVE | IOAPIC_RED_TRIGGERED_EDGE;
                    }

                    HalIoApicSetIoRedirectionByMask(IoApic, i, RedirectionEntry, Mask);
                }
            }
        }

        Record = (ACPI_MADT_RECORD_HEADER *)((PTR)Record + Record->RecordLength);
    }


    //
    // Override ISA IRQs by interrupt source override records.
    //

    Record = FirstRecord;
    while ((PTR)Record < MadtEnd)
    {
        if (Record->EntryType == ACPI_MADT_RECORD_INTSRC_OVERRIDE)
        {
            ACPI_INTERRUPT_SOURCE_OVERRIDE *Override = (ACPI_INTERRUPT_SOURCE_OVERRIDE *)Record;

            DASSERT(Override->BusSource == 0);

            IOAPIC *IoApic = HalIoApicGetBlockByGSI(Override->GSI);

            DASSERT(IoApic != NULL);

            BGXTRACE("ISA IRQ %d -> GSI %d, MPS INTI Flags 0x%04x\n", 
                Override->Irq, Override->GSI, Override->Flags.Value);

            if (IoApic->GSIBase == 0)
            {
                DASSERT(IoApic->GSILimit >= 15 && Override->Irq <= 15);

                U16 TriggerMode = HalPicReadTriggerModeRegister();

                U64 RedirectionEntry = 0;
                U64 Mask = IOAPIC_RED_POLARITY_MASK | IOAPIC_RED_TRIGGERED_MASK;

                switch (Override->Flags.Polarity)
                {
                case ACPI_INT_OVERRIDE_POLARITY_CONFORM_SPEC:
                {
                    // Conforms to the specifications of the bus
                    if (TriggerMode & (1 << Override->Irq))
                    {
                        RedirectionEntry |= IOAPIC_RED_POLARITY_LOW_ACTIVE;
                    }
                    else
                    {
                        RedirectionEntry |= IOAPIC_RED_POLARITY_HIGH_ACTIVE;
                    }
                    break;
                }
                case ACPI_INT_OVERRIDE_POLARITY_ACTIVE_LOW:
                {
                    RedirectionEntry |= IOAPIC_RED_POLARITY_LOW_ACTIVE;
                    break;
                }
                case ACPI_INT_OVERRIDE_POLARITY_ACTIVE_HIGH:
                {
                    RedirectionEntry |= IOAPIC_RED_POLARITY_HIGH_ACTIVE;
                    break;
                }
                }

                switch (Override->Flags.TriggerMode)
                {
                case ACPI_INT_OVERRIDE_TRIG_CONFORM_SPEC:
                {
                    // Conforms to the specifications of the bus
                    if (TriggerMode & (1 << Override->Irq))
                    {
                        RedirectionEntry |= IOAPIC_RED_TRIGGERED_LEVEL;
                    }
                    else
                    {
                        RedirectionEntry |= IOAPIC_RED_TRIGGERED_EDGE;
                    }
                    break;
                }
                case ACPI_INT_OVERRIDE_TRIG_EDGE:
                {
                    RedirectionEntry |= IOAPIC_RED_TRIGGERED_EDGE;
                    break;
                }
                case ACPI_INT_OVERRIDE_TRIG_LEVEL:
                {
                    RedirectionEntry |= IOAPIC_RED_TRIGGERED_LEVEL;
                    break;
                }
                }

                // GSI == INTIN if GSIBase is zero.
                // We'll only set polarity and trigger mode.
                HalIoApicSetIoRedirectionByMask(IoApic, Override->GSI, RedirectionEntry, Mask);

                // Remove previous identity mappings.
                HalLegacyIrqToGSIMappings[Override->GSI] = -1;
                HalGSIToLegacyIrqMappings[Override->Irq] = -1;

                // Set new ISA IRQ <-> GSI mappings.
                HalLegacyIrqToGSIMappings[Override->Irq] = Override->GSI;
                HalGSIToLegacyIrqMappings[Override->GSI] = Override->Irq;
            }
        }

        Record = (ACPI_MADT_RECORD_HEADER *)((PTR)Record + Record->RecordLength);
    }

    BGXTRACE("\n");

    HalAcpiXsdt = Xsdt;
    HalAcpiMadt = Madt;
}

/**
 * @brief Returns first local apic structure.
 * 
 * @param [in] Madt             Pointer to ACPI_MADT.
 * 
 * @return Returns pointer to ACPI_LOCAL_APIC if succeeds.\n
 *         This function returns NULL if it fails.
 */
ACPI_LOCAL_APIC *
KERNELAPI
HalAcpiGetFirstProcessor(
    IN ACPI_MADT *Madt)
{
    ACPI_MADT_RECORD_HEADER *Record = (ACPI_MADT_RECORD_HEADER *)(Madt + 1);
    PTR MadtEnd = (PTR)Madt + Madt->Header.Length;

    while ((PTR)Record < MadtEnd)
    {
        if (Record->EntryType == ACPI_MADT_RECORD_LOCAL_APIC)
        {
            return (ACPI_LOCAL_APIC *)Record;
        }

        Record = (ACPI_MADT_RECORD_HEADER *)((PTR)Record + Record->RecordLength);
    }

    return NULL;
}

/**
 * @brief Returns next local apic structure.
 * 
 * @param [in] Madt             Pointer to ACPI_MADT.
 * @param [in] ProcessorRecord  Pointer to ACPI_LOCAL_APIC from previous call.
 * 
 * @return Returns pointer to ACPI_LOCAL_APIC if succeeds.\n
 *         This function returns NULL if there are no more apic structures to return.
 */
ACPI_LOCAL_APIC *
KERNELAPI
HalAcpiGetNextProcessor(
    IN ACPI_MADT *Madt,
    IN ACPI_LOCAL_APIC *ProcessorRecord)
{
    ACPI_MADT_RECORD_HEADER *Record = &ProcessorRecord->Header;
    PTR MadtEnd = (PTR)Madt + Madt->Header.Length;

    Record = (ACPI_MADT_RECORD_HEADER *)((PTR)Record + Record->RecordLength);

    while ((PTR)Record < MadtEnd)
    {
        if (Record->EntryType == ACPI_MADT_RECORD_LOCAL_APIC)
        {
            return (ACPI_LOCAL_APIC *)Record;
        }

        Record = (ACPI_MADT_RECORD_HEADER *)((PTR)Record + Record->RecordLength);
    }

    return NULL;
}

