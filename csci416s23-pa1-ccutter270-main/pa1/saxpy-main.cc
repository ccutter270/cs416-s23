#include <getopt.h>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <limits>
#include "CycleTimer.h"
extern "C" {
  // Prevent C++ from mangling the names (and causing link time errors)
  #include "cblas.h"
}

#ifndef HAVE_ALIGN_VAL
// include intrinsics to get aligned malloc when align_val_t is not available
#include <immintrin.h>
#endif

#include "saxpy_ispc.h"

using namespace ispc;

const int kRuns = 3;
const int kN = 20 * 1000 * 1000;
const float kAlpha = 0.2f;

int gTasks = 1;
int gThreads = 1;

// Specify expected options and usage
const char* kShortOptions = "s:h";
const struct option kLongOptions[] = {{"tasks", required_argument, nullptr, 's'},
                                      {"help", no_argument, nullptr, 'h'},
                                      {nullptr, 0, nullptr, 0}};

void PrintUsage(const char* program_name) {
  printf("Usage: %s [options]\n", program_name);
  printf("Options:\n");
  printf("  -s  --tasks <INT>    Run ISPC implementation with tasks, default: %d\n", gTasks);
  printf("  -h  --help           Print this message\n");
}

/**
 * @brief Initialize SAXPY inputs with random values
 *
 * @param n Array length
 * @param x "X" operand array
 * @param y "Y" operand array
 */
void InitSaxpyInputs(int n, float x[], float y[]);

bool CompareSaxpyResults(int n, float result[], float result_ref[], float epsilon=0.0001);

/**
 * @brief Run SAXPY function num_runs times, reporting minimum time
 *
 * @param num_runs Number of benchmark runs
 * @param fn Function to execute
 * @param n Array length
 * @param alpha "A" in SAXPY
 * @param x "X" operand array
 * @param y "Y" operand array
 * @param args Any additional arguments to function
 * @return double
 */
template <class Fn, class... Args>
double SaxpyBenchmark(int num_runs, Fn&& fn, int n, float alpha, float x[], float y[],
                      Args&&... args) {
  double min_time = std::numeric_limits<double>::max();
  for (int i = 0; i < num_runs; i++) {
    InitSaxpyInputs(kN, x, y);
    double start_time = CycleTimer::currentSeconds();
    fn(kN, alpha, x, y, std::forward<Args>(args)...);
    double end_time = CycleTimer::currentSeconds();
    min_time = std::min(min_time, end_time - start_time);
  }
  return min_time;
}

/**
 * @brief Serial SAXPY implementation. Computes Y=alpha*X+Y.
 *
 * @param n Array length
 * @param alpha "A" in SAXPY
 * @param x "X" operand array
 * @param y "Y" operand array
 */
void SaxpySerial(int n, float alpha, float x[], float y[]);

/**
 * @brief Wrapper for BLAS SAXPY function
 *
 * @param n Array length
 * @param alpha "A" in SAXPY
 * @param x "X" operand array
 * @param y "Y" operand array
 */
void SaxpyCBLAS(int n, float alpha, float x[], float y[]);

int main(int argc, char** argv) {
  {
    int opt;
    while ((opt = getopt_long(argc, argv, kShortOptions, kLongOptions, nullptr)) != -1) {
      switch (opt) {
        case 's':
          gTasks = atoi(optarg);
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
  float* x_array = new (std::align_val_t(32)) float[kN];
  float* y_array_ref = new (std::align_val_t(32)) float[kN];
  float* y_array = new (std::align_val_t(32)) float[kN];
  #else
  // If you are not using C++17 there are other approaches for aligned allocation (https://stackoverflow.com/a/32612833).
  // Here we use an allocater provided by the intrinsics library (which unfortunately has its own free...).
  float* x_array = (float*)_mm_malloc(kN * sizeof(float), 32); 
  float* y_array_ref = (float*)_mm_malloc(kN * sizeof(float), 32);
  float* y_array = (float*)_mm_malloc(kN * sizeof(float), 32);
  #endif

  double min_serial = SaxpyBenchmark(kRuns, SaxpySerial, kN, kAlpha, x_array, y_array_ref);
  printf("[saxpy serial]:\t%.3f ms\t%.3fX speedup\n", min_serial * 1000, 1.f);

  double min_blas = SaxpyBenchmark(kRuns, SaxpyCBLAS, kN, kAlpha, x_array, y_array);
  printf("[saxpy blas]:\t%.3f ms\t%.3fX speedup\n", min_blas * 1000, min_serial / min_blas);
  if (!CompareSaxpyResults(kN, y_array, y_array_ref)) {
    fprintf(stderr, "Blas implementation doesn't satisfy accuracy requirement\n");
    return 1;
  }

  double min_ispc = SaxpyBenchmark(kRuns, SaxpyISPC, kN, kAlpha, x_array, y_array);
  printf("[saxpy ispc]:\t%.3f ms\t%.3fX speedup\n", min_ispc * 1000, min_serial / min_ispc);
  if (!CompareSaxpyResults(kN, y_array, y_array_ref)) {
    fprintf(stderr, "IPSC implementation doesn't satisfy accuracy requirement\n");
    return 1;
  }

  double min_ispc_tasks =
      SaxpyBenchmark(kRuns, SaxpyISPCTasks, kN, kAlpha, x_array, y_array, gTasks);
  printf("[saxpy ispc %d tasks]:\t%.3f ms\t%.3fX speedup\n", gTasks, min_ispc_tasks * 1000,
         min_serial / min_ispc_tasks);
  if (!CompareSaxpyResults(kN, y_array, y_array_ref)) {
    fprintf(stderr, "ISPC tasks implementation doesn't satisfy accuracy requirement\n");
    return 1;
  }

  #ifdef HAVE_ALIGN_VAL
  delete[] x_array;
  delete[] y_array_ref;
  delete[] y_array;
  #else
  _mm_free(x_array);
  _mm_free(y_array_ref);
  _mm_free(y_array);
  #endif

  return 0;
}

void InitSaxpyInputs(int n, float x[], float y[]) {
  for (int i = 0; i < n; i++) {
    x[i] = i;
    y[i] = i;
  }
}

bool CompareSaxpyResults(int n, float result[], float result_ref[], float epsilon) {
  for (int i = 0; i < n; i++) {
    if (fabs(result[i] - result_ref[i]) > epsilon) {
      fprintf(stderr, "Error at index (%d) - Expected value: %f, Actual value: %f\n", i,
              result_ref[i], result[i]);
      return false;
    }
  }
  return true;
}

void SaxpySerial(int n, float alpha, float x[], float y[]) {
#pragma clang loop vectorize(disable)
  for (int i = 0; i < n; i++) {
    y[i] = alpha * x[i] + y[i];
  }
}

void SaxpyCBLAS(int n, float alpha, float x[], float y[]) { cblas_saxpy(n, alpha, x, 1, y, 1); }