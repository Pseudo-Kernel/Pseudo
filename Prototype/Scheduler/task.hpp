#pragma once

#include "prototype_base.h"

namespace prototype
{
    //
    // task structure.
    //

    struct ktask_private_data
    {
        uint64_t counter;
    };

    enum queued_state : int
    {
        not_queued,
        wait,
        ready,
        ready_not_expired,
    };

    struct ktask
    {
        long lock;
        int id;
        CONTEXT context;
        DLIST_ENTRY list;

        bool is_busy; // true if context is running
//        uint64_t cpu_time_total;
        uint64_t recent_cpu_time_delta;
        uint64_t recent_context_run_start;
        uint64_t recent_context_run_end;

        int32_t last_timeslice;
        int32_t remaining_timeslice;
        uint64_t remaining_cpu_time;

        uint32_t priority;
        uint32_t real_priority_dyn;
        queued_state state;
        queued_state prev_state;

        void *stack_base;
        uint32_t stack_size;

        class scheduler *scheduler_object;

        ktask_private_data priv_data;
    };

    int32_t task_update_timeslice(ktask *task, uint32_t ms_per_timeslice)
    {
        // NOTE: must check whether lock is held

        uint64_t counter_per_timeslice = 
            ms_per_timeslice * platform_wall_clock::sec_to_counter() / 1000;

        uint64_t remaining_cpu_time = task->remaining_cpu_time;
        int32_t remaining_timeslice = task->remaining_timeslice;

        uint64_t temp_cpu_time = remaining_cpu_time;  // !!!!!!!!!!!!!!!
        int32_t temp_timeslice = remaining_timeslice; // !!!!!!!!!!!!!!!

        if (remaining_cpu_time >= counter_per_timeslice)
        {
            remaining_timeslice += remaining_cpu_time / counter_per_timeslice;
            remaining_cpu_time %= counter_per_timeslice;

            task->remaining_timeslice = remaining_timeslice;
            task->remaining_cpu_time = remaining_cpu_time;
        }

        return remaining_timeslice;
    }

    int32_t task_consume_timeslice(ktask *task, uint32_t ms_per_timeslice)
    {
        // NOTE: must check whether lock is held

        uint64_t counter_per_timeslice =
            ms_per_timeslice * platform_wall_clock::sec_to_counter() / 1000;

        uint64_t consume_cpu_time = task->recent_cpu_time_delta;
        uint64_t consume_cpu_time_remainder = consume_cpu_time % counter_per_timeslice;

        int32_t remaining_timeslice = task->remaining_timeslice;
        uint64_t remaining_cpu_time = task->remaining_cpu_time;

        remaining_timeslice -= consume_cpu_time / counter_per_timeslice;

        if (consume_cpu_time_remainder > remaining_cpu_time)
        {
            remaining_cpu_time += counter_per_timeslice;
            remaining_timeslice--;
        }

        remaining_cpu_time -= consume_cpu_time_remainder;

//        if (remaining_timeslice < -20)
//            remaining_timeslice = -20;
//        else if (remaining_timeslice > 20)
//            remaining_timeslice = 20;

        task->remaining_timeslice = remaining_timeslice;
        task->remaining_cpu_time = remaining_cpu_time;
        //task->recent_cpu_time_delta = 0;

        return remaining_timeslice;
    }

    uint32_t task_get_real_priority(ktask *task)
    {
        return task->real_priority_dyn;
    }

    bool task_set_initial_context(ktask *task, void(*start_address)(void *), void *param, unsigned long stack_size)
    {
        if (!stack_size)
            stack_size = 0x80000;

        auto stack_base = VirtualAlloc(nullptr, stack_size, MEM_COMMIT, PAGE_READWRITE);
        if (!stack_base)
            return false;

        task->stack_base = stack_base;
        task->stack_size = stack_size;

#ifdef _X86_
        intptr_t *Xsp = reinterpret_cast<intptr_t *>((char *)stack_base + stack_size - 0x20);

        Xsp--;
        Xsp[0] = 0xdeadbeef; // set invalid return address
        Xsp[1] = reinterpret_cast<intptr_t>(param); // push 1st param

        memset(&task->context, 0, sizeof(task->context));
        task->context.ContextFlags = CONTEXT_ALL;// &~CONTEXT_SEGMENTS;
        task->context.Esp = reinterpret_cast<intptr_t>(Xsp);
        task->context.Eip = reinterpret_cast<intptr_t>(start_address);
#elif (defined _AMD64_)
        intptr_t *Xsp = reinterpret_cast<intptr_t *>((char *)stack_base + stack_size - 0x40);

        Xsp--;
        Xsp[0] = 0xfeedbaaddeadbeef; // set invalid return address

        memset(&task->context, 0, sizeof(task->context));
        task->context.ContextFlags = CONTEXT_ALL;// &~CONTEXT_SEGMENTS;
        task->context.Rsp = reinterpret_cast<intptr_t>(Xsp);
        task->context.Rip = reinterpret_cast<intptr_t>(start_address);
        task->context.Rcx = reinterpret_cast<intptr_t>(param); // set 1st param

#else
#error Requires x86 or amd64!
#endif

        return true;
    }
}
