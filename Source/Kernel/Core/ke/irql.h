#pragma once

#include <base/base.h>

typedef U32                                 KIRQL;


KIRQL
KERNELAPI
KeGetCurrentIrql(
    VOID);

KIRQL
KERNELAPI
KeLowerIrql(
    IN KIRQL TargetIrql);

KIRQL
KERNELAPI
KeRaiseIrql(
    IN KIRQL TargetIrql);
