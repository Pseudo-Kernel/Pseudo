
/**
 * @file ioapic.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements the IOAPIC related routines.
 * @version 0.1
 * @date 2021-10-16
 * 
 * @copyright Copyright (c) 2021
 */

#include <base/base.h>
#include <ke/lock.h>
#include <ke/inthandler.h>
#include <ke/interrupt.h>
#include <ke/kprocessor.h>
#include <hal/acpi.h>
#include <hal/ioapic.h>
#include <mm/mm.h>


IOAPIC HalIoApicBlock[32];
ULONG HalIoApicCount;

/**
 * @brief Converts the GSI to IOAPIC INTIN.
 * 
 * @param [in] GSIBase      Base GSI.
 * @param [in] GSILimit     GSI Limit.
 * @param [in] GSI          GSI to be converted.
 * @param [out] INTIN       Converted INTIN.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
HalIoApicGSIToINTIN(
    IN U32 GSIBase,
    IN U32 GSILimit,
    IN U32 GSI,
    OUT U32 *INTIN)
{
    if (GSIBase <= GSI && GSI <= GSILimit)
    {
        *INTIN = GSI - GSIBase;
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief Initializes the IOAPIC structure.
 * 
 * @param [out] IoApic      IOAPIC structure to be initialized.
 * @param [in] IoApicBase   Physical base address of IOAPIC.
 * @param [in] GSIBase      Starting GSI number of IOAPIC. See ACPI_IOAPIC::GSI_Base.
 * 
 * @return ESTATUS code.
 */
ESTATUS
KERNELAPI
HalIoApicInitialize(
    OUT IOAPIC *IoApic,
    IN PHYSICAL_ADDRESS IoApicBase,
    IN U32 GSIBase)
{
    if (IoApicBase & PAGE_MASK)
    {
        return E_INVALID_PARAMETER;
    }

    PTR VirtualBase = 0;

    ESTATUS Status = MmAllocateVirtualMemory(NULL, &VirtualBase, PAGE_SIZE, VadInUse);
    if (!E_IS_SUCCESS(Status))
    {
        return Status;
    }

    Status = MmMapSinglePage(IoApicBase, VirtualBase, ARCH_X64_PXE_WRITABLE | ARCH_X64_PXE_CACHE_DISABLED);
    if (!E_IS_SUCCESS(Status))
    {
        ESTATUS FreeStatus = MmFreeVirtualMemory(VirtualBase, PAGE_SIZE);
        DASSERT(E_IS_SUCCESS(FreeStatus));

        return Status;
    }

    U32 IoApicId = HalIoApicRead(VirtualBase, IOAPICID);
    U32 IoApicVer = HalIoApicRead(VirtualBase, IOAPICVER);
    U32 IoApicArb = HalIoApicRead(VirtualBase, IOAPICARB);
    U32 RedirectionEntriesCountMinus1 = IOAPIC_VER_GET_MAX_REDIRENT(IoApicVer);

    memset(IoApic, 0, sizeof(*IoApic));

    KeInitializeSpinlock(&IoApic->Lock);
    IoApic->PhysicalBase = IoApicBase;
    IoApic->VirtualBase = VirtualBase;
    IoApic->GSIBase = GSIBase;
    IoApic->GSILimit = GSIBase + RedirectionEntriesCountMinus1;
    IoApic->IoApicId = IOAPIC_ID_GET_ID(IoApicId);
    IoApic->IoApicArbId = IOAPIC_ARB_GET_ARB_ID(IoApicArb);
    IoApic->Version = IOAPIC_VER_GET_VERSION(IoApicVer);

    IoApic->IsValid = TRUE;

    return E_SUCCESS;
}

/**
 * @brief Adds the IOAPIC structure.\n
 *        This cannot be undone because it is used only during system initialization.\n
 * 
 * @param [in] GSIBase              Starting GSI number of IOAPIC. See ACPI_IOAPIC::GSI_Base.
 * @param [in] IoApicPhysicalBase   Physical base address of IOAPIC.
 * 
 * @return Pointer of IOAPIC structure.\n
 *         If fails, this function returns NULL.
 */
IOAPIC *
KERNELAPI
HalIoApicAdd(
    IN U32 GSIBase,
    IN PHYSICAL_ADDRESS IoApicPhysicalBase)
{
    if (HalIoApicCount >= COUNTOF(HalIoApicBlock))
    {
        return NULL;
    }

    IOAPIC *IoApic = &HalIoApicBlock[HalIoApicCount];

    ESTATUS Status = HalIoApicInitialize(IoApic, IoApicPhysicalBase, GSIBase);
    if (!E_IS_SUCCESS(Status))
    {
        return NULL;
    }

    HalIoApicCount++;

    return IoApic;
}

/**
 * @brief Reads the IOAPIC register.
 * 
 * @param [in] IoApicBase   Virtual base address of IOAPIC.
 * @param [in] Register     Register number.
 * 
 * @return Returns 32-bit value.
 */
U32
KERNELAPI
HalIoApicRead(
    IN PTR IoApicBase, 
    IN U32 Register)
{
    PTR Base = IoApicBase;

    volatile U32 *RegSel = (volatile U32 *)IOREGSEL(Base);
    volatile U32 *RegWin = (volatile U32 *)IOREGWIN(Base);

    *RegSel = Register;
    return *RegWin;
}

/**
 * @brief Writes the IOAPIC register.
 * 
 * @param [in] IoApicBase   Virtual base address of IOAPIC.
 * @param [in] Register     Register number.
 * @param [in] Value        Value to be written.
 * 
 * @return Returns previous 32-bit value.
 */
U32
KERNELAPI
HalIoApicWrite(
    IN PTR IoApicBase, 
    IN U32 Register, 
    IN U32 Value)
{
    PTR Base = IoApicBase;

    volatile U32 *RegSel = (volatile U32 *)IOREGSEL(Base);
    volatile U32 *RegWin = (volatile U32 *)IOREGWIN(Base);

    *RegSel = Register;

    U32 PrevValue = *RegWin;
    *RegWin = Value;

    return PrevValue;
}

/**
 * @brief Acquires IOAPIC lock.
 * 
 * @param [in] IoApic       IOAPIC structure.
 * @param [out] PrevState   Previous state.
 * 
 * @return None.
 */
VOID
KERNELAPI
HalpIoApicAcquireLock(
    IN IOAPIC *IoApic,
    OUT BOOLEAN *PrevState)
{
    KeAcquireSpinlockDisableInterrupt(&IoApic->Lock, PrevState);
}

/**
 * @brief Releases IOAPIC lock.
 * 
 * @param [in] IoApic       IOAPIC structure.
 * @param [in] PrevState    Previous state.
 * 
 * @return None.
 */
VOID
KERNELAPI
HalpIoApicReleaseLock(
    IN IOAPIC *IoApic,
    IN BOOLEAN PrevState)
{
    KeReleaseSpinlockRestoreInterrupt(&IoApic->Lock, PrevState);
}

/**
 * @brief Reads I/O redirection table entry (IOREDTBLn) of IOAPIC.
 * 
 * @param [in] IoApic           IOAPIC structure.
 * @param [in] IntIn            INTIN# number which connected to redirection table entry.\n
 *                              To convert GSI to INTIN, See HalIoApicGSIToINTIN().\n
 * @param [out] Redirection     64-bit result of redirection table entry read.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
HalIoApicGetIoRedirection(
    IN IOAPIC *IoApic, 
    IN U32 IntIn, 
    OUT U64 *Redirection)
{
    BOOLEAN PrevState = FALSE;

    // Must be 0 <= IntIn <= GSILimit - GSIBase
    if (0 > IntIn || IoApic->GSIBase + IntIn > IoApic->GSILimit)
    {
        return FALSE;
    }

    HalpIoApicAcquireLock(IoApic, &PrevState);

    U32 High = HalIoApicRead(IoApic->VirtualBase, IOREDTBL_HIGH(IntIn));
    U32 Low = HalIoApicRead(IoApic->VirtualBase, IOREDTBL_LOW(IntIn));

    HalpIoApicReleaseLock(IoApic, PrevState);

    *Redirection = ((U64)High << 0x20) | Low;

    return TRUE;
}

/**
 * @brief Get IOAPIC structure by IOAPIC ID.
 * 
 * @param [in] IoApicId     IOAPIC ID.
 * 
 * @return Pointer of IOAPIC structure.\n
 *         If fails, this function returns NULL.
 */
IOAPIC *
KERNELAPI
HalIoApicGetBlock(
    IN U32 IoApicId)
{
    for (ULONG i = 0; i < HalIoApicCount; i++)
    {
        DASSERT(HalIoApicBlock[i].IsValid);
        if (HalIoApicBlock[i].IoApicId == IoApicId)
        {
            return &HalIoApicBlock[i];
        }
    }

    return NULL;
}

/**
 * @brief Get IOAPIC structure by GSI.
 * 
 * @param [in] GSI          GSI number of IOAPIC.
 * 
 * @return Pointer of IOAPIC structure.\n
 *         If fails, this function returns NULL.
 */
IOAPIC *
KERNELAPI
HalIoApicGetBlockByGSI(
    IN U32 GSI)
{
    for (ULONG i = 0; i < HalIoApicCount; i++)
    {
        IOAPIC *IoApic = &HalIoApicBlock[i];
        DASSERT(IoApic->IsValid);
        if (IoApic->GSIBase <= GSI && GSI <= IoApic->GSILimit)
        {
            return IoApic;
        }
    }

    return NULL;
}

/**
 * @brief Masks interrupt by given INTIN#.
 * 
 * @param [in] IoApic               IOAPIC structure.
 * @param [in] IntIn                INTIN# number which connected to redirection table entry.\n
 *                                  To convert GSI to INTIN, See HalIoApicGSIToINTIN().\n
 * @param [in] Mask                 Specifies the mask state. TRUE for masked, FALSE for unmasked.
 * @param [out] PrevMaskedState     Previous interrupt mask state.
 * 
 * @return None. 
 */
VOID
KERNELAPI
HalIoApicMaskInterrupt(
    IN IOAPIC *IoApic, 
    IN U32 IntIn, 
    IN BOOLEAN Mask, 
    OUT BOOLEAN *PrevMaskedState)
{
    BOOLEAN PrevState = FALSE;

    HalpIoApicAcquireLock(IoApic, &PrevState);

    DASSERT(IoApic->GSIBase + IntIn <= IoApic->GSILimit);

    U32 PrevLow = HalIoApicRead(IoApic->VirtualBase, IOREDTBL_LOW(IntIn));

    if (PrevMaskedState)
        *PrevMaskedState = !!(PrevLow & IOAPIC_RED_INTERRUPT_MASKED);

    U32 New = PrevLow;

    if (Mask)
        New |= IOAPIC_RED_INTERRUPT_MASKED;
    else
        New &= ~IOAPIC_RED_INTERRUPT_MASKED;

    HalIoApicWrite(IoApic->VirtualBase, IOREDTBL_LOW(IntIn), New);

    HalpIoApicReleaseLock(IoApic, PrevState);
}

/**
 * @brief Sets I/O redirection table entry by given INTIN#.
 * 
 * @param [in] IoApic               IOAPIC structure.
 * @param [in] IntIn                INTIN# number which connected to redirection table entry.\n
 *                                  To convert GSI to INTIN, See HalIoApicGSIToINTIN().\n
 * @param [in] RedirectionEntry     New value of redirection table entry.
 * 
 * @return Previous 64-bit value of redirection table entry. 
 */
U64
KERNELAPI
HalIoApicSetIoRedirection(
    IN IOAPIC *IoApic, 
    IN U32 IntIn, 
    IN U64 RedirectionEntry)
{
    BOOLEAN PrevState = FALSE;

    HalpIoApicAcquireLock(IoApic, &PrevState);

    DASSERT(IoApic->GSIBase + IntIn <= IoApic->GSILimit);

    //
    // Mask interrupt first so that we can safely change the redirection entry.
    // (We can't read the 64-bit redirection entry at once!)
    //

    U32 PrevLow = HalIoApicRead(IoApic->VirtualBase, IOREDTBL_LOW(IntIn));
    HalIoApicWrite(IoApic->VirtualBase, IOREDTBL_LOW(IntIn), PrevLow | IOAPIC_RED_INTERRUPT_MASKED);

    U32 PrevHigh = HalIoApicWrite(IoApic->VirtualBase, IOREDTBL_HIGH(IntIn), (U32)(RedirectionEntry >> 0x20));
    HalIoApicWrite(IoApic->VirtualBase, IOREDTBL_LOW(IntIn), (U32)(RedirectionEntry & 0xffffffff));

    HalpIoApicReleaseLock(IoApic, PrevState);

    return ((U64)PrevHigh << 0x20) | PrevLow;
}

/**
 * @brief Do read-modify-set operation for I/O redirection table entry.
 * 
 * @param [in] IoApic               IOAPIC structure.
 * @param [in] IntIn                INTIN# number which connected to redirection table entry.\n
 *                                  To convert GSI to INTIN, See HalIoApicGSIToINTIN().\n
 * @param [in] RedirectionEntry     New value of redirection table entry.
 * @param [in] SetMask              Specifies which bits are needed to be changed.\n
 *                                  - If zero is specified, nothing will be changed.\n
 *                                  - If -1 is specified, all bits will be applied.\n
 *                                  - If IOREDTBL[INTIN] = 0x03000000000000c0, RedirectionEntry = 0x0000000000010070, \n
 *                                    SetMask = 0x00000000000100ff, changes will be applied as follows:\n
 *                                    IOREDTBL[INTIN] = (IOREDTBL[INTIN] & ~SetMask) | (RedirectionEntry & SetMask)\n
 *                                                    = 0x0300000000010070 (new value)\n
 * 
 * @return Previous 64-bit value of redirection table entry. 
 */
U64
KERNELAPI
HalIoApicSetIoRedirectionByMask(
    IN IOAPIC *IoApic, 
    IN U32 IntIn, 
    IN U64 RedirectionEntry, 
    IN U64 SetMask)
{
    BOOLEAN PrevState = FALSE;

    HalpIoApicAcquireLock(IoApic, &PrevState);

    DASSERT(IoApic->GSIBase + IntIn <= IoApic->GSILimit);

    PTR VirtualBase = IoApic->VirtualBase;

    U32 LowMask = (U32)(SetMask & 0xffffffff);
    U32 HighMask = (U32)(SetMask >> 0x20);


    //
    // Mask interrupt first so that we can safely change the redirection entry.
    // (We can't read the 64-bit redirection entry at once!)
    //

    U32 PrevLow = HalIoApicRead(VirtualBase, IOREDTBL_LOW(IntIn));
    U32 PrevHigh = HalIoApicRead(VirtualBase, IOREDTBL_HIGH(IntIn));
    HalIoApicWrite(VirtualBase, IOREDTBL_LOW(IntIn), PrevLow | IOAPIC_RED_INTERRUPT_MASKED);

    U32 LowWrite = (PrevLow & ~LowMask) | ((U32)(RedirectionEntry & 0xffffffff) & LowMask);
    U32 HighWrite = (PrevHigh & ~HighMask) | ((U32)(RedirectionEntry >> 0x20) & HighMask);

    HalIoApicWrite(VirtualBase, IOREDTBL_HIGH(IntIn), HighWrite);
    HalIoApicWrite(VirtualBase, IOREDTBL_LOW(IntIn), LowWrite);

    HalpIoApicReleaseLock(IoApic, PrevState);

    return ((U64)PrevHigh << 0x20) | PrevLow;
}

/**
 * @brief Tests whether the I/O redirection entry is allocated.
 * 
 * @param [in] IoApic   IOAPIC structure.
 * @param [in] IntIn    INTIN# number which connected to redirection table entry.\n
 *                      To convert GSI to INTIN, See HalIoApicGSIToINTIN().\n
 * @param [in] Lock     If TRUE, IOAPIC will be locked during test.
 * 
 * @return TURE if specified redirection entry is allocated.
 *         FALSE otherwise.
 */
BOOLEAN
KERNELAPI
HalIoApicpIsIoRedirectionAllocated(
    IN IOAPIC *IoApic,
    IN U32 IntIn,
    IN BOOLEAN Lock)
{
    BOOLEAN PrevState = FALSE;

    DASSERT(IoApic->GSIBase + IntIn <= IoApic->GSILimit);

    if (Lock)
    {
        HalpIoApicAcquireLock(IoApic, &PrevState);
    }

    BOOLEAN InUse = !!(IoApic->AllocationBitmap[IntIn >> 3] & (1 << (IntIn & 0x07)));

    if (Lock)
    {
        HalpIoApicReleaseLock(IoApic, PrevState);
    }

    return InUse;
}

/**
 * @brief Allocates I/O redirection entry from given range [IntInStart, IntInEnd).\n
 * 
 * @param [in] IoApic       IOAPIC structure.
 * @param [out] IntIn       Starting INTIN# number of resulting range [IntIn, IntIn + Count - 1].
 * @param [in] Count        Entry count.
 * @param [in] IntInStart   Starting INTIN# number.
 * @param [in] IntInEnd     Ending INTIN# number.
 * 
 * @return ESTATUS code.
 */
ESTATUS
KERNELAPI
HalIoApicAllocateIoRedirection(
    IN IOAPIC *IoApic,
    OUT U32 *IntIn,
    IN U32 Count,
    IN U32 IntInStart,
    IN U32 IntInEnd)
{
    BOOLEAN PrevState = FALSE;
    ESTATUS Status = E_NOT_ENOUGH_RESOURCE;

    if (IntInStart > IntInEnd || Count <= 0)
    {
        return E_INVALID_PARAMETER;
    }

    HalpIoApicAcquireLock(IoApic, &PrevState);

    BOOLEAN FirstFound = FALSE;
    U32 FirstIndex = 0;

    for (U32 i = IoApic->GSIBase; i <= IoApic->GSILimit; i++)
    {
        U32 Index = i - IoApic->GSIBase;
        if (IntInStart <= Index && Index < IntInEnd)
        {
            U8 AllocationBit = 1 << (Index & 0x07);
            U8 *Bitmap = &IoApic->AllocationBitmap[Index >> 3];

            if (!((*Bitmap) & AllocationBit))
            {
                if (!FirstFound)
                {
                    FirstFound = TRUE;
                    FirstIndex = Index;
                }

                if (FirstFound)
                {
                    if (Index - FirstIndex + 1 >= Count)
                    {
                        Status = E_SUCCESS;
                        break;
                    }
                }
            }
            else
            {
                FirstIndex = FALSE;
            }
        }
    }

    if (Status == E_SUCCESS)
    {
        for (U32 i = 0; i < Count; i++)
        {
            U32 Index = i + FirstIndex;
            U8 AllocationBit = 1 << (Index & 0x07);
            U8 *Bitmap = &IoApic->AllocationBitmap[Index >> 3];

            *Bitmap |= AllocationBit;
        }

        *IntIn = FirstIndex;
    }

    HalpIoApicReleaseLock(IoApic, PrevState);

    return Status;
}

/**
 * @brief Frees I/O redirection entry.
 * 
 * @param [in] IoApic       IOAPIC structure.
 * @param [in] IntIn        INTIN# number to free.
 * @param [in] Count        Entry count.
 * 
 * @return None.
 */
VOID
KERNELAPI
HalIoApicFreeIoRedirection(
    IN IOAPIC *IoApic,
    IN U32 IntIn,
    IN U32 Count)
{
    BOOLEAN PrevState = FALSE;

    DASSERT(IoApic->GSIBase + IntIn <= IoApic->GSILimit);
    DASSERT(IoApic->GSIBase + IntIn + Count <= IoApic->GSILimit + 1);

    HalpIoApicAcquireLock(IoApic, &PrevState);

    for (U32 i = 0; i < Count; i++)
    {
        U32 Index = i + IntIn;
        U8 AllocationBit = 1 << (Index & 0x07);
        U8 *Bitmap = &IoApic->AllocationBitmap[Index >> 3];
        
        DASSERT((*Bitmap) & AllocationBit);
        *Bitmap &= ~AllocationBit;
    }

    HalpIoApicReleaseLock(IoApic, PrevState);
}


/**
 * @brief Allocates I/O redirection entry from given GSI range [GSIStart, GSIEnd).\n
 * 
 * @param [out] GSI         Starting GSI of resulting GSI range [GSI, GSI + Count - 1].
 * @param [in] Count        Entry count.
 * @param [in] GSIStart     Starting GSI number.
 * @param [in] GSIEnd       Ending GSI number.
 * 
 * @return ESTATUS code.
 */
ESTATUS
KERNELAPI
HalAllocateInterruptRedirection(
    OUT U32 *GSI,
    IN U32 Count,
    IN U32 GSIStart,
    IN U32 GSIEnd)
{
    if (GSIStart > GSIEnd || Count <= 0)
    {
        return E_INVALID_PARAMETER;
    }

    ESTATUS Status = E_FAILED;
    U32 IntIn = 0;

    IOAPIC *IoApic = HalIoApicGetBlockByGSI(GSIStart);
    if (!IoApic)
    {
        return E_INVALID_PARAMETER;
    }

    U32 GSINext = IoApic->GSILimit + 1;
    if (GSINext >= GSIEnd)
    {
        GSINext = GSIEnd;
    }

    Status = HalIoApicAllocateIoRedirection(IoApic, &IntIn, Count, 
        GSIStart - IoApic->GSIBase, GSINext - IoApic->GSIBase);

    if (E_IS_SUCCESS(Status))
    {
        if (GSI)
        {
            *GSI = IntIn + IoApic->GSIBase;
        }
    }

    return Status;
}

/**
 * @brief Frees I/O redirection entry.
 * 
 * @param [in] GSI      GSI number to free.
 * @param [in] Count    Entry count.
 * 
 * @return ESTATUS code.
 */
ESTATUS
KERNELAPI
HalFreeInterruptRedirection(
    IN U32 GSI,
    IN U32 Count)
{
    if (Count <= 0)
    {
        return E_INVALID_PARAMETER;
    }

    IOAPIC *IoApic = HalIoApicGetBlockByGSI(GSI);
    if (!IoApic)
    {
        return E_INVALID_PARAMETER;
    }

    U32 IntIn = 0;
    DASSERT(HalIoApicGSIToINTIN(IoApic->GSIBase, IoApic->GSILimit, GSI, &IntIn));

    HalIoApicFreeIoRedirection(IoApic, IntIn, Count);

    return E_SUCCESS;
}

/**
 * @brief Sets I/O redirection entry for given GSI.
 * 
 * @param [in] GSI                      GSI number to redirect.
 * @param [in] Vector                   Interrupt vector number for given GSI.
 * @param [in] DestinationProcessor     Destination processor id for given GSI.
 * 
 * @return ESTATUS code.
 */
ESTATUS
KERNELAPI
HalSetInterruptRedirection(
    IN U32 GSI,
    IN U8 Vector,
    IN U8 DestinationProcessor,
    IN U32 Flags)
{
    IOAPIC *IoApic = HalIoApicGetBlockByGSI(GSI);
    if (!IoApic)
    {
        return E_INVALID_PARAMETER;
    }

    U16 ApicId = KiProcessorIdToApicId[DestinationProcessor];
    if (ApicId & ~0xff)
    {
        // Mapping not exists (ProcessorId -> ApicId)
        return E_INVALID_PARAMETER;
    }

    U64 RedirectionEntry =
        IOAPIC_RED_VECTOR(Vector) | 
        IOAPIC_RED_DESTINATION_FIELD(ApicId) | IOAPIC_RED_DEST_MODE_PHYSICAL | 
        IOAPIC_RED_DELIVERY_MODE(IOAPIC_RED_DELIVER_FIXED);
    
    U64 Mask = 
        IOAPIC_RED_SETBIT_VECTOR | IOAPIC_RED_SETBIT_MASK_INT |
        IOAPIC_RED_SETBIT_DESTINATION | IOAPIC_RED_SETBIT_DESTINATION_MODE |
        IOAPIC_RED_SETBIT_DELIVERY_MODE;

    if (Flags & INTERRUPT_REDIRECTION_FLAG_SET_POLARITY)
    {
        if (Flags & INTERRUPT_REDIRECTION_FLAG_LOW_ACTIVE)
            RedirectionEntry |= IOAPIC_RED_POLARITY_LOW_ACTIVE;
        Mask |= IOAPIC_RED_SETBIT_POLARITY;
    }

    if (Flags & INTERRUPT_REDIRECTION_FLAG_SET_TRIGGER_MODE)
    {
        if (Flags & INTERRUPT_REDIRECTION_FLAG_LEVEL_TRIGGERED)
            RedirectionEntry |= IOAPIC_RED_TRIGGERED_LEVEL;
        Mask |= IOAPIC_RED_SETBIT_TRIGGERED;
    }

    HalIoApicSetIoRedirectionByMask(IoApic, GSI, RedirectionEntry, Mask);

    return E_SUCCESS;
}

