
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
    OUT UINT8 *Buffer,
    IN UINTN BufferLength);

BOOLEAN
EFIAPI
OslWaitForKeyInput(
    IN UINT16 ScanCode,
    IN CHAR16 UnicodeCharacter,
    IN UINT64 Timeout);

