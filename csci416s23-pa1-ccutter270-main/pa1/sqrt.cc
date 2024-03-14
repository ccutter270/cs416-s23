#include "sqrt.h"
#include <immintrin.h>
#include <cmath>
#include <cstdio>

static const float kTol = 0.00001f;

void SqrtSerial(int n, float initial_guess, float values[], float result[]) {
#pragma clang loop vectorize(disable)
  for (int i = 0; i < n; i++) {
    float guess = initial_guess;
    float value = values[i];

    // Compute 1/sqrt(value) via Newton's method (to within a ratio of kTol)
    float ratio = (3.f - value * guess * guess) * 0.5;
    while (fabs(ratio - 1.f) > kTol) {
      guess = guess * ratio;
      ratio = (3.f - value * guess * guess) * 0.5;
    }
    result[i] = value * guess;
  }
}

namespace {
/**
 * Print __m256 vector to stderr
 */
void print_intrinsic(__m256 vector) {
  // You will also see the GCC specific extension, __attribute__((aligned(16))), used to enforce
  // alignment
  alignas(32) float buffer[8];
  _mm256_store_ps(buffer, vector);
  for (int i = 0; i < 8; i++) {
    fprintf(stderr, "%f ", buffer[i]);
  }
  fprintf(stderr, "\n");
}

/**
 * Perform fabs on packed 8-wide vector by masking the sign bit
 */
inline __m256 _mm256_fabs_ps(__m256 value) {
  alignas(32) static const unsigned int mask[8] = {0x7FFFFFFFu, 0x7FFFFFFFu, 0x7FFFFFFFu,
                                                   0x7FFFFFFFu, 0x7FFFFFFFu, 0x7FFFFFFFu,
                                                   0x7FFFFFFFu, 0x7FFFFFFFu};
  return _mm256_and_ps(value, *(__m256*)mask);
}

/**
 * Return true if any of the sign bits in an 8-wide vector are true
 *
 * This works with SIMD comparison functions, e.g. _mm256_cmp_ps, which set all bits to true or
 * false
 */
inline bool _mm256_any_ps(__m256 cmp_vec) { return !_mm256_testz_ps(cmp_vec, cmp_vec); }


}  // namespace

void SqrtIntrinsics(int n, float initial_guess, float values[], float result[]) {
}

