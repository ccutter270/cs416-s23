# CS416 Programming Assignment 2

This assignment consists of several implementation parts and several questions to answer. Modify this README file with your answers. Only modify the specified files in the specified way. Other changes will break the programs and/or testing infrastructure.

You are encouraged to develop and perform correctness testing on the platform of your choice, but you should answer the questions below based on your testing on the *ada* cluster, and specifically the compute nodes of the cluster (which have more cores than the head node). Recall that you don't access the compute nodes directly, instead you submit jobs to the queueing system (SLURM) which executes jobs on your behalf. Thus you will test your program by submitting cluster jobs and reviewing the output.

## Getting Started

1. Connect to *ada* via `ssh username@ada.middlebury.edu` (if you are off-campus you will need to use the VPN), logging in with your regular Middlebury username and password (i.e. replace `username` with your username).

    When you connect to *ada* you are logging into the cluster "head" node. You should use this node for compilation, interacting with GitHub, and other "administrative" tasks, but all testing will occur via submitting jobs to the cluster's queueing system.

2. Clone the assignment skeleton from GitHub to *ada* or copy from the files from your local computer.

3. Load the CS416 environment by executing the following:

    ```
    module use /home/mlinderman/modules/modulefiles
    module load cs416/s23
    ```

    These commands configure the environment so that you can access the tools we use in class. You will need to execute these commands every time you log in. Doing so is tedious, so instead you can add the two lines your `.bash_profile` file (at `~/.bash_profile`) so they execute every time you login.

4. Prepare the skeleton for compilation

    In the root directory of the skeleton, prepare for compilation by invoking `cmake3 .`. Note the *3*. You will need to explicitly invoke `cmake3` on *ada* to get the correct version, on other computers it will just be `cmake`. You only need to do this once.

5. Change to the assignment directory and compile the assignment programs

    ```
    cd pa2
    make
    ```

6. Submit a job to the cluster to test your programs

    The assignment directory includes `ada-submit`, a cluster submission script that invokes the assignment program. This script will request the entire node for testing but restrict your program to only using a specified number of cores (with a default of 8). You can change the number of cores with the `--cpus-per-task` option to `sbatch`. In the following example I submitted the script with the `sbatch` command, specifying 4 cores. SLURM (the cluster job queueing system) responded that it created job 4238.
    
    ```
    [mlinderman@ada pa1]$ sbatch --cpus-per-task 4 ada-submit 
    Submitted batch job 4238
    ```

    I then checked the cluster status to see if that job is running (or pending). If you don't see your job listed, it has already completed. 

    ```
    [mlinderman@ada pa1]$ squeue
             JOBID PARTITION     NAME     USER ST       TIME  NODES NODELIST(REASON)
              4238     short      pa1 mlinderm  R       0:09      1 node004
    ```

    And when it is done, I reviewed the results by examining the output file `pa2-4238.out` created by SLURM (i.e. `pa2-<JOB ID>.out`). Review the guide on Canvas for more details on working with *ada*.

Each subsequent time you work on the assignment you will need start with steps 1 and 3 above, then recompile the test program(s) and resubmit your cluster job each time you change your program.

## Introduction

The ISPC task system relies on a runtime that can execute the tasks, i.e., create tasks and assign them to different threads for execution. In this assignment you will implement a similar runtime that can execute "bulk synchronous" task launches using C++ threads. In the process you will learn about C++ threads and mechanisms for implementing data parallelism on shared memory systems.

Recall that every ISPC task is executing the same function, but with a unique `taskIndex` (that you can use to implement the data decomposition). We will use a similar approach. Our tasks will inherit from the abstract base class `Runnable`. Each task will need to implement (override) the `void RunTask(int task_id, int task_count)` method to perform the work of the task. The runtime system will be responsible invoking the `RunTask` method on the task object with the correct `task_id` and `task_count` (total number of tasks in the launch). 

For example the following task sleeps for some number of seconds and then prints its task ID.

```cpp
class SleepTask : public Runnable {
 public:
  SleepTask() : seconds_(1) {}
  SleepTask(int seconds) : seconds_(seconds) {}

  void RunTask(int task_id, int task_count) override {
    std::this_thread::sleep_for(std::chrono::seconds(seconds_));
    fprintf(stderr, "Completed task with ID %d (of %d tasks)\n", task_id, task_count);
  }

 protected:
  int seconds_;
};
```

We would use this task with our task runner as follows:

```cpp
SleepTask sleeper(2); // Each task sleeps for 2 seconds
TaskRunnerSerial runner; // Create serial task runner

// Bulk launch 10 tasks. Run method will return when tasks are complete.
runner.Run(&sleeper, 10); 
```

The task runner implements the `TaskRunner` abstract base class. The `Run` method bulk launches the specified number of tasks (10 in this example) for the supplied `Runnable` object. This example would print the following to standard error:

```
Completed task with ID 0 (of 10 tasks)
Completed task with ID 1 (of 10 tasks)
Completed task with ID 2 (of 10 tasks)
Completed task with ID 3 (of 10 tasks)
Completed task with ID 4 (of 10 tasks)
Completed task with ID 5 (of 10 tasks)
Completed task with ID 6 (of 10 tasks)
Completed task with ID 7 (of 10 tasks)
Completed task with ID 8 (of 10 tasks)
Completed task with ID 9 (of 10 tasks)
```

This approach is termed "bulk synchronous launch" because the `Run` function does *not* return until all of the tasks in that launch have *completed* (i.e. is "synchronous").

As the name suggests `TaskRunnerSerial` serially executes the tasks using a single thread. It will be the baseline. An implementation for `TaskRunnerSerial` is provided in `tasksys.cc`. Your assignment is to implement several multi-threaded tasks runners of increasing sophistication. The skeleton includes a set of test tasks designed to exercise your task runners in different ways so that you can investigate the advantages and disadvantages of the different approaches.

The skeleton includes starter code for three different runners, but with serial implementations for the `Run` method. You will replace the `Run` method with an appropriate implementation for each runner. You will likely also need to modify the class definition in `tasksys.h`.

You will use C++ threads to implement the different task runners (`TaskRunnerSpawn`, `TaskRunnerSpin` and `TaskRunnerSleep`). The constructor for the multi-threaded task runners takes a single parameter, `num_threads`, the maximum number of working threads to use. Depending on your implementation, you may also have an "original" or "main" thread, but you should never be executing more than `num_threads` task instances at any one time.

As preparation I suggest reviewing the lecture slides on using C++ threads, and particularly on the different synchronization primitives that are available to you. I suggest working on each task runner in turn, as each builds on the previous. When you have completed the implementation make sure to edit the README with answers to the questions below (at the end).

## Implementation

You can compile the main program for this assignment via `make pa2-main`. The `pa2-main` program will run various benchmarks using the different task runners (use the `-h` option to see the available options). You can change the number of threads with the `-t` option and specify a single test to run with the `-n` option (to list the available tests use the `-l` option, i.e. `./pa2-main -l`). For example the following would run a single benchmark named `SuperLightTest` with 4 threads.

```
./pa2-main -n SuperLightTest -t 4
Test [4 threads]: SuperLightTest
[TaskRunnerSerial]:             61.553 ms
[TaskRunnerSpawn]:              55.300 ms
[TaskRunnerSpin]:               40.761 ms
[TaskRunnerSleep]:              36.673 ms
```

The test workloads are implemented in `test/tasks.h`. Review these tests to get a sense of the different kinds of workloads that your task runners will need to execute efficiently. Some tests run many tasks, some only a few. For some tests all of the tasks do the same amount of work, in others the tasks do different amounts of work. 

### Implement `TaskRunnerSpawn`

The first and simplest runner you will implement, `TaskRunnerSpawn`, should spawn a new set of threads for each launch, i.e. each invocation of `Run`. Thus it will look similar to the previous "threaded" functions you have implemented. As in previous assignments you will need to think through how you want to assign tasks to the different threads to achieve good performance across all of the benchmarks. Different assignment strategies may require different synchronization approaches. For example is there any shared state, i.e. variables accessed by multiple threads, that you need to protect from simultaneous access?

### Implement `TaskRunnerSpin`

Launch a new set of threads for every task launch can introduce substantial overhead, especially when each task only does a small amount of work. We can reduce this overhead by implementing a "thread pool" that reuses the same set of threads for each launch. The `TaskRunnerSpin` runner should create a set of worker threads when it is first constructed (i.e. in the `TaskRunnerSpin` constructor) and reuse those threads for each launch. In between launches the threads should "spin", that is each thread should be executing a while loop that is repeatedly checking for new work to perform.

`TaskRunnerSpin` will be more complex than `TaskRunnerSpawn`, as it is now trickier to know when all of the work for a launch has been completed. I suggest building up to a fully working implementation in small steps:

1. Successfully launch the spinning threads in `TaskRunnerSpawn` constructor and stop the join the threads in `TaskRunnerSpawn`'s destructor. Do not detach threads, instead design your threads to terminate gracefully when the runner is destructed.
1. Successfully notify the worker threads that there is new work available. How will you communicate to the worker threads that new work is available and how will you synchronize reads and writes to that variable(s)?
1. Execute tasks on each of the worker threads. At this point, it may be more productive to test your runner with just the simple `SleepTask` or `ValidatorTask` in `test/task.h` instead of using all the tests.
1. Ensure that `Run` does not return until all the tasks are *completed*. This will require `Run` to "spin" until all of the worker threads have completed their tasks.

### Implement `TaskRunnerSleep`

Although the "spinning" threads are not doing any useful work while they are waiting for the next task or for the worker threads to complete tasks, the spinning is still consuming computational resources in ways that may negatively impact overall performance. To reduce the negative impacts of spinning we can put waiting threads to sleep (so they don't consume computational resources). As its name suggests, the `TaskRunnerSleep` runner will sleep threads waiting for some condition (i.e. for new work, or in `Run` for all the workers to complete).

One possible approach is to use the condition variable synchronization primitive. Threads that are "waiting" on a condition variable will go to sleep (and not consume any computation resources) until they are signaled by another thread. Review the lecture notes as condition variables require careful attention to lock management and care around missed/spurious "wake-ups".

## Testing

Submit the test program to the cluster as described above, e.g.

```
sbatch --cpus-per-task 8 ada-submit 
```

The `cpus-per-task` option constrains the number of cores that the cluster job can use. You can pass the `-n` argument to the script to run just a single test, e.g.

```
sbatch --cpus-per-task 8 ada-submit -n SpinBetweenTasks
```

Some of the most interesting behavior is observed when there are more threads the cores. If possible I encourage you to test the program on a computer with simultaneous multithreading (e.g. Intel Hyper-threading). Try creating twice as many threads as physical cores. Unfortunately Hyper-threading is disabled on *ada*. To observe similar example try the following the configuration on *ada*:

```
sbatch --cpus-per-task 4 ada-submit -t 16 -n SpinBetweenTasks
```
 
Your implementation will be tested against a reference implementation. You can run the reference implementation on *ada* by executing `pa2-ref` (if you have loaded the class module, the reference program will automatically be in your PATH). The performance target is speedups within 20% of the reference implementation, i.e., within 1.2 (ignore the performance of `OnlyRunsTaskOnce`, it is only used for correctness testing). 

## What to turn in

You should have modified `tasksys.h` and `tasksys.cc` with implementations for `TaskRunnerSpawn`, `TaskRunnerSpin` and `TaskRunnerSleep`. Answer the following questions by editing this README (i.e. place your answer after the question, update any tables, etc.).

1. Briefly describe how you implemented your task runners, including how you assigned work to threads (i.e. static or dynamic assignment) and how you ensured that all work was complete before returning from `Run`. What if any optimizations did you implement to improve performance (i.e. how did your implementation evolve)?

2. Test your implementation with 8 threads  

```
sbatch --cpus-per-task 8 ada-submit
```

The more complicated implementations are not always the most efficient. For what benchmarks (list by name) were the simpler implementations, i.e. `TaskRunnerSpawn`, or even the serial runner more than trivially faster? Why do you think that is the case? Ignore `OnlyRunsTaskOnce` which is just for correctness testing.

3. What did you observe when you ran with many more threads than cores, e.g.

```
sbatch --cpus-per-task 4 ada-submit -t 16
```

Which of your two different thread pools performed better in this "oversubscribed" situation? Why?

## Grading

Assessment | Requirements
-------|--------
**N**eeds revision | Some but not all components compile. Some but not all components are correct, or some but not all questions are answered, or none of the components meet the performance targets.
**M**eets Expectations | All components are correct, you meet the specified performance targets for at least one but not all benchmarks (excluding `OnlyRunsTaskOnce`) and your answers to the embedded questions meet expectations.
**E**xemplary | All requirements for *Meets Expectations* and all the benchmarks (excluding `OnlyRunsTaskOnce`) meet performance targets.

### Acknowledgements

This assignment was adapted from a course taught by Kayvon Fatahalian and Kunle Olukotun.
