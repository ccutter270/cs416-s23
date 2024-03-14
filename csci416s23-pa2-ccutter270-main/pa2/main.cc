#include <getopt.h>
#include <cstdio>
#include <cstdlib>
#include "tasksys.h"
#include "test/tasks.h"

const int kRuns = 3;
int gThreads = 1;

// Specify expected options and usage
const char* kShortOptions = "t:n:lr:h";
const struct option kLongOptions[] = {{"threads", required_argument, nullptr, 't'},
                                      {"name", required_argument, nullptr, 'n'},
                                      {"list", no_argument, nullptr, 'l'},
                                      {"runner", required_argument, nullptr, 'r'},
                                      {"help", no_argument, nullptr, 'h'},
                                      {nullptr, 0, nullptr, 0}};

void PrintUsage(const char* program_name) {
  printf("Usage: %s [options]\n", program_name);
  printf("Options:\n");
  printf(
      "  -t  --threads <INT>  Run C++ threads implementation with specified threads, default: %d\n",
      gThreads);
  printf("  -n --name <NAME>     Run the test with <NAME>\n");
  printf("  -l --list            List the available tests and exit\n");
  printf("  -r --runner <NAME>   Use the test runner with <NAME>\n");
  printf("  -h  --help           Print this message\n");
}

enum class TaskRunnerKind : int {
  kSerial = 0,
  kSpawn,
  kSpin,
  kSleep,
  kMaxKind,
};

TaskRunner* TaskRunnerFactory(TaskRunnerKind kind, int num_threads) {
  switch (kind) {
    case TaskRunnerKind::kSerial:
      return new TaskRunnerSerial();
    case TaskRunnerKind::kSpawn:
      return new TaskRunnerSpawn(num_threads);
    case TaskRunnerKind::kSpin:
      return new TaskRunnerSpin(num_threads);
    case TaskRunnerKind::kSleep:
      return new TaskRunnerSleep(num_threads);
    default:
      return nullptr;
  }
}

const char* TaskRunnerName(TaskRunnerKind kind) {
  switch (kind) {
    case TaskRunnerKind::kSerial:
      return "TaskRunnerSerial";
    case TaskRunnerKind::kSpawn:
      return "TaskRunnerSpawn";
    case TaskRunnerKind::kSpin:
      return "TaskRunnerSpin";
    case TaskRunnerKind::kSleep:
      return "TaskRunnerSleep";
    default:
      assert(false);
      return "";
  }
}

int main(int argc, char** argv) {
  std::string test_name;
  std::string test_runner;
  {
    int opt;
    while ((opt = getopt_long(argc, argv, kShortOptions, kLongOptions, nullptr)) != -1) {
      switch (opt) {
        case 't':
          gThreads = atoi(optarg);
          break;
        case 'n':
          test_name = optarg;
          break;
        case 'l':
          for (auto& test : kTestFunctions) {
            printf("%s\n", test.second);
          }
          return 0;
        case 'r':
          test_runner = optarg;
          break;
        case 'h':
          PrintUsage(argv[0]);
          return 0;
        case '?':  // Unrecognized option
        default:
          PrintUsage(argv[0]);
          return 1;
      }
    }
  }

  for (auto& test : kTestFunctions) {
    // Run just the specified test
    if (!test_name.empty() && test_name != test.second) {
      continue;
    }

    printf("Test [%d threads]: %s\n", gThreads, test.second);
    for (int i = 0; i < static_cast<int>(TaskRunnerKind::kMaxKind); i++) {
      // Execute just the specified runner
      const char* runner_name = TaskRunnerName(static_cast<TaskRunnerKind>(i));
      if (!test_runner.empty() && test_runner != runner_name) {
        continue;
      }

      double min_time = std::numeric_limits<double>::max();
      for (int j = 0; j < kRuns; j++) {
        // Create a new task system
        TaskRunner* runner = TaskRunnerFactory(static_cast<TaskRunnerKind>(i), gThreads);

        // Run test
        TestResult result = test.first(*runner);

        // Check that the test result was correct
        if (!result.correct_) {
          fprintf(
              stderr,
              "Error: Correctness check for failed for test %s with runner %s on iteration %d\n",
              test.second, runner_name, j);
          exit(1);  // Exit with non-zero code on error
        }

        min_time = std::min(min_time, result.exec_time_);

        // Clean up task runner
        delete runner;
      }
      printf("[%s]:\t\t%.3f ms\n", runner_name, min_time * 1000);
      fflush(NULL); // Try to flush any pending print operations
    }
  }
}