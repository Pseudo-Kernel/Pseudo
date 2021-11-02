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

#define DASSERT(_x)             if (!(_x)) { __debugbreak(); }

