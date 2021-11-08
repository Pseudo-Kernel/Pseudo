#pragma once

#include <stdio.h>
#include <map>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

#include <intrin.h>
#include <Windows.h>
#include "types.h"
#include "list.h"

#pragma comment(lib, "winmm.lib")

#define DASSERT(_x)             if (!(_x)) { __debugbreak(); }
#define DEBUG_DETAILED_LOG      0

namespace prototype
{
    void init()
    {
        timeBeginPeriod(1);
    }

    void shutdown()
    {
        timeEndPeriod(1);
    }
}

