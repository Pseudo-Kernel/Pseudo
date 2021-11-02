#pragma once

#include "prototype_base.h"

namespace prototype
{
    //
    // simple spinlocks.
    //

    bool try_acquire_lock(long *lock)
    {
        return !!_InterlockedExchange((volatile long *)lock, 1);
    }

    void acquire_lock(long *lock)
    {
        while (!try_acquire_lock(lock))
            _mm_pause();
    }

    void release_lock(long *lock)
    {
        DASSERT(_InterlockedExchange((volatile long *)lock, 0) == 1);
    }

    bool try_acquire_lock_n(std::initializer_list<long *> lock_pointers)
    {
        long *locked_pointers[32];
        int locked_count = 0;
        bool all_locked = true;

        for (auto &lock_ptr : lock_pointers)
        {
            if (!try_acquire_lock(lock_ptr))
            {
                all_locked = false;
                break;
            }

            locked_pointers[locked_count] = lock_ptr;
            locked_count++;
        }

        if (!all_locked)
        {
            for (int i = locked_count - 1; i >= 0; i--)
            {
                // reverse order
                release_lock(locked_pointers[i]);
            }
        }

        return all_locked;
    }

    void acquire_lock_n(std::initializer_list<long *> lock_pointers)
    {
        while (!try_acquire_lock_n(lock_pointers))
            _mm_pause();
    }

    void release_lock_n(std::initializer_list<long *> lock_pointers)
    {
        if (!lock_pointers.size())
            return;

        auto it = lock_pointers.end();
        while (it != lock_pointers.begin())
        {
            // reverse order
            --it;
            release_lock(*it);
        }
    }
}
