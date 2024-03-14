#include <getopt.h>

#include <algorithm>
#include <cstdio>
#include <random>

#include "Benchmark.h"

#ifndef HAVE_ALIGN_VAL
// include intrinsics to get aligned malloc when align_val_t is not available
#include <immintrin.h>
#endif

#include "sqrt.h"
#include "sqrt_ispc.h"

using namespace ispc;

const int kRuns = 3;
const int kN = 20 * 1000 * 1000;

int gThreads = 1;
bool gIntrinsics = false;

// Specify expected options and usage
const char* kShortOptions = "t:hi";
const struct option kLongOptions[] = {{"help", no_argument, nullptr, 'h'},
                                      {"threads", required_argument, nullptr, 't'},
                                      {"intrinsics", no_argument, nullptr, 'i'},
                                      {nullptr, 0, nullptr, 0}};

void PrintUsage(const char* program_name) {
  printf("Usage: %s [options]\n", program_name);
  printf("Options:\n");
  printf("  -h  --help           Print this message\n");
  printf(
      "  -t  --threads <INT>  Run C++ threads implementation with specified threads, default: %d\n",
      gThreads);
  printf("  -i  --intrinsics     Run SIMD intrinsics implementation of sqrt\n");
}

/**
 * @brief Compare computed results to double-precision sqrt in standard library
 *
 * @param n Length of input and output arrays
 * @param value Input array
 * @param result Output array
 * @param epsilon Tolerance for correct result
 * @return true All results within epsilon of the reference implementation
 * @return false One or more results differ from reference implementation by than epsilon
 */
bool CompareSqrtResults(int n, float value[], float result[], double epsilon = 0.0001);

/**
 * @brief Reset output to zeros
 *
 * @param n Length of output array
 * @param output Output array
 */
void ResetSqrtOutput(int n, float output[]);

int main(int argc, char** argv) {
  {
    int opt;
    while ((opt = getopt_long(argc, argv, kShortOptions, kLongOptions, nullptr)) != -1) {
      switch (opt) {
        case 't':
          gThreads = atoi(optarg);
          break;
        case 'i':
          gIntrinsics = true;
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

#ifdef HAVE_ALIGN_VAL
  float* values = new (std::align_val_t(32)) float[kN];
  float* output = new (std::align_val_t(32)) float[kN];
#else
  // If you are not using C++17 there are other approaches for aligned allocation
  // (https://stackoverflow.com/a/32612833). Here we use an allocator provided by the intrinsics
  // library (which unfortunately has its own free...).
  float* values = (float*)_mm_malloc(kN * sizeof(float), 32);
  float* output = (float*)_mm_malloc(kN * sizeof(float), 32);
#endif

  // TODO: Modify this initialization code to test different input configurations, e.g.
  // change the inputs assigned to values[i] in the loop below.

  // Generate uniform random numbers in the range 0.001-2.999 (0 and 3 won't converge)
  std::default_random_engine rand_engine;
  auto rand_dist = std::uniform_real_distribution<float>(0.001, 2.999);
  for (int i = 0; i < kN; i++) {
    values[i] = rand_dist(rand_engine);
  }

  double min_serial = Benchmark(kRuns, SqrtSerial, kN, 1.f, values, output);
  printf("[sqrt serial]:\t\t%.3f ms\t%.3fX speedup\n", min_serial * 1000, 1.);
  if (!CompareSqrtResults(kN, values, output)) {
    fprintf(stderr, "Serial implementation doesn't satisfy accuracy requirement\n");
    return 1;
  }

  ResetSqrtOutput(kN, output);
  double min_ispc = Benchmark(kRuns, SqrtISPC, kN, 1.f, values, output);
  printf("[sqrt ispc]:\t\t%.3f ms\t%.3fX speedup\n", min_ispc * 1000, min_serial / min_ispc);
  if (!CompareSqrtResults(kN, values, output)) {
    fprintf(stderr, "ISPC implementation doesn't satisfy accuracy requirement\n");
    return 1;
  }

  ResetSqrtOutput(kN, output);
  double min_ispc_tasks = Benchmark(kRuns, SqrtISPCTasks, kN, 1.f, values, output);
  printf("[sqrt ispc tasks]:\t\t%.3f ms\t%.3fX speedup\n", min_ispc_tasks * 1000,
         min_serial / min_ispc_tasks);
  if (!CompareSqrtResults(kN, values, output)) {
    fprintf(stderr, "ISPC tasks implementation doesn't satisfy accuracy requirement\n");
    return 1;
  }

  // Don't modify this initialization code to ensure consistent performance results
  {  // Use fixed seed to performance comparisons are consistent
    rand_engine.seed(42);
    auto rand_dist = std::uniform_real_distribution<float>(0.001, 2.999);
    for (int i = 0; i < kN; i++) {
      values[i] = rand_dist(rand_engine);
    }
  }

  if (gIntrinsics) {
    ResetSqrtOutput(kN, output);
    double min_intrinsics = Benchmark(kRuns, SqrtIntrinsics, kN, 1.f, values, output);
    printf("[sqrt intrinsics]:\t\t%.3f ms\t%.3fX speedup\n", min_intrinsics * 1000,
           min_serial / min_intrinsics);
    if (!CompareSqrtResults(kN, values, output)) {
      fprintf(stderr, "Intrinsics implementation doesn't satisfy accuracy requirement\n");
      return 1;
    }

  }

  double min_serial_leaderboard = Benchmark(kRuns, SqrtSerial, kN, 1.f, values, output);
  ResetSqrtOutput(kN, output);
  double min_ispc_leaderboard = Benchmark(kRuns, SqrtISPC, kN, 1.f, values, output);
  printf("[sqrt ispc (leaderboard)]:\t\t%.3f ms\t%.3fX speedup\n", min_ispc_leaderboard * 1000,
         min_serial_leaderboard / min_ispc_leaderboard);
  if (!CompareSqrtResults(kN, values, output)) {
    fprintf(stderr, "ISPC implementation doesn't satisfy accuracy requirement\n");
    return 1;
  }

#ifdef HAVE_ALIGN_VAL
  delete[] values;
  delete[] output;
#else
  _mm_free(values);
  _mm_free(output);
#endif

  return 0;
}

bool CompareSqrtResults(int n, float value[], float result[], double epsilon) {
  for (int i = 0; i < n; i++) {
    double expected = sqrt(static_cast<double>(value[i]));
    if (fabs((static_cast<double>(result[i]) / expected) - 1) > epsilon) {
      fprintf(stderr, "Error at index (%d) - Expected value: %f, Actual value: %f\n", i, expected,
              result[i]);
      return false;
    }
  }
  return true;
}

void ResetSqrtOutput(int n, float output[]) { memset(output, 0, n * sizeof(float)); }
