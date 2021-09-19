
#pragma once

VOID
Trace(
    IN CHAR16 *Format,
    ...);

VOID
EFIAPI
OslDbgWaitEnterKey(
    IN OS_LOADER_BLOCK *LoaderBlock, 
    IN CHAR16 *Message);

BOOLEAN
EFIAPI
OslDbgDumpLowMemoryToDisk(
    IN OS_LOADER_BLOCK *LoaderBlock);

BOOLEAN
EFIAPI
OslDbgFillScreen(
    IN OS_LOADER_BLOCK *LoaderBlock,
    IN UINT32 Color);

