#include "tasksys.h"
#include <atomic>
#include <cassert>
#include <thread>

void TaskRunnerSerial::Run(Runnable* runnable, int num_tasks) {
  for (int i = 0; i < num_tasks; i++) {
    runnable->RunTask(i, num_tasks);
  }
}


void TaskRunnerSpawn::Run(Runnable* runnable, int num_tasks) {
  // TODO: Replace this placeholder implementation (used to ensure skeleton is runnable) with your
  // code
  TaskRunnerSerial serial;
  serial.Run(runnable, num_tasks);
}


void TaskRunnerSpin::Run(Runnable* runnable, int num_tasks) {
  // TODO: Replace this placeholder implementation (used to ensure skeleton is runnable) with your
  // code
  TaskRunnerSerial serial;
  serial.Run(runnable, num_tasks);
}

TaskRunnerSpin::TaskRunnerSpin(int num_threads) : num_threads_(num_threads) {}

TaskRunnerSpin::~TaskRunnerSpin() {}



void TaskRunnerSleep::Run(Runnable* runnable, int num_tasks) {
  // TODO: Replace this placeholder implementation (used to ensure skeleton is runnable) with your
  // code
  TaskRunnerSerial serial;
  serial.Run(runnable, num_tasks);
}

TaskRunnerSleep::TaskRunnerSleep(int num_threads) : num_threads_(num_threads) {}

TaskRunnerSleep::~TaskRunnerSleep() {}

