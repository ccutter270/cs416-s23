#include <cuda.h>

#ifdef NDEBUG
#define cudaErrorCheck(ans) ans
#else
// Adapted from: https://stackoverflow.com/a/14038590 and cs149

/// Halt program if CUDA runtime call fails
/// You can use this as a wrapper for cuda calls, e.g. cudaErrorCheck( cudaMalloc(...) );
#define cudaErrorCheck(ans) { cudaAssert((ans), __FILE__, __LINE__); }

inline void cudaAssert(cudaError_t code, const char *file, int line, bool abort=true) {
  if (code != cudaSuccess) {
    fprintf(stderr,"CUDA Error: %s at %s:%d\n", cudaGetErrorString(code), file, line);
    if (abort)
      exit(code);
  }
}
#endif