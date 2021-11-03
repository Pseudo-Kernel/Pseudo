#pragma once

#include "prototype_base.h"

namespace prototype
{
    //
    // wall clock.
    //

    class platform_wall_clock
    {
    public:
        static uint64_t read_counter()
        {
            LARGE_INTEGER counter{};
            QueryPerformanceCounter(&counter);
            return counter.QuadPart;
        }

        static uint64_t counter_to_us(uint64_t counter)
        {
            LARGE_INTEGER freq{};
            QueryPerformanceFrequency(&freq);
            return counter / (freq.QuadPart / 1000000);
        }

        static uint64_t counter_to_ms(uint64_t counter)
        {
            LARGE_INTEGER freq{};
            QueryPerformanceFrequency(&freq);
            return counter / (freq.QuadPart / 1000);
        }

        static uint64_t sec_to_counter()
        {
            LARGE_INTEGER freq{};
            QueryPerformanceFrequency(&freq);
            return freq.QuadPart;
        }
    };

}
