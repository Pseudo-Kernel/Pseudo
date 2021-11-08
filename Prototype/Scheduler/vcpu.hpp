#pragma once

#include "prototype_base.h"

#include "task.hpp"

namespace prototype
{
    //
    // simulates context switching and timer.
    //

    std::atomic_uint64_t global_test_counter = 1;

    class vcpu
    {
    public:
        vcpu() :
            apic_id_(0),
            sched_(nullptr),
            interruptable_(false),
            current_task_(&initial_task_),
            initial_task_{},
            vcpu_context_runner_thread_(),
            vcpu_timer_thread_()
        {
            init();

            // 
            // WARNING: Do not call wait functions inside context runner thread
            //          as it may cause undefined behavior!
            //          (SetThreadContext may not work as expected even if it returns TRUE)
            // 

            vcpu_context_runner_thread_ = std::thread([this] {
                kernel_initial_task_start_entry();
            });
        }

        void wait_for_break()
        {
            if (vcpu_context_runner_thread_.joinable())
                vcpu_context_runner_thread_.join();
        }

        void set_interruptable(bool interruptable)
        {
            interruptable_ = interruptable;
        }

        void register_task(ktask *task)
        {
            sched_->queue_task(task);
        }

        void halt_control(bool resume)
        {
            auto handle = vcpu_context_runner_thread_.native_handle();
            if (resume)
            {
                ResumeThread(handle);
                set_interruptable(true);
            }
            else
            {
                set_interruptable(false);
                SuspendThread(handle);
            }
        }

        static void task_start_entry(void *p)
        {
            while (true)
            {
                //DASSERT(task == reinterpret_cast<decltype(task)>(p));
                auto task = reinterpret_cast<prototype::ktask *>(p);
                auto priv_data = &task->priv_data;

                priv_data->counter++;
                global_test_counter++;
            }
        }

    private:

        bool remote_context_switch()
        {
            auto handle = vcpu_context_runner_thread_.native_handle();
            if (GetCurrentThreadId() == GetThreadId(handle))
                return false;

            auto task = sched_->peek_next_task();

            if (task)
            {
                DASSERT(task != current_task_);
                
                //
                // swap thread
                //

                auto prev_task = current_task_;
                if (prev_task)
                {
                    // add to the scheduler
                    DASSERT(sched_->queue_task(prev_task));
                }

                current_task_ = task;
                sched_->remove_task(task);

                //
                // do context switch.
                //

                context_switch(prev_task, task);

#if DEBUG_DETAILED_LOG
                con.printf("swap task %d -> %d  (cnt %11lld, cnt %11lld)\n",
                    prev_task->id, task->id,
                    prev_task->priv_data.counter, task->priv_data.counter);
                con.printf("\n");
#else
                con.printf_xy(0, 1, "swap task %2d -> %2d  ", prev_task->id, task->id);
#endif
            }

            return true;
        }

        void context_switch(ktask *curr, ktask *next)
        {
            auto handle = vcpu_context_runner_thread_.native_handle();

            if (curr->is_busy)
            {
#if 1
                curr->recent_context_run_start = 0;
                curr->recent_context_run_end = 0;
                curr->recent_cpu_time_delta = platform_wall_clock::sec_to_counter() / 1000;
                curr->is_busy = false;
#else
                // update timeslice
                curr->recent_context_run_end = platform_wall_clock::read_counter();
                curr->recent_cpu_time_delta = curr->recent_context_run_end - curr->recent_context_run_start;
                curr->is_busy = false;
#endif
            }

            DASSERT(!next->is_busy);
            next->is_busy = true;
            next->recent_context_run_start = platform_wall_clock::read_counter();
            next->recent_context_run_end = 0;

            //
            // 1. save current context to curr
            // 2. load context from next
            //

            DASSERT(SuspendThread(handle) != DWORD(-1));
            curr->context.ContextFlags = CONTEXT_ALL;
            DASSERT(GetThreadContext(handle, &curr->context));
            next->context.ContextFlags = CONTEXT_ALL;
            DASSERT(SetThreadContext(handle, &next->context));
            DASSERT(ResumeThread(handle) != DWORD(-1));
        }

        void init()
        {
            apic_id_ = next_apic_id_++;

            sched_ = std::make_unique<scheduler_normal>(32);

            vcpu_timer_thread_ = std::thread([this] {
                apic_timer_raise_irq_loop();
            });
        }

        void apic_timer_raise_irq_loop()
        {
            while (true)
            {
#if DEBUG_DETAILED_LOG
                if (current_task_)
                    con.printf("task %d runs\n", current_task_->id);
#endif

                Sleep(global_timeslice_ms);

                if (interruptable_)
                    remote_context_switch();
            }
        }

        void kernel_initial_task_start_entry()
        {
            while (true)
            {
                _mm_pause();
            }
        }

    private:
        // kernel-related
        std::unique_ptr<scheduler> sched_;
        ktask *current_task_;
        ktask initial_task_;

        // architectural state
        int apic_id_;
        std::thread vcpu_context_runner_thread_;
        std::thread vcpu_timer_thread_;
        std::atomic_bool interruptable_;

        static std::atomic_int next_apic_id_;
    };

    std::atomic_int vcpu::next_apic_id_;
}
