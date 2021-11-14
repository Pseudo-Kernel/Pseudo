
#pragma once
#include "runner_q.h"

#include "sched_base.h"

#define KSCHED_NORMAL_CLASS_LEVELS          16


BOOLEAN
KiSchedNormalInsertThread(
    IN KSCHED_CLASS *This,
    IN KTHREAD *Thread,
    IN PVOID SchedulerContext,
    IN U32 Queue,
    IN U32 Flags);

BOOLEAN
KiSchedNormalRemoveThread(
    IN KSCHED_CLASS *This,
    IN KTHREAD *Thread,
    IN PVOID SchedulerContext);

BOOLEAN
KiSchedNormalPeekThread(
    IN KSCHED_CLASS *This,
    OUT KTHREAD **Thread,
    IN PVOID SchedulerContext,
    IN U32 Queue,
    IN U32 Flags);

BOOLEAN
KiSchedNormalNextThread(
    IN KSCHED_CLASS *This,
    OUT KTHREAD **Thread,
    IN PVOID SchedulerContext);




