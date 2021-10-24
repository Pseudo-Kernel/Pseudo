
/**
 * @file 8254pit.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements built-in driver (8254 PIT).
 * @version 0.1
 * @date 2021-10-10 (original date 201?-??-??)
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <base/base.h>
#include <ke/ke.h>
#include <init/bootgfx.h>
#include <mm/mm.h>
#include <mm/pool.h>
#include <hal/8259pic.h>
#include <hal/8254pit.h>

typedef struct _HAL_8254_PIT_CONTEXT
{
    PKINTERRUPT Interrupt;
    ULONG Vector;
} HAL_8254_PIT_CONTEXT;

HAL_8254_PIT_CONTEXT Bid_8254Context;

volatile U64 Bid_8254TickCount;
U32 Bid_8254TimerFrequency;

#define PIT_VECTOR                  (IRQL_TO_VECTOR_START(IRQL_LEGACY) + 0)


/**
 * @brief ISR for 8254 PIT.
 * 
 * @param [in] Interrupt            Interrupt object.
 * @param [in] InterruptContext     Interrupt context.
 * 
 * @return Always InterruptAccepted (which means successfully handled).
 */
KINTERRUPT_RESULT
KERNELAPI
Hal_8254Isr_PIC(
    IN PKINTERRUPT Interrupt,
    IN PVOID InterruptContext)
{
    Bid_8254TickCount++;
    HalPicSendEoi(PIT_VECTOR);

    return InterruptAccepted;
}

/**
 * @brief Setup the PIT timer with given frequency.
 * 
 * @param [in] Frequency    Frequency.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
KERNELAPI
Hal_8254SetupTimer(
	IN U32 Frequency)
{
	if (!Frequency)
        return FALSE;

    // 1.1931816666 MHz / Frequency
    U32 Data = PIT_BASE_FREQ / Frequency;
    Bid_8254TimerFrequency = Frequency;

    //
    // Initialize 8254 PIT.
    // Channel 0, Lo-hibyte, Rate generator, Use binary counter
    //

    __outbyte(PIT_COMMAND, PIT_CW_CHANNEL(0) | PIT_CW_ACCESS_LOWHIGH | PIT_CW_MODE_RATE | PIT_CW_COUNTER_BINARY);
    __outbyte(PIT_CHANNEL0_DATA, (U8)(Data & 0xff));
    __outbyte(PIT_CHANNEL0_DATA, (U8)(Data >> 0x08));

    return TRUE;
}

/**
 * @brief Returns tick count.
 * 
 * @return 64-bit tick count.
 */
U64
KERNELAPI
Hal_8254GetTickCount(
    VOID)
{
    return Bid_8254TickCount;
}

/**
 * @brief Initializes the 8254 PIT.
 * 
 * @return None.
 */
VOID
KERNELAPI
Hal_8254Initialize(
	VOID)
{
    PKINTERRUPT Interrupt = MmAllocatePool(PoolTypeNonPaged, sizeof(KINTERRUPT), 0x10, 0);
    if (!Interrupt)
    {
        FATAL("Failed to allocate interrupt object");
    }

    ULONG Vector = 0;
    ESTATUS Status = KeInitializeInterrupt(Interrupt, &Hal_8254Isr_PIC, NULL, 0);

    DASSERT(E_IS_SUCCESS(Status));
    Status = KeAllocateIrqVector(IRQL_LEGACY, 1, PIT_VECTOR, INTERRUPT_IRQ_HINT_EXACT_MATCH, &Vector, TRUE);
    if (!E_IS_SUCCESS(Status))
    {
        FATAL("Failed to allocate irq vector 0x%02x", PIT_VECTOR);
    }

    DASSERT(PIT_VECTOR == Vector);
    Status = KeConnectInterrupt(Interrupt, PIT_VECTOR, 0);
    if (!E_IS_SUCCESS(Status))
    {
        FATAL("Failed to connect interrupt chain (vector = 0x%02x)", PIT_VECTOR);
    }

    Bid_8254Context.Interrupt = Interrupt;
    Bid_8254Context.Vector = Vector;

	Bid_8254TickCount = 0;
	Hal_8254SetupTimer(1000); // 1 Interrupt per millisecond
}
