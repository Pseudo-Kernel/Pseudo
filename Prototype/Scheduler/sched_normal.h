
#pragma once
#include "runner_q.h"

#include "sched_base.h"

#define SCHED_NORMAL_CLASS_LEVELS           16

typedef struct _KSCHED_CONTEXT_NORMAL
{
    KRUNNER_QUEUE WaitQueue;
    KRUNNER_QUEUE ReadyQueue;
    KRUNNER_QUEUE RunnerSwapQueue;
} KSCHED_CONTEXT_NORMAL;

