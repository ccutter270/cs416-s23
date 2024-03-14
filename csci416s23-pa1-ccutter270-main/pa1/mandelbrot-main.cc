#include <getopt.h>
#include <algorithm>
#include <limits>
#include "Benchmark.h"
#include "CycleTimer.h"

// Uncomment the following line if you run into errors with std::align_val_t
//#define NO_ALIGN_VAL
#ifdef NO_ALIGN_VAL
#include <immintrin.h>
#endif

#include "mandelbrot.h"
// Include functions created by the ispc compiler
#include "mandelbrot_ispc.h"

using namespace ispc;

const int kRuns = 3;
const int kWidth = 1600;
const int kHeight = 1200;
const int kMaxIterations = 256;

int gTasks = 1;
int gThreads = 1;

// Specify expected options and usage
const char* kShortOptions = "s:t:h";
const struct option kLongOptions[] = {{"tasks", required_argument, nullptr, 's'},
                                      {"threads", required_argument, nullptr, 't'},
                                      {"help", no_argument, nullptr, 'h'},
                                      {nullptr, 0, nullptr, 0}};

void PrintUsage(const char* program_name) {
  printf("Usage: %s [options]\n", program_name);
  printf("Options:\n");
  printf("  -s  --tasks <INT>    Run ISPC implementation with tasks, default: %d\n", gTasks);
  printf(
      "  -t  --threads <INT>  Run C++ threads implementation with specified threads, default: %d\n",
      gThreads);
  printf("  -h  --help         Print this message\n");
}

/**
 * @brief Reset image buffer to zeros
 *
 * @param width Image width
 * @param height Image height
 * @param output Image buffer
 */
void ResetImageOutput(int width, int height, int output[]);

/**
 * @brief Compare results between implementations
 * 
 * @param width Image width
 * @param height Image height
 * @param ref_output Reference output
 * @param output Test output
 * @return true If the reference and test output match exactly
 * @return false If the reference and test output differ
 */
bool CompareMandelbrotResults(int width, int height, int ref_output[], int output[]);

int main(int argc, char** argv) {
  {
    int opt;
    while ((opt = getopt_long(argc, argv, kShortOptions, kLongOptions, nullptr)) != -1) {
      switch (opt) {
        case 't':
          gThreads = atoi(optarg);
          break;
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

  float x0 = -2;
  float x1 = 1;
  float y0 = -1;
  float y1 = 1;



  #ifndef NO_ALIGN_VAL
  // Here we use the new C++17 standard approach to ensure memory alignment (as required by AVX instructions)
  int* output_ref = new (std::align_val_t(32)) int[kWidth * kHeight];
  int* output_test = new (std::align_val_t(32)) int[kWidth * kHeight];
  #else
  // If you are not using C++17 there are other approaches for aligned allocation (https://stackoverflow.com/a/32612833).
  // Here we use an allocater provided by the intrinsics library (which unfortunately has its own free...).
  int* output_ref = (int*)_mm_malloc(kWidth * kHeight * sizeof(int), 32);
  int* output_test = (int*)_mm_malloc(kWidth * kHeight * sizeof(int), 32);
  #endif

  double min_serial = Benchmark(kRuns, MandelbrotSerial, x0, y0, x1, y1, kWidth, kHeight,
                                kMaxIterations, output_ref, 0, kHeight, 0, kWidth);
  printf("[mandelbrot serial]:\t\t%.3f ms\t%.3fX speedup\n", min_serial * 1000, 1.);
  WritePPM(output_ref, kWidth, kHeight, "mandelbrot-serial.ppm");

  ResetImageOutput(kWidth, kHeight, output_test);
  double min_threads = Benchmark(kRuns, MandelbrotThreads, x0, y0, x1, y1, kWidth, kHeight,
                                 kMaxIterations, output_test, gThreads);
  printf("[mandelbrot %d threads]:\t%.3f ms\t%.3fX speedup\n", gThreads, min_threads * 1000,
         min_serial / min_threads);
  if (!CompareMandelbrotResults(kWidth, kHeight, output_ref, output_test)) {
    fprintf(stderr, "Threads[%d] implementation doesn't match serial implementation\n", gThreads);
    return 1;
  }
  WritePPM(output_test, kWidth, kHeight, "mandelbrot-threads.ppm");

  ResetImageOutput(kWidth, kHeight, output_test);
  double min_ispc = Benchmark(kRuns, MandelbrotISPC, x0, y0, x1, y1, kWidth, kHeight,
                              kMaxIterations, output_test);
  printf("[mandelbrot ispc]:\t%.3f ms\t%.3fX speedup\n", min_ispc * 1000, min_serial / min_ispc);
  if (!CompareMandelbrotResults(kWidth, kHeight, output_ref, output_test)) {
    fprintf(stderr, "ispc implementation doesn't match serial implementation\n");
    return 1;
  }
  WritePPM(output_test, kWidth, kHeight, "mandelbrot-ispc.ppm");

  ResetImageOutput(kWidth, kHeight, output_test);
  double min_ispc_tasks = Benchmark(kRuns, MandelbrotISPCTasks, x0, y0, x1, y1, kWidth, kHeight,
                                    kMaxIterations, output_test, gTasks);
  printf("[mandelbrot ispc %d tasks]:\t%.3f ms\t%.3fX speedup\n", gTasks, min_ispc_tasks * 1000,
         min_serial / min_ispc_tasks);
  if (!CompareMandelbrotResults(kWidth, kHeight, output_ref, output_test)) {
    fprintf(stderr, "ispc tasks[%d] implementation doesn't match serial implementation\n", gTasks);
    return 1;
  }
  WritePPM(output_test, kWidth, kHeight, "mandelbrot-ispc-tasks.ppm");

  #ifndef NO_ALIGN_VAL
  delete[] output_ref;
  delete[] output_test;
  #else
  _mm_free(output_ref);
  _mm_free(output_test);
  #endif
}

void ResetImageOutput(int width, int height, int output[]) {
  memset(output, 0, kWidth * kHeight * sizeof(int));
}

bool CompareMandelbrotResults(int width, int height, int ref_output[], int output[]) {
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      int index = j * width + i;
      if (ref_output[index] != output[index]) {
        fprintf(stderr, "Error at index (%d, %d) - Expected value: %d, Actual value: %d\n", j, i,
                ref_output[index], output[index]);
        return false;
      }
    }
  }
  return true;
}
