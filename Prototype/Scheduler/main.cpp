
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

    init();
    
    // allocate 100 tasks with priority
    for (int i = 0; i < 50; i++)
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


    for (;;)
    {
        vcpu0.halt_control(false);

        std::map<int, ktask *> task_map = {};
        task_pool.traverse([&task_map](ktask& task) {
            if (task.id)
            {
                task_map[task.id] = &task;
            }
            return true;
        });

        const int record_width = 27 + 1;
        int records_per_line = con.size_x() / record_width;
        const int base_x = 0;
        const int base_y = 4;

        int printed_count = 0;

        for (auto& it : task_map)
        {
            //con.set_xy(
            //    base_x + (printed_count % records_per_line) * record_width,
            //    base_y + (printed_count / records_per_line));

            con.printf_xy(
                base_x + (printed_count % records_per_line) * record_width,
                base_y + (printed_count / records_per_line),
            //  ddd.[pri.ddd]:.dddddddddd.
                "%3d [pri %3d]: %10lld ",
                it.first, 
                task_get_real_priority(it.second), 
                it.second->priv_data.counter);

            printed_count++;
        }

        vcpu0.halt_control(true);

        Sleep(100);
    }

    // wait for thread termination for vcpu0
    vcpu0.wait_for_break();


    shutdown();

    return 0;
}

