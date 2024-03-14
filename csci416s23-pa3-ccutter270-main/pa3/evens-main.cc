#include <getopt.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <random>
#include "Benchmark.h"

const int kRuns = 3;
const int kN = 20 * 1000 * 1000;

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
 * @brief Return true if evens result array matches expected values
 * 
 * @param n Length of the input array
 * @param indices_count Number of evens in actual data
 * @param indices Actual data array
 * @param indices_ref_count Number of evens in reference data
 * @param indices_ref Actual reference data
 */
bool CompareEvensResults(int n, int indices_count, int indices[], int indices_ref_count,
                         int indices_ref[]);

/**
 * @brief Populate array with indices of even values
 * 
 * @param n Length of the input array
 * @param values Input array
 * @param indices Output array of indices
 * @param indices_count Number of even values detected
 */
void EvensSerial(int n, int values[], int indices[], int* indices_count);

/**
 * @brief Populate array with indices of even values using CUDA and block-level CUB
 * 
 * @param n Length of the input array
 * @param values Input array
 * @param indices Output array of indices
 * @param indices_count Number of even values detected
 */
void EvensBlockCUB(int n, int values[], int indices[], int* indices_count);

/**
 * @brief Populate array with indices of even values using CUDA and device-level CUB
 * 
 * @param n Length of the input array
 * @param values Input array
 * @param indices Output array of indices
 * @param indices_count Number of even values detected
 */
void EvensDeviceCUB(int n, int values[], int indices[], int* indices_count);



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

  int* values = new int[kN];
  int* indices = new int[kN];
  int* indices_ref = new int[kN];

  int indices_count = 0;
  int indices_ref_count = 0;

  // Generate uniform random numbers in the range 0-100
  std::default_random_engine rand_engine;
  auto rand_dist = std::uniform_int_distribution<int>(0, 100);
  for (int i = 0; i < kN; i++) {
    values[i] = rand_dist(rand_engine);
  }

  double min_serial = Benchmark(kRuns, EvensSerial, kN, values, indices_ref, &indices_ref_count);
  printf("[evens serial]:\t\t%.3f ms\t%.3fX speedup\n", min_serial * 1000, 1.);

  double min_block = Benchmark(kRuns, EvensBlockCUB, kN, values, indices, &indices_count);
  printf("[evens cuda block]:\t\t%.3f ms\t%.3fX speedup\n", min_block * 1000, min_serial / min_block);
  if (!CompareEvensResults(kN, indices_count, indices, indices_ref_count, indices_ref)) {
    fprintf(stderr, "CUDA CUB block implementation doesn't satisfy accuracy requirement\n");
    return 1;
  }

  double min_device = Benchmark(kRuns, EvensDeviceCUB, kN, values, indices, &indices_count);
  printf("[evens cuda device]:\t\t%.3f ms\t%.3fX speedup\n", min_device * 1000,
         min_serial / min_device);
  if (!CompareEvensResults(kN, indices_count, indices, indices_ref_count, indices_ref)) {
    fprintf(stderr, "CUDA CUB device implementation doesn't satisfy accuracy requirement\n");
    return 1;
  }

  delete[] values;
  delete[] indices;

  return 0;
}

bool CompareEvensResults(int n, int indices_count, int indices[], int indices_ref_count,
                         int indices_ref[]) {
  if (indices_count >= n || indices_count != indices_ref_count) {
    fprintf(stderr, "Error in output count- Expected value: %d, Actual value: %d\n",
            indices_ref_count, indices_count);
    return false;
  }
  for (int i = 0; i < indices_count; i++) {
    if (indices[i] != indices_ref[i]) {
      fprintf(stderr, "Error at index (%d) - Expected value: %d, Actual value: %d\n", i,
              indices_ref[i], indices[i]);
      return false;
    }
  }
  return true;
}

void EvensSerial(int n, int values[], int indices[], int* indices_count) {
  int even_count = 0;
#pragma clang loop vectorize(disable)
  for (int i = 0; i < n; i++) {
    if (values[i] % 2 == 0) indices[even_count++] = i;
  }
  *indices_count = even_count;
}