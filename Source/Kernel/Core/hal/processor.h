
#pragma once

#include <base/base.h>

typedef
VOID
(KERNELAPI *PKPROCESSOR_START_ROUTINE)(
    VOID);

#define HAL_PROCESSOR_RESET_VECTOR          0x04
#define HAL_PROCESSOR_RESET_ADDRESS         (HAL_PROCESSOR_RESET_VECTOR << 12)

typedef struct _HAL_PRIVATE_DATA
{
    struct
    {
        KINTERRUPT ApicSpurious;
        KINTERRUPT ApicTimer;
        KINTERRUPT ApicError;
    } InterruptObjects;
    
} HAL_PRIVATE_DATA;



VOID
KERNELAPI
HalStartProcessors(
    VOID);

VOID
KERNELAPI
HalInitializeProcessor(
    VOID);
