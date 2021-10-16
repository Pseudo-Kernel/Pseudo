
/**
 * @file ioapic.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements the IOAPIC related routines.
 * @version 0.1
 * @date 2021-10-16
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <base/base.h>
#include <ke/lock.h>
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
