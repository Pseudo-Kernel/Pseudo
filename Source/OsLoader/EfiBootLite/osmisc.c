
/**
 * @file osmisc.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements miscellaneous routines.
 * @version 0.1
 * @date 2021-09-10
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "OsLoader.h"
#include "osmisc.h"


/**
 * @brief Determines whether the two GUIDs are equal.
 * 
 * @param [in] Guid1    GUID to compare.
 * @param [in] Guid2    GUID to compare.
 * 
 * @return TRUE if equal, FALSE otherwise. 
 */
BOOLEAN
EFIAPI
EfiIsEqualGuid(
    IN EFI_GUID *Guid1,
    IN EFI_GUID *Guid2)
{
    UINT64 *p1 = (UINT64 *)Guid1;
    UINT64 *p2 = (UINT64 *)Guid2;

    if (sizeof(*Guid1) != 0x10)
        return FALSE;

    return (p1[0] == p2[0] && p1[1] == p2[1]);
}

/**
 * @brief Gets the 64-bit random value.
 * 
 * @param [in] LoaderBlock  Loader block.
 * 
 * @return EFI_SUCCESS      The operation is completed successfully.
 * @return else             An error occurred during the operation.
 */
EFI_STATUS
EFIAPI
OslGetRandom(
    OUT UINT64 *Random)
{
    EFI_RNG_PROTOCOL *RngProtocol = NULL;
    EFI_STATUS Status = EFI_SUCCESS;

    Status = gBS->HandleProtocol(gImageHandle, &gEfiRngProtocolGuid, (void **)&RngProtocol);
    if (Status != EFI_SUCCESS)
        return Status;

    Status = RngProtocol->GetRNG(RngProtocol, NULL, sizeof(*Random), (UINT8 *)Random);
    if (Status != EFI_SUCCESS)
        return Status;

    return EFI_SUCCESS;
}

/**
 * @brief Wait for single keystroke.
 * 
 * @param [in] ScanCode             Scancode to wait.
 * @param [in] UnicodeCharacter     Unicode character for scancode.\n
 *                                  If specified ScanCode == 0, UnicodeCharacter will be used.
 * @param [in] Timeout              Timeout in microseconds.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
EFIAPI
OslWaitForKeyInput(
    IN UINT16 ScanCode,
    IN CHAR16 UnicodeCharacter,
    IN UINT64 Timeout)
{
    EFI_INPUT_KEY Key;
    EFI_EVENT TimerEvent;
    EFI_STATUS Status;
    EFI_EVENT WaitEvents[2];
    BOOLEAN BreakLoop;
    BOOLEAN Result;

    Status = gBS->CreateEvent(EVT_TIMER, 0, NULL, NULL, &TimerEvent);

    if (Status != EFI_SUCCESS)
    {
        return FALSE;
    }

    // SetTimer requires 100ns-unit timeout.
    Status = gBS->SetTimer(TimerEvent, TimerRelative, Timeout * 10);

    if (Status != EFI_SUCCESS)
    {
        gBS->CloseEvent(TimerEvent);
        return FALSE;
    }

    WaitEvents[0] = TimerEvent;
    WaitEvents[1] = gConIn->WaitForKey;
    BreakLoop = Result = FALSE;

    do
    {
        UINTN Index;

        Status = gBS->WaitForEvent(ARRAY_SIZE(WaitEvents), WaitEvents, &Index);
        if (Status != EFI_SUCCESS)
        {
            break;
        }

        switch (Index)
        {
        case 0:
            // Timeout!
            BreakLoop = TRUE;
            break;
        case 1:
            // Key was pressed!
            gConIn->ReadKeyStroke(gConIn, &Key);
            // TRACEF("Key.ScanCode 0x%02X, Key.UCh 0x%04X\r\n", Key.ScanCode, Key.UnicodeChar);

            if (Key.ScanCode == ScanCode)
            {
                if (Key.ScanCode != 0 ||
                    (Key.ScanCode == 0 && Key.UnicodeChar == UnicodeCharacter))
                {
                    BreakLoop = TRUE;
                    Result = TRUE;
                }
            }
            break;
        }
    } while (!BreakLoop);

    gBS->CloseEvent(TimerEvent);

    return Result;
}
