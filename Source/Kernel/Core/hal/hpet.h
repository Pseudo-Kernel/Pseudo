
#pragma once

//
// HPET general registers.
//

#define HPET_REGISTER_CAPABILITIES_ID                   0x0000  // RO
#define HPET_REGISTER_CONFIGURATION                     0x0010  // RW

#define HPET_REGISTER_INTERRUPT_STATUS                  0x0020  // RW Clear
#define HPET_REGISTER_MAIN_COUNTER                      0x00f0  // RW

#define HPET_REGISTER_TIMER0_CONF_CAPABILITY            0x0100  // RW
#define HPET_REGISTER_TIMER0_COMPARATOR                 0x0108  // RW
#define HPET_REGISTER_TIMER0_FSB_INTERRUPT_ROUTE        0x0110  // RW

#define HPET_SIZE_REGISTER_SPACE                        0x0400  // Timer register space is 1024 bytes
#define HPET_ACCESS_ALIGNMENT                           8       // Alignment for register access

//
// Registers for timer 0..31 (if it exists).
//

#define HPET_REGISTER_TIMER_N_CONF_CAPABILITY(_n)       ( HPET_REGISTER_TIMER0_CONF_CAPABILITY + (_n) * 0x20 )
#define HPET_REGISTER_TIMER_N_COMPARATOR(_n)            ( HPET_REGISTER_TIMER0_COMPARATOR + (_n) * 0x20 )
#define HPET_REGISTER_TIMER_N_FSB_INTERRUPT_ROUTE(_n)   ( HPET_REGISTER_TIMER0_FSB_INTERRUPT_ROUTE + (_n) * 0x20 )
#define HPET_REGISTER_TIMER_N_CONF_CAPABILITY(_n)       ( HPET_REGISTER_TIMER0_CONF_CAPABILITY + (_n) * 0x20 )


typedef union _HPET_CAPABILITIES_AND_ID
{
    // All fields are Read-only.
    struct
    {
        U8 REV_ID;

        U8 NUM_TIM_CAP:5;       // (Number of timers) - 1
        U8 COUNT_SIZE_CAP:1;    // Main counter is 64-bit wide if set
        U8 ReservedZero:1;      // Must be zero

        // If this bit is a 1, it indicates that the
        // hardware supports the LegacyReplacement Interrupt Route option.
        U8 LEG_ROUTE_CAP:1;

        // This read-only field will be the same as what would be assigned
        // if this logic was a PCI function. 
        U16 VENDOR_ID;

        //
        // This read-only field indicates the period at which the counter
        // increments in femptoseconds (10^-15 seconds). 
        // The value in this field must be less than or equal to 05F5E100h.
        //
        U32 COUNTER_CLK_PERIOD; // Counter increases (10^15 / COUNTER_CLK_PERIOD) per second

        //U64 REV_ID:8;
        //U64 NUM_TIM_CAP:5;
        //U64 COUNT_SIZE_CAP:1;
        //U64 Reserved:1;
        //U64 LEG_ROUTE_CAP:1;
        //U64 VENDOR_ID:16;
        //U64 COUNTER_CLK_PERIOD:32;
    };
    
    U64 Value;
} HPET_CAPABILITIES_AND_ID;

typedef union _HPET_CONFIGURATION_AND_CAPABILITIES
{
    struct
    {
        U16 ReservedZero1:1;
        U16 TN_INT_TYPE_CNF:1;      // [RW] Set to 1 for level-triggered, 0 for edge-triggered.
        U16 TN_INT_ENB_CNF:1;       // [RW] Interrupt enable bit. Timer still operates even if this bit is cleared.
        U16 TN_TYPE_CNF:1;          // [RW] Enables the timer to operate periodic mode if set to 1 (valid only TN_PER_INT_CAP is set)
        U16 TN_PER_INT_CAP:1;       // [RO] Supports periodic mode if set to 1
        U16 TN_SIZE_CAP:1;          // [RO] Indicates the size of the timer. 1 = 64-bits, 0 = 32-bits.

        //
        // Software uses this read/write bit only for timers that have been 
        // set to periodic mode. By writing this bit to a 1, the software is then 
        // allowed to directly set a periodic timer’s accumulator.
        //
        U16 TN_VAL_SET_CNF:1;       // [RW] Automatically cleared

        U16 ReservedZero2:1;
        U16 TN_32MODE_CNF:1;        // [RW] Forces 32-bit timer by setting this bit to 1

        // 
        // This 5-bit read/write field indicates the routing for the interrupt to the 
        // I/O APIC. A maximum value of 32 interrupts are supported.
        // 
        // @note : Must prove whether the value is valid after write...
        // 
        U16 TN_INT_ROUTE_CNF:5;     // [RW]

        U16 TN_FSB_EN_CNF:1;        // Related to FSB interrupt delivery. Not interesting...
        U16 TN_FSB_INT_DEL_CAP:1;   // Related to FSB interrupt delivery. Not interesting...

        U16 ReservedZero3;

        //
        // This 32-bit read-only field indicates to which interrupts in the I/O (x) APIC
        // this timer’s interrupt can be routed. This is used in conjunction with the
        // Tn_INT_ROUTE_CNF field. 
        //
        U32 TN_INT_ROUTE_CAP;       // [RO] 
    };
    
    U64 Value;
} HPET_CONFIGURATION_AND_CAPABILITIES;


//
// General configuration register bits.
//

#define HPET_GENERAL_CONFIGURATION_ENABLE_CNF           (1 << 0)
#define HPET_GENERAL_CONFIGURATION_LEG_RT_CNF           (1 << 1)
#define HPET_GENERAL_CONFIGURATION_VALID_MASK           (~(U64)(HPET_GENERAL_CONFIGURATION_ENABLE_CNF | HPET_GENERAL_CONFIGURATION_LEG_RT_CNF))



ESTATUS
HalHpetInitialize(
    VOID);

