#pragma once

#include "prototype_base.h"

namespace prototype
{
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

        std::vector<DLIST_ENTRY> ready_head_;
        std::vector<DLIST_ENTRY> wait_head_;
        uint32_t levels_;
        long lock_;
    };
}
