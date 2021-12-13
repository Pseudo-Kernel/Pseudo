
#pragma once
#include <ke/sched_base.h>
#include <ke/sched_normal.h>


VOID
KiProcessorSchedInitialize(
    VOID);

ESTATUS
KiScheduleSwitchContext(
    IN KSTACK_FRAME_INTERRUPT *InterruptFrame);

VOID
KiYieldThreadInternal(
    IN KSTACK_FRAME_INTERRUPT *InterruptFrame);
