
#pragma once

#include <base/base.h>

U64
KERNELAPI
HalGetTickCount(
    VOID);

ESTATUS
KERNELAPI
HalInitializePlatformTimer(
    VOID);

