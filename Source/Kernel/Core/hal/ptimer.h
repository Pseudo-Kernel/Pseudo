
#pragma once

#include <base/base.h>

U64
KERNELAPI
HalGetTickCount(
    VOID);

ESTATUS
KERNELAPI
HalTimerGetFrequency(
    OUT U64 *Frequency);

ESTATUS
KERNELAPI
HalTimerReadCounter(
    OUT U64 *Counter);

ESTATUS
KERNELAPI
HalInitializePlatformTimer(
    VOID);

