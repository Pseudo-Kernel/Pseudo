#pragma once

#include "prototype_base.h"

#include "task.hpp"

namespace prototype
{
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

        void traverse(std::function<bool(ktask&)> f)
        {
            acquire_lock(&lock_);

            for (auto& it : task_map_)
            {
                if (!f(it.second))
                    break;
            }

            release_lock(&lock_);
        }

    private:
        std::map<int, ktask> task_map_;
        std::atomic_int task_id_next_;
        long lock_;
    };

    task_object_pool task_pool;
}
