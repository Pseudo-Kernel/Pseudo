
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

        KINTERRUPT PlatformTimer;
    } InterruptObjects;
    
} HAL_PRIVATE_DATA;



HAL_PRIVATE_DATA *
KERNELAPI
HalGetPrivateData(
    VOID);

VOID
KERNELAPI
HalStartProcessors(
    VOID);

ESTATUS
KERNELAPI
HalRegisterInterrupt(
    IN OUT KINTERRUPT *Interrupt,
    IN PKINTERRUPT_ROUTINE InterruptRoutine,
    IN PVOID InterruptContext,
    IN ULONG Vector);

VOID
KERNELAPI
HalInitializeProcessor(
    VOID);
