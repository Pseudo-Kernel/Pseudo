
/**
 * @file irql.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements IRQL-related routines.
 * @version 0.1
 * @date 2021-10-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <base/base.h>
#include <ke/irql.h>
#include <init/bootgfx.h>


/**
 * @brief Returns current IRQL.
 * 
 * @return Current IRQL.
 */
KIRQL
KERNELAPI
KeGetCurrentIrql(
    VOID)
{
    // IRQL = LAPIC_TPR[7:4] = CR8[3:0]
    return (KIRQL)(__readcr8() & 0x0f);
}

/**
 * @brief Sets the current IRQL to given IRQL.\n
 * 
 * @param [in] TargetIrql   Desired IRQL (<= CurrentIrql).
 * 
 * @return Previous IRQL.
 */
KIRQL
KERNELAPI
KeLowerIrql(
    IN KIRQL TargetIrql)
{
    KIRQL CurrentIrql = KeGetCurrentIrql();

    if (TargetIrql > CurrentIrql)
    {
        FATAL("IRQL is high (current %d, target %d)", CurrentIrql, TargetIrql);
    }

    // IRQL = LAPIC_TPR[7:4] = CR8[3:0]
    __writecr8(TargetIrql);

    return CurrentIrql;
}

/**
 * @brief Sets the current IRQL to given IRQL.\n
 * 
 * @param [in] TargetIrql   Desired IRQL (>= CurrentIrql).
 * 
 * @return Previous IRQL.
 */
KIRQL
KERNELAPI
KeRaiseIrql(
    IN KIRQL TargetIrql)
{
    KIRQL CurrentIrql = KeGetCurrentIrql();

    if (TargetIrql < CurrentIrql)
    {
        FATAL("IRQL is low (current %d, target %d)", CurrentIrql, TargetIrql);
    }

    // IRQL = LAPIC_TPR[7:4] = CR8[3:0]
    __writecr8(TargetIrql);

    return CurrentIrql;
}

