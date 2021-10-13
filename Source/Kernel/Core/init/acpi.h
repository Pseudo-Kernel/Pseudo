
#pragma once

#include <base/base.h>

#pragma pack(push, 1)

//
// Root System Description Pointer
//

typedef struct _ACPI_ROOT_POINTER
{
    CHAR Signature[8];      // "RSD PTR " in 16-byte boundary.
    U8 Checksum;
    CHAR OemId[6];
    U8 Revision;            // 0-1.0, 1-2.0+
    U32 RSDT;
} ACPI_ROOT_POINTER, *PACPI_ROOT_POINTER;

//
// Extended RSDP structure (V 2.0 or higher)
//

typedef struct _ACPI_ROOT_POINTER_EXT
{
    ACPI_ROOT_POINTER Ver1Descriptor;
    U32 Length;
    U64 XSDT;
    U8 ExtChecksum;
    U8 Reserved[3];
} ACPI_ROOT_POINTER_EXT, *PACPI_ROOT_POINTER_EXT;

#define ACPI_RSDP_SIGNATURE         "RSD PTR "

//
// RSDT/XSDT structure.
//

typedef struct _ACPI_DESCRIPTION_HEADER
{
    CHAR Signature[4];
    U32 Length;
    U8 Revision;
    U8 Checksum;
    CHAR OemId[6];
    CHAR OemTableId[8];
    U32 OemRevision;
    U32 CreatorId;
    U32 CreatorRevision;
} ACPI_DESCRIPTION_HEADER, *PACPI_DESCRIPTION_HEADER;

typedef struct _ACPI_RSDT
{
    ACPI_DESCRIPTION_HEADER Header;
    U32 Entry[1];
} ACPI_RSDT, *PACPI_RSDT;

typedef struct _ACPI_XSDT
{
    ACPI_DESCRIPTION_HEADER Header;
    U64 Entry[1];
} ACPI_XSDT, *PACPI_XSDT;

#define ACPI_RSDT_SIGNATURE         "RSDT"
#define ACPI_XSDT_SIGNATURE         "XSDT"
#define ACPI_MADT_SIGNATURE         "APIC"
#define ACPI_BERT_SIGNATURE         "BERT"
#define ACPI_CPEP_SIGNATURE         "CPEP"
#define ACPI_DSDT_SIGNATURE         "DSDT"
#define ACPI_ECDT_SIGNATURE         "ECDT"
#define ACPI_EINJ_SIGNATURE         "EINJ"
#define ACPI_ERST_SIGNATURE         "ERST"
#define ACPI_FADT_SIGNATURE         "FACP"
#define ACPI_FACS_SIGNATURE         "FACS"
#define ACPI_HEST_SIGNATURE         "HEST"
#define ACPI_MSCT_SIGNATURE         "MSCT"
#define ACPI_MPST_SIGNATURE         "MPST"
#define ACPI_OEMX_SIGNATURE         "OEMx"
#define ACPI_PMTT_SIGNATURE         "PMTT"
#define ACPI_PSDT_SIGNATURE         "PSDT"
#define ACPI_RASF_SIGNATURE         "RASF"
#define ACPI_SBST_SIGNATURE         "SBST"
#define ACPI_SLIT_SIGNATURE         "SLIT"
#define ACPI_SRAT_SIGNATURE         "SRAT"
#define ACPI_SSDT_SIGNATURE         "SSDT"


//
// MADT record type definitions.
//

#define ACPI_MADT_RECORD_LAPIC              0
#define ACPI_MADT_RECORD_IOAPIC             1
#define ACPI_MADT_RECORD_INTSRC_OVERRIDE    2

typedef struct _ACPI_MADT_RECORD_HEADER
{
    U8 EntryType;
    U8 RecordLength;
} ACPI_MADT_RECORD_HEADER, *PACPI_MADT_RECORD_HEADER;

typedef struct _ACPI_LOCAL_APIC
{
    // Entry Type 0.
    ACPI_MADT_RECORD_HEADER Header;
    U8 AcpiProcessorId;
    U8 ApicId;

    union
    {
        struct
        {
            U32 ProcessorEnabled:1;
            U32 Unknown0:31;
        };
        U32 Flags;
    };
} ACPI_LOCAL_APIC, *PACPI_LOCAL_APIC;

typedef struct _ACPI_IOAPIC
{
    // Entry Type 1.
    ACPI_MADT_RECORD_HEADER Header;
    U8 IoApicId;
    U8 Reserved;
    U32 IoApicAddress;

    //
    // Global System Interrupt Base.
    //
    // The system interrupt vector index where this IOAPIC’s INTI lines start.
    // The number of INTI lines is determined by the IO APIC’s Max Redir Entry register
    //

    U32 GSI_Base;
} ACPI_IOAPIC, *PACPI_IOAPIC;

// Flags.Polarity
#define ACPI_INT_OVERRIDE_POLARITY_CONFORM_SPEC             0
#define ACPI_INT_OVERRIDE_POLARITY_ACTIVE_HIGH              1
#define ACPI_INT_OVERRIDE_POLARITY_ACTIVE_LOW               3

// Flags.TriggerMode
#define ACPI_INT_OVERRIDE_TRIG_CONFORM_SPEC                 0
#define ACPI_INT_OVERRIDE_TRIG_EDGE                         1
#define ACPI_INT_OVERRIDE_TRIG_LEVEL                        3

typedef struct _ACPI_INTERRUPT_SOURCE_OVERRIDE
{
    // Entry Type 2.
    ACPI_MADT_RECORD_HEADER Header;
    U8 BusSource;       // Constant(it'll be only 0), meaning ISA
    U8 IrqSource;       // IRQ

    //
    // Global System Interrupt.
    //
    // The Global System Interrupt Vector that this bus-
    // relative interrupt source will trigger.
    //
    U32 GSI_Vector;
    
    union
    {
        struct
        {
            // Polarity of the APIC I/O Input signals:
            // 00 - Conforms to the specifications of the bus
            // 01 - Active high
            // 10 - Reserved
            // 11 - Active low
            U16 Polarity:2;

            // Trigger mode of the APIC I/O Input signals:
            // 00 - Conforms to specifications of the bus
            // 01 - Edge-triggered
            // 10 - Reserved
            // 11 - Level Triggered
            U16 TriggerMode:2;
            U16 Reserved:12;
        };
        U16 Flags;
    };

    //
    // For example, if your machine has the ISA Programmable Interrupt Timer (PIT)
    // connected to ISA IRQ 0, but in APIC mode, it is connected to I/O APIC interrupt
    // input 2, then you would need an Interrupt Source Override where the source entry
    // is '0' and the Global System Interrupt is '2.'
    //

    // Redirect: MADT.INT_OVERRIDE.IrqSource -> MADT.INT_OVERRIDE.GSI_Vector - MADT.IOAPIC.GSI_Base
} ACPI_INTERRUPT_SOURCE_OVERRIDE, *PACPI_INTERRUPT_SOURCE_OVERRIDE;



typedef struct _ACPI_MADT
{
    ACPI_DESCRIPTION_HEADER Header;
    U32 LocalApicBase;
    union
    {
        struct
        {
            U32 PCAT_COMPAT:1;  // Duel 8259 PIC Installed.
            U32 Reserved:31;
        };
        U32 Flags;
    };
} ACPI_MADT, *PACPI_MADT;




typedef struct _ACPI_GENERIC_ADDRESS
{
    U8 AddressSpace;
    U8 BitWidth;
    U8 BitOffset;
    U8 AccessSize;
    U64 Address;
} ACPI_GENERIC_ADDRESS, *PACPI_GENERIC_ADDRESS;

typedef struct _ACPI_FADT
{
    ACPI_DESCRIPTION_HEADER Header;
    U32 FirmwareCtrl;
    U32 PointerToDsdt;

    U8 Reserved; // Used in ACPI 1.0 (no longer use)

    U8 PreferredPMProfile;
    U16 SCIIntVector;           // SCI_INT
    U32 SMICommand;             // PORT: SMI_CMD
    U8 AcpiEnable;              // CMD: ACPI_ENABLE
    U8 AcpiDisable;             // CMD: ACPI_DISABLE
    U8 S4BiosRequest;           // CMD: S4BIOS_REQ
    U8 PerfStateControl;        // PSTATE_CNT
    U32 PM1aEventBlock;         // PORT: PM1a_EVT_BLK
    U32 PM1bEventBlock;         // PORT: PM1b_EVT_BLK
    U32 PM1aControlBlock;       // PORT: PM1a_CNT_BLK
    U32 PM1bControlBlock;       // PORT: PM1b_CNT_BLK
    U32 PM2ControlBlock;        // PORT: PM2_CNT_BLK
    U32 PMTimerBlock;           // PORT: PM_TMR_BLK
    U32 GPE0Block;              // PORT: GPE0_BLK
    U32 GPE1Block;              // PORT: GPE1_BLK

    U8 PM1EventLength;
    U8 PM1ControlLength;
    U8 PM2ControlLength;
    U8 PMTimerLength;
    U8 GPE0Length;
    U8 GPE1Length;
    U8 GPE1Base;
    U8 CStateControl;
    U16 WorstC2Latency;
    U16 WorstC3Latency;
    U16 FlushSize;
    U16 FlushStride;
    U8 DutyOffset;
    U8 DutyWidth;
    U8 DayAlarm;
    U8 MonthAlarm;
    U8 Century;

    U16 BootArchitectureFlags_IAPC; // ACPI 2.0+

    U16 Reserved2_Zero;
    U32 Flags;

    ACPI_GENERIC_ADDRESS ResetReg;

    U8 ResetValue;
    U16 BootArchitectureFlags_ARM;
    U8 FadtMinorVersion;

    // 64bit pointers.
    U64 Pointer64ToXFacs; // ACPI 2.0+
    U64 Pointer64ToXDsdt; // ACPI 2.0+

    ACPI_GENERIC_ADDRESS XPM1aEventBlock;
    ACPI_GENERIC_ADDRESS XPM1bEventBlock;
    ACPI_GENERIC_ADDRESS XPM1aControlBlock;
    ACPI_GENERIC_ADDRESS XPM1bControlBlock;
    ACPI_GENERIC_ADDRESS XPM2ControlBlock;
    ACPI_GENERIC_ADDRESS XPMTimerBlock;
    ACPI_GENERIC_ADDRESS XGPE0Block;
    ACPI_GENERIC_ADDRESS XGPE1Block;
} ACPI_FADT, *PACPI_FADT;




#pragma pack(pop)



ESTATUS
KERNELAPI
PiAcpiPreInitialize(
    IN PVOID Rsdp);

