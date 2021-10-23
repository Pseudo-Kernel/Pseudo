
#pragma once

#include <base/base.h>
#include <hal/apstub16.h>

extern VIRTUAL_ADDRESS HalApicBase;
extern AP_INIT_PACKET volatile *HalAPInitPacket;


VOID
KERNELAPI
HalPreInitialize(
    IN PVOID Rsdp);

VOID
KERNELAPI
HalInitialize(
    VOID);

