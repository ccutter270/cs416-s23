/**
 * @file tasks.h
 *
 * Adapted in part from CS149 at Stanford.
 */
#pragma once
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <map>
#include <thread>
#include "CycleTimer.h"
#include "tasksys.h"

class TestResult {
 public:
  TestResult() : correct_(true), exec_time_(0.) {}

  bool correct_;
  double exec_time_;
};

typedef TestResult (*TestFunction)(TaskRunner&);

class ValidatorTask : public Runnable {
 public:
  ValidatorTask(int n) : task_counts_(n, 0) {}

  void RunTask(int task_id, int num_tasks) override {
    assert(num_tasks == static_cast<int>(task_counts_.size()) && task_id < num_tasks);
    task_counts_[task_id]++;
  }

  bool Valid() const {
    return std::all_of(task_counts_.begin(), task_counts_.end(), [](int c) { return c == 1; });
  }

 protected:
  std::vector<int> task_counts_;
};

class FastTask : public Runnable {
 public:
  FastTask(int output[]) : output_(output) {}

  void RunTask(int task_id, int num_tasks) override { output_[task_id] = task_id; }

 protected:
  int* output_;
};

class RecursiveFibonacciTask : public Runnable {
 public:
  RecursiveFibonacciTask(int output[], int n) : output_(output), n_(n) {}

  static inline int RecursiveFibonacci(int n) {
    if (n == 0)
      return 0;
    else if (n == 1)
      return 1;
    else
      return RecursiveFibonacci(n - 1) + RecursiveFibonacci(n - 2);
  }

  void RunTask(int task_id, int num_tasks) override { output_[task_id] = RecursiveFibonacci(n_); }

 protected:
  int* output_;
  int n_;
};

class PingPongTask : public Runnable {
 public:
  PingPongTask(int num_elements, int* input_array, int* output_array, bool equal_work,
               int iterations)
      : num_elements_(num_elements),
        input_array_(input_array),
        output_array_(output_array),
        equal_work_(equal_work),
        iterations_(iterations) {}

  inline int NumIterations(int i) const {
    int max_iters = 2 * iterations_;
    return std::floor((static_cast<float>(num_elements_ - i) / num_elements_) * max_iters);
  }

  static inline int Work(int iterations, int input) {
    int accum = input;
    for (int j = 0; j < iterations; j++) {
      if (j % 2 == 0) accum++;
    }
    return accum;
  }

  void RunTask(int task_id, int num_tasks) override {
    // Handle case where num_elements_ is not evenly divisible by num_total_tasks
    int elements_per_task = (num_elements_ + num_tasks - 1) / num_tasks;
    int start_index = elements_per_task * task_id;
    int end_index = std::min(start_index + elements_per_task, num_elements_);

    if (equal_work_) {
      for (int i = start_index; i < end_index; i++)
        output_array_[i] = Work(iterations_, input_array_[i]);
    } else {
      for (int i = start_index; i < end_index; i++) {
        int iterations = NumIterations(i);
        output_array_[i] = Work(iterations, input_array_[i]);
      }
    }
  }

 protected:
  int num_elements_;
  int* input_array_;
  int* output_array_;
  bool equal_work_;
  int iterations_;
};

class SleepTask : public Runnable {
 public:
  SleepTask() : seconds_(1) {}
  SleepTask(int seconds) : seconds_(seconds) {}

  void RunTask(int task_id, int num_total_tasks) override {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fprintf(stderr, "Completed task with ID %d (of %d tasks)\n", task_id, num_total_tasks);
  }

 protected:
  int seconds_;
};

TestResult PingPongTest(TaskRunner& runner, bool equal_work, int num_elements, int base_iterations,
                        int num_tasks = 64, int num_bulk_task_launches = 400) {
  std::vector<int> input(num_elements);
  std::vector<int> output(num_elements);

  // Initialize inputs
  for (int i = 0; i < num_elements; i++) {
    input[i] = i;
    output[i] = 0;
  }

  // Ping-pong input and output buffers with all the back-to-back task launches
  std::vector<PingPongTask> runnables;
  for (int i = 0; i < num_bulk_task_launches; i++) {
    if (i % 2 == 0)
      runnables.emplace_back(num_elements, input.data(), output.data(), equal_work,
                             base_iterations);
    else
      runnables.emplace_back(num_elements, output.data(), input.data(), equal_work,
                             base_iterations);
  }

  // Run the test
  double start_time = CycleTimer::currentSeconds();

  for (int i = 0; i < num_bulk_task_launches; i++) {
    runner.Run(&runnables[i], num_tasks);
  }

  double end_time = CycleTimer::currentSeconds();

  // Correctness validation
  TestResult results;

  // Number of ping-pongs determines which buffer to look at for the results
  auto& buffer = (num_bulk_task_launches % 2 == 1) ? output : input;

  for (int i = 0; i < num_elements; i++) {
    int expected = i;
    for (int j = 0; j < num_bulk_task_launches; j++) {
      int iters = equal_work ? base_iterations : runnables[j].NumIterations(i);
      expected += (iters + 1) / 2;
    }

    if (buffer[i] != expected) {
      results.correct_ = false;
      fprintf(stderr, "PingPong error at index (%d) - Expected value: %d, Actual value: %d\n", i,
              expected, buffer[i]);
      break;
    }
  }
  results.exec_time_ = end_time - start_time;

  return results;
}

TestResult FewUnbalancedTest(TaskRunner& runner, int num_small_tasks = 1, int num_med_tasks = 2) {
  std::vector<int> small_output(num_small_tasks, 0);
  std::vector<int> med_output(num_med_tasks, 0);

  FastTask small_task(small_output.data());
  RecursiveFibonacciTask med_task(med_output.data(), 40);

  // Run the test
  double start_time = CycleTimer::currentSeconds();

  runner.Run(&small_task, num_small_tasks);
  runner.Run(&med_task, num_med_tasks);
  runner.Run(&small_task, num_small_tasks);

  double end_time = CycleTimer::currentSeconds();

  // Correctness validation
  TestResult results;
  for (int i = 0; i < num_med_tasks; i++) {
    if (med_output[i] != 102334155) {
      results.correct_ = false;
      fprintf(stderr,
              "FewUnbalancedTest error at index (%d) - Expected value: %d, Actual value: %d\n", i,
              102334155, med_output[i]);
      break;
    }
  }
  results.exec_time_ = end_time - start_time;

  return results;
}

TestResult SpinBetweenTasks(TaskRunner& runner) { return FewUnbalancedTest(runner); }

TestResult SuperSuperLightTest(TaskRunner& runner) {
  const int num_elements = 32 * 1024;
  const int base_iters = 0;
  return PingPongTest(runner, true, num_elements, base_iters);
}

TestResult SuperLightTest(TaskRunner& runner) {
  const int num_elements = 32 * 1024;
  const int base_iters = 32;
  return PingPongTest(runner, true, num_elements, base_iters);
}

TestResult PingPongEqualTest(TaskRunner& runner) {
  const int num_elements = 512 * 1024;
  const int base_iters = 32;
  return PingPongTest(runner, true, num_elements, base_iters);
}

TestResult PingPongUnequalTest(TaskRunner& runner) {
  const int num_elements = 512 * 1024;
  const int base_iters = 32;
  return PingPongTest(runner, false, num_elements, base_iters);
}

TestResult OnlyRunsTaskOnce(TaskRunner& runner) {
  const int num_launches = 2;
  std::vector<ValidatorTask> runnables;
  for (int i = 0; i < num_launches; i++) {
    runnables.emplace_back(100);
  }

  // Run the test
  double start_time = CycleTimer::currentSeconds();

  for (int i = 0; i < num_launches; i++) {
    runner.Run(&runnables[i], 100);
  }

  double end_time = CycleTimer::currentSeconds();

  // Correctness validation
  TestResult results;
  for (int i = 0; i < num_launches; i++) {
    if (!runnables[i].Valid()) {
      results.correct_ = false;
      fprintf(stderr,
              "ValidatorTask error on launch %d, not all tasks run only once\n", i);
      break;
    }
  }
  results.exec_time_ = end_time - start_time;

  return results;
}

#define TEST_FUNCTION(name) \
  { name, #name }

const std::map<TestFunction, const char*> kTestFunctions = {
    // clang-format off
    TEST_FUNCTION(SuperSuperLightTest), 
    TEST_FUNCTION(SuperLightTest),
    TEST_FUNCTION(PingPongEqualTest),
    TEST_FUNCTION(PingPongUnequalTest),
    TEST_FUNCTION(SpinBetweenTasks),
    TEST_FUNCTION(OnlyRunsTaskOnce),
    // clang-format on
};

#undef TEST_FUNCTION
