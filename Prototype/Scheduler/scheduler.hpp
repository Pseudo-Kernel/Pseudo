#pragma once

#include "prototype_base.h"

namespace prototype
{
    struct priority_range
    {
        uint32_t start;
        uint32_t end;
    };

    //
    // scheduler class.
    //

    class scheduler
    {
    public:
        virtual ktask *peek_next_task() = 0;
        virtual bool queue_task(ktask *task) = 0;
        virtual bool remove_task(ktask *task) = 0;
        virtual void get_priority_range(priority_range &range) = 0;
    };

    class scheduler_normal : public scheduler
    {
    public:
        scheduler_normal(uint32_t levels) : 
            levels_(levels),
            wait_head_(levels),
            ready_head_(levels),
            next_ready_head_(levels),
            weight_(levels)
        {
            DASSERT(levels > 0);

            for (uint32_t i = 0; i < levels; i++)
            {
                DListInitializeHead(&wait_head_[i]);
                DListInitializeHead(&ready_head_[i]);
                DListInitializeHead(&next_ready_head_[i]);
            }

            set_weight();
        }

        void set_weight(std::function<uint32_t(uint32_t, uint32_t)> wf = nullptr)
        {
            auto weight_function = wf;
            if (!weight_function)
            {
                weight_function = [](uint32_t p, uint32_t c) {
                    return p + 1;
                };
            }

            for (uint32_t i = 0; i < levels_; i++)
            {
                weight_[i] = weight_function(i, levels_);
            }
        }

        uint32_t weight_to_level(uint32_t weight)
        {
            uint32_t level = 0;
            uint32_t delta_min = weight_[0] - weight;

            for (uint32_t i = 1; i < levels_; i++)
            {
                uint32_t delta = weight_[i] - weight;
                if (delta < delta_min)
                {
                    level = i;
                    delta_min = delta;
                }
            }

            return level;
        }

        virtual ktask *peek_next_task()
        {
            ktask *task = nullptr;

            acquire_lock(&lock_);

            std::pair<decltype(ready_head_)&, const char *> lists[] = {
                { next_ready_head_, "next_ready_list" },
                { wait_head_, "wait_list" },
            };

            for (int i = static_cast<int>(levels_) - 1; i >= 0; i--)
            {
                if (DListIsEmpty(&ready_head_[i]))
                    continue;

                task = CONTAINING_RECORD(ready_head_[i].Next, ktask, list);
#if DEBUG_DETAILED_LOG
                con.printf("peek next task %d from ready list %d\n", task->id, i);
#endif
                break;
            }

            if (!task)
            {
                for (auto& it : lists)
                {
                    for (int i = static_cast<int>(levels_) - 1; i >= 0; i--)
                    {
                        if (DListIsEmpty(&it.first[i]))
                            continue;

                        if (!task)
                            task = CONTAINING_RECORD(it.first[i].Next, ktask, list);

                        if (&it.first)
                        {
                            DListMoveAfter(&ready_head_[i], &it.first[i]);
                        }
#if DEBUG_DETAILED_LOG
                        con.printf("peek next task %d from %s %d\n", task->id, it.second, i);
#endif
//                        break;
                    }

                    if (task)
                        break;
                }
            }

            release_lock(&lock_);

            return task;
        }

        virtual bool queue_task(ktask *task)
        {
            if (task->scheduler_object)
                return false;

            auto lock_pointers = { &task->lock, &lock_ };

            acquire_lock_n(lock_pointers);

            // update timeslice before insert
            task_consume_timeslice(task, global_timeslice_ms);
            task_update_timeslice(task, global_timeslice_ms);

            int32_t timeslice = task->remaining_timeslice;

            task->scheduler_object = this;
            DListInitializeHead(&task->list);

            switch (task->state)
            {
            case queued_state::not_queued:
                // * -> wait
                task->real_priority_dyn = task->priority;
                DListInsertBefore(&wait_head_[task->real_priority_dyn], &task->list);
                task->state = queued_state::wait;
                break;
            case queued_state::wait:
                // wait -> ready
                task->remaining_timeslice = weight_[task->real_priority_dyn];
                DListInsertBefore(&ready_head_[task->real_priority_dyn], &task->list);
                task->state = queued_state::ready;
                break;
            case queued_state::ready:
                // ready -> next_ready OR ready -> wait
                if (timeslice > 0)
                {
                    // not expired yet, increase priority.
                    //task->real_priority_dyn = weight_to_level(timeslice);
                    DListInsertBefore(&next_ready_head_[task->real_priority_dyn], &task->list);
                    task->state = queued_state::ready_not_expired;
                }
                else
                {
                    // expired. load base priority.
                    task->real_priority_dyn = task->priority;
                    DListInsertBefore(&wait_head_[task->real_priority_dyn], &task->list);
                    task->state = queued_state::wait;
                }
                break;
            case queued_state::ready_not_expired:
                // next_ready -> ...?
                DListInsertBefore(&ready_head_[task->real_priority_dyn], &task->list);
                task->state = queued_state::ready;
                break;
            default:
                DASSERT(false);
            }

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
            //task->state = queued_state::not_queued;
            DListRemoveEntry(&task->list);
            DListInitializeHead(&task->list);

            release_lock_n(lock_pointers);

            return true;
        }

        virtual void get_priority_range(priority_range &range)
        {
            range.start = 0;
            range.end = levels_ - 1;
        }

    private:
        void acquire()
        {
            while (!try_acquire())
                _mm_pause();
        }

        bool try_acquire()
        {
            return try_acquire_lock(&lock_);
        }

        void release()
        {
            release_lock(&lock_);
        }

        std::vector<uint32_t> weight_;

        std::vector<DLIST_ENTRY> ready_head_;
        std::vector<DLIST_ENTRY> wait_head_;
        std::vector<DLIST_ENTRY> next_ready_head_;
        uint32_t levels_;
        long lock_;
    };
}
