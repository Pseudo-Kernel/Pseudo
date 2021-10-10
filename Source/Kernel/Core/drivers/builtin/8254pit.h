
#pragma once

#define PIT_BASE_FREQ           1193181

// PIT Control

#define PIT_CHANNEL0_DATA       0x40
#define PIT_CHANNEL1_DATA       0x41
#define PIT_CHANNEL2_DATA       0x42
#define PIT_COMMAND             0x43


#define PIT_CW_CHANNEL(_ch)         (((_ch) & 0x03) << 6)

#define PIT_CW_ACCESS_LATCH         (0 << 4)
#define PIT_CW_ACCESS_LOW           (1 << 4)
#define PIT_CW_ACCESS_HIGH          (2 << 4)
#define PIT_CW_ACCESS_LOWHIGH       (3 << 4)

#define PIT_CW_MODE_TERMINAL_COUNT  (0 << 1)
#define PIT_CW_MODE_ONESHOT         (1 << 1)
#define PIT_CW_MODE_RATE            (2 << 1)
#define PIT_CW_MODE_SQUARE_WAVE     (3 << 1)
#define PIT_CW_MODE_SW_STROBE       (4 << 1)
#define PIT_CW_MODE_HW_STROBE       (5 << 1)

#define PIT_CW_COUNTER_BINARY       (0 << 0)
#define PIT_CW_COUNTER_BCD          (1 << 0)


BOOLEAN
KERNELAPI
Bid_8254SetupTimer(
    IN U32 Frequency);

U64
KERNELAPI
Bid_8254GetTickCount(
    VOID);

VOID
KERNELAPI
Bid_8254Initialize(
    VOID);

