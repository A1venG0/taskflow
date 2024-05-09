#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <taskflow/taskflow.hpp>
#include <taskflow/algorithm/for_each.hpp>
#include <chrono>

TEST_CASE("ArenaCorrectness" * doctest::timeout(300)) {
    for (size_t outerIdx = 0; outerIdx < 1000; ++outerIdx)
    {
        tf::Executor e;



        tf::Taskflow taskflow;
        std::shared_ptr<tf::TaskArena> arena = std::make_shared<tf::TaskArena>(e, 0);
        //tf::TaskArena* arena = new tf::TaskArena(e, 0);



        thread_local bool is_outer_task_running = false;
        is_outer_task_running = false;
        //std::cerr << outerIdx << ' ' << arena.use_count() << std::endl;
        try
        {
            taskflow.for_each_index(0, 32, 1, [&](int i)
                {
                    CHECK(is_outer_task_running == false);



                    is_outer_task_running = true;
                    e.isolate(arena, [&] {
                        // Inside of this functor our worker cannot take tasks that are older than this task (e.g. from the outer for_each_index)
                        // We can achieve this by switching current thread to another task queue
                        tf::Taskflow child_taskflow;
                        child_taskflow.for_each_index(0, 2, 1, [&](int j) {}); // These tasks will be spawned inside of our new TaskQueue of TaskArena "arena"
                        e.corun(child_taskflow); // Guaranteed to never run tasks from the outer loop
                        });
                    //tf::Taskflow child_taskflow;
                    //child_taskflow.for_each_index(0, 2, 1, [&](int j) {}); // These tasks will be spawned inside of our new TaskQueue of TaskArena "arena"
                    // e.corun(child_taskflow); // Guaranteed to never run tasks from the outer loop
                    is_outer_task_running = false;
                });
            CHECK(e.run(taskflow).wait_for(std::chrono::seconds(15)) == std::future_status::ready);
        }
        catch (const std::exception&)
        {
            std::cout << "Exception occurred" << '\n';
        }
        //delete arena;
    }
}



TEST_CASE("NestedArenas" * doctest::timeout(300)) {
    tf::Executor e;

    tf::Taskflow taskflow;
    /*tf::TaskArena arena(e, 0);
    tf::TaskArena second_arena(e, 1);*/
    std::shared_ptr<tf::TaskArena> arena = std::make_shared<tf::TaskArena>(e, 0);
    std::shared_ptr<tf::TaskArena> second_arena = std::make_shared<tf::TaskArena>(e, 1);

    thread_local bool is_outer_task_running = false;
    try
    {
        taskflow.for_each_index(0, 32, 1, [&](int i)
        {
            CHECK(is_outer_task_running == false);

            is_outer_task_running = true;
            e.isolate(arena, [&] {
                // Inside of this functor our worker cannot take tasks that are older than this task (e.g. from the outer for_each_index)
                // We can achieve this by switching current thread to another task queue
                tf::Taskflow child_taskflow;
                child_taskflow.for_each_index(0, 2, 1, [&](int j) { // These tasks will be spawned inside of our new TaskQueue of TaskArena "arena"
                    e.isolate(second_arena, [&] {
                        tf::Taskflow third_taskflow;
                        third_taskflow.for_each_index(0, 4, 1, [&](int k) {});
                        e.corun(third_taskflow);
                        });
                    });
                e.corun(child_taskflow); // Guaranteed to never run tasks from the outer loop
                });
            //tf::Taskflow child_taskflow;
            //child_taskflow.for_each_index(0, 2, 1, [&](int j) {}); // These tasks will be spawned inside of our new TaskQueue of TaskArena "arena"
            // e.corun(child_taskflow); // Guaranteed to never run tasks from the outer loop
            is_outer_task_running = false;
        });
        CHECK(e.run(taskflow).wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    }
    catch (const std::exception&)
    {
        std::cout << "Exception occurred" << '\n';
    }
}
