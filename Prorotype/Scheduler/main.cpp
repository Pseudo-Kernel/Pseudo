
#include <stdio.h>
#include <map>
#include <vector>
#include <thread>
#include <atomic>

#include <intrin.h>
#include <Windows.h>
#include "types.h"
#include "list.h"

#define DASSERT(_x)             if (!(_x)) { __debugbreak(); }


//
// task scheduler prototype.
//

namespace prototype
{
    struct v128
    {
        int32_t v4[4];
    };

    struct v256
    {
        int32_t v4[8];
    };

    //
    // task context.
    //

    __declspec(align(32)) struct task_context
    {
        intptr_t Xax;
        intptr_t Xcx;
        intptr_t Xdx;
        intptr_t Xbx;
        intptr_t Xsi;
        intptr_t Xdi;
        intptr_t Xbp;
        intptr_t Xsp;

        intptr_t R8;  // x86-64 (only in 64-bit mode)
        intptr_t R9;  // x86-64 (only in 64-bit mode)
        intptr_t R10; // x86-64 (only in 64-bit mode)
        intptr_t R11; // x86-64 (only in 64-bit mode)
        intptr_t R12; // x86-64 (only in 64-bit mode)
        intptr_t R13; // x86-64 (only in 64-bit mode)
        intptr_t R14; // x86-64 (only in 64-bit mode)
        intptr_t R15; // x86-64 (only in 64-bit mode)

        intptr_t Xip;
        intptr_t Xflags;
        intptr_t Reserved[6];

        v256 Ymm0; // x86
        v256 Ymm1; // x86
        v256 Ymm2; // x86
        v256 Ymm3; // x86
        v256 Ymm4; // x86
        v256 Ymm5; // x86
        v256 Ymm6; // x86
        v256 Ymm7; // x86

        v256 Ymm8;  // x86-64 (only in 64-bit mode)
        v256 Ymm9;  // x86-64 (only in 64-bit mode)
        v256 Ymm10; // x86-64 (only in 64-bit mode)
        v256 Ymm11; // x86-64 (only in 64-bit mode)
        v256 Ymm12; // x86-64 (only in 64-bit mode)
        v256 Ymm13; // x86-64 (only in 64-bit mode)
        v256 Ymm14; // x86-64 (only in 64-bit mode)
        v256 Ymm15; // x86-64 (only in 64-bit mode)
    };

    //
    // task structure.
    //

    struct ktask
    {
        long lock;
        int id;
        task_context context;
        DLIST_ENTRY list;
        uint64_t cpu_time_total;
        uint64_t cpu_time_delta;
        uint64_t cpu_time_starve;
        uint32_t priority;
        uint32_t priority_acceleration;

        class scheduler *scheduler_object;
    };

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

    //
    // task pool.
    //

    class task_object_pool
    {
    public:
        task_object_pool() : task_id_next_(1), lock_(0)
        {
        }

        ktask *allocate(uint32_t priority)
        {
            int id = task_id_next_++;
            ktask task{};
            task.id = id;
            task.priority = priority;

            acquire_lock(&lock_);
            task_map_[id] = task;
            release_lock(&lock_);

            return &task_map_[id];
        }

        bool free(int id)
        {
            bool result = false;

            acquire_lock(&lock_);

            do
            {
                auto &it = task_map_.find(id);
                if (it == task_map_.end())
                    break;

                task_map_.erase(id);
                result = true;
            } while (false);

            release_lock(&lock_);

            return result;
        }

    private:
        std::map<int, ktask> task_map_;
        std::atomic_int task_id_next_;
        long lock_;
    };

    uint32_t task_get_real_priority(ktask *task)
    {
        return task->priority + task->priority_acceleration;
    }

    //
    // scheduler class.
    //

    class scheduler
    {
    public:
        virtual ktask *get_next_task() = 0;
        virtual bool add_task(ktask *task) = 0;
        virtual bool remove_task(ktask *task) = 0;
    };

    class scheduler_normal : public scheduler
    {
    public:
        scheduler_normal(uint32_t levels) : levels_(levels), wait_head_(levels), ready_head_(levels)
        {
            for (uint32_t i = 0; i < levels; i++)
            {
                DListInitializeHead(&wait_head_[i]);
                DListInitializeHead(&ready_head_[i]);
            }
        }

        virtual ktask *get_next_task()
        {
            ktask *task = nullptr;

            acquire_lock(&lock_);

            for (int i = static_cast<int>(levels_) - 1; i >= 0; i--)
            {
                if (DListIsEmpty(&ready_head_[i]))
                    continue;

                task = CONTAINING_RECORD(ready_head_[i].Next, ktask, list);
                break;
            }

            if (!task)
            {
                // no more tasks to schedule.
                // reload them from wait_head_.
                for (int i = static_cast<int>(levels_) - 1; i >= 0; i--)
                {
                    if (DListIsEmpty(&wait_head_[i]))
                        continue;

                    if (!task)
                        task = CONTAINING_RECORD(wait_head_[i].Next, ktask, list);

                    DListMoveAfter(&ready_head_[i], &wait_head_[i]);
                }
            }

            release_lock(&lock_);

            return task;
        }

        virtual bool add_task(ktask *task)
        {
            if (task->scheduler_object)
                return false;

            auto lock_pointers = { &task->lock, &lock_ };

            acquire_lock_n(lock_pointers);

            task->scheduler_object = this;
            DListInitializeHead(&task->list);
            uint32_t real_priority = task_get_real_priority(task);
            DListInsertBefore(&wait_head_[real_priority], &task->list);

            release_lock_n(lock_pointers);

            return true;
        }

        virtual bool remove_task(ktask *task)
        {
            if (!task->scheduler_object || 
                task->scheduler_object != this)
                return false;

            auto lock_pointers = { &task->lock, &lock_ };

            acquire_lock_n(lock_pointers);

            task->scheduler_object = nullptr;
            DListRemoveEntry(&task->list);
            DListInitializeHead(&task->list);

            release_lock_n(lock_pointers);

            return true;
        }

    private:
        void acquire()
        {
            while (!try_acquire());
        }

        bool try_acquire()
        {
            return try_acquire_lock(&lock_);
        }

        void release()
        {
            release_lock(&lock_);
        }

        std::vector<DLIST_ENTRY> ready_head_;
        std::vector<DLIST_ENTRY> wait_head_;
        uint32_t levels_;
        long lock_;
    };

    //
    // simulates context switching and timer IRQ.
    //

    class vcpu
    {
        struct vector_number
        {
            static const int apic_timer = 0x20;
        };

    public:
        vcpu() :
            handle_apic_timer_(nullptr),
            apic_id_(0),
            sched_(nullptr),
            interruptable_(false),
            halt_forever_(false),
            current_task_(&initial_task_),
            initial_task_{},
            vcpu_thread_()
        {
            vcpu_thread_ = std::thread([this] {
                init();
                idle();
            });
        }

        void wait_for_break()
        {
            if (vcpu_thread_.joinable())
                vcpu_thread_.join();
        }

        void set_interruptable(bool interruptable)
        {
            // simulates "cli" and "sti" instruction
            interruptable_ = interruptable;
        }

        void register_task(ktask *task)
        {
            sched_->add_task(task);
        }

    private:
        static void APIENTRY apic_timer_interrupt(void *this_ptr, uint32_t lo, uint32_t hi)
        {
            vcpu *p = reinterpret_cast<vcpu *>(this_ptr);
            p->call_interrupt(vector_number::apic_timer, nullptr);
        }

        void call_interrupt(int vector, void *context)
        {
            if (!interruptable_)
                return;

            switch (vector)
            {
            case vector_number::apic_timer:
                isr_apic_timer(vector, context);
                break;

            default:
                break;
            }
        }

        void isr_apic_timer(int vector, void *context)
        {
            //printf("%s: irq hit\n", __func__);
            auto task = sched_->get_next_task();

            if (task)
            {
                //
                // swap thread
                //

                auto prev_task = current_task_;
                if (prev_task)
                {
                    // add to the scheduler
                    DASSERT(sched_->add_task(prev_task));
                }

                current_task_ = task;
                sched_->remove_task(task);

                //
                // do context switch.
                //

                context_switch(prev_task, task);
                printf("swap task %d -> %d\n", prev_task->id, task->id);
            }
        }

        void context_switch(ktask *curr, ktask *next)
        {
            //
            // 1. save current context to curr
            // 2. load context from next
            //

            // todo: not implemented yet
        }

        void init()
        {
            apic_id_ = next_apic_id_++;

            sched_ = std::make_unique<scheduler_normal>(32);

            char name[64];
            sprintf_s(name, "VCPU%d_INTERRUPT_APIC_TIMER", apic_id_);
            handle_apic_timer_ = CreateWaitableTimerA(nullptr, FALSE, name);
            DASSERT(handle_apic_timer_);

            LARGE_INTEGER due_time{ };
            long period = 1000 / 100; // 10ms
            DASSERT(SetWaitableTimer(handle_apic_timer_, &due_time, period, 
                reinterpret_cast<PTIMERAPCROUTINE>(apic_timer_interrupt), 
                reinterpret_cast<void *>(this), FALSE));
        }

        void halt()
        {
            // simulates "hlt" instruction
            do
            {
                SleepEx(INFINITE, TRUE);
            } while (!interruptable_);
        }

        void idle()
        {
            while (true)
            {
                halt();
                printf("woke up by irq\n");
            }
        }


    private:
        // kernel-related
        std::unique_ptr<scheduler> sched_;
        ktask *current_task_;
        ktask initial_task_;

        // architectural state
        void *handle_apic_timer_;
        int apic_id_;
        std::thread vcpu_thread_;
        std::atomic_bool interruptable_;
        std::atomic_bool halt_forever_;

        static std::atomic_int next_apic_id_;
    };

    std::atomic_int vcpu::next_apic_id_;
}


int main()
{
    prototype::task_object_pool pool;
    prototype::vcpu vcpu0;

    std::vector<prototype::ktask *> tasks;
    
    // allocate 100 tasks with priority
    for (int i = 0; i < 100; i++)
    {
        tasks.push_back(pool.allocate(i / 10));
    }

    // register task to cpu
    for (auto &it : tasks)
    {
        vcpu0.register_task(it);
    }

    // enable timer interrupt to perform task switching
    vcpu0.set_interruptable(true);

    // wait for thread termination for vcpu0
    vcpu0.wait_for_break();

    return 0;
}

