#include <getopt.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include "CycleTimer.h"
extern "C" {
// Prevent C++ from mangling the names (and causing link time errors)
#include "cblas.h"
}

const int kRuns = 3;
const int kN = 20 * 1000 * 1000;
const float kAlpha = 0.2f;

// Specify expected options and usage
const char* kShortOptions = "h";
const struct option kLongOptions[] = {{"help", no_argument, nullptr, 'h'},
                                      {nullptr, 0, nullptr, 0}};

void PrintUsage(const char* program_name) {
  printf("Usage: %s [options]\n", program_name);
  printf("Options:\n");
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

/**
 * @brief Return true if SAXPY results match expected values
 *
 * @param n Array length
 * @param result Actual results array
 * @param result_ref Expected results array
 * @param epsilon Equality tolerance
 */
bool CompareSaxpyResults(int n, float result[], float result_ref[], float epsilon = 0.0001);

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

/**
 * @brief Wrapper for CUDA SAXPY function
 *
 * @param n Array length
 * @param alpha "A" in SAXPY
 * @param x "X" operand array
 * @param y "Y" operand array
 */
void SaxpyCUDA(int n, float alpha, float x[], float y[]);

/**
 * @brief Wrapper for CUBLAS SAXPY function
 *
 * @param n Array length
 * @param alpha "A" in SAXPY
 * @param x "X" operand array
 * @param y "Y" operand array
 */
void SaxpyCUDABLAS(int n, float alpha, float x[], float y[]);

int main(int argc, char** argv) {
  {
    int opt;
    while ((opt = getopt_long(argc, argv, kShortOptions, kLongOptions, nullptr)) != -1) {
      switch (opt) {
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

  float* x_array = new float[kN];
  float* y_array_ref = new float[kN];
  float* y_array = new float[kN];

  double min_serial = SaxpyBenchmark(kRuns, SaxpySerial, kN, kAlpha, x_array, y_array_ref);
  printf("[saxpy serial]:\t%.3f ms\t%.3fX speedup\n", min_serial * 1000, 1.f);

  double min_blas = SaxpyBenchmark(kRuns, SaxpyCBLAS, kN, kAlpha, x_array, y_array);
  printf("[saxpy blas]:\t%.3f ms\t%.3fX speedup\n", min_blas * 1000, min_serial / min_blas);
  if (!CompareSaxpyResults(kN, y_array, y_array_ref)) {
    fprintf(stderr, "Blas implementation doesn't satisfy accuracy requirement\n");
    return 1;
  }

  double min_cuda = SaxpyBenchmark(kRuns, SaxpyCUDA, kN, kAlpha, x_array, y_array);
  printf("[saxpy cuda]:\t%.3f ms\t%.3fX speedup\n", min_cuda * 1000, min_serial / min_cuda);
  if (!CompareSaxpyResults(kN, y_array, y_array_ref)) {
    fprintf(stderr, "CUDA implementation doesn't satisfy accuracy requirement\n");
    return 1;
  }

  double min_cuda_blas = SaxpyBenchmark(kRuns, SaxpyCUDABLAS, kN, kAlpha, x_array, y_array);
  printf("[saxpy cublas]:\t%.3f ms\t%.3fX speedup\n", min_cuda_blas * 1000,
         min_serial / min_cuda_blas);
  if (!CompareSaxpyResults(kN, y_array, y_array_ref)) {
    fprintf(stderr, "cublas implementation doesn't satisfy accuracy requirement\n");
    return 1;
  }

  delete[] x_array;
  delete[] y_array_ref;
  delete[] y_array;

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