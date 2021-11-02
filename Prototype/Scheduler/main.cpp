
/**
 * @file main.cpp
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Task scheduler prototype.
 * @version 0.1
 * @date 2021-10-30
 * 
 * @copyright Copyright (c) 2021
 * 
 * @todo Implement scheduler class. (round-robin, normal, idle)\n
 *       Implement wait functions.\n
 *       Implement task load balancing.\n
 */

#include "prototype_base.h"
#include "console.hpp"
#include "wall_clock.hpp"
#include "spinlock.hpp"

#include "task.hpp"
#include "task_pool.hpp"
#include "scheduler.hpp"
#include "vcpu.hpp"


using namespace prototype;

int main()
{
    vcpu vcpu0;

    std::vector<ktask *> tasks;
    
    // allocate 100 tasks with priority
    for (int i = 0; i < 100; i++)
    {
        auto task = task_pool.allocate(i / 10);
        tasks.push_back(task);

        DASSERT(task_set_initial_context(task, &vcpu::task_start_entry, task, 0));
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

