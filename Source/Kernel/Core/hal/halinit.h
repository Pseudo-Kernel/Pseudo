
#pragma once

#include <base/base.h>


#define HAL_PROCESSOR_RESET_VECTOR          0x04
#define HAL_PROCESSOR_RESET_ADDRESS         (HAL_PROCESSOR_RESET_VECTOR << 12)


VOID
KERNELAPI
HalPreInitialize(
    IN PVOID Rsdp);

VOID
KERNELAPI
HalInitialize(
    VOID);

