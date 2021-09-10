
#pragma once


//
// Misc routines.
//

BOOLEAN
EFIAPI
EfiIsEqualGuid(
    IN EFI_GUID *Guid1,
    IN EFI_GUID *Guid2);

EFI_STATUS
EFIAPI
OslGetRandom(
    OUT UINT64 *Random);

BOOLEAN
EFIAPI
OslWaitForKeyInput(
    IN UINT16 ScanCode,
    IN CHAR16 UnicodeCharacter,
    IN UINT64 Timeout);

