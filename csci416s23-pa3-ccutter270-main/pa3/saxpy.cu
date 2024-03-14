#include <cstdio>
#include <cuda.h>
#include <cuda_runtime.h>
#include "cublas_v2.h"
#include "CycleTimer.h"
#include "cuda-util.h"

/**
 * @brief Saxpy CUDA kernel
 * 
 * The __global__ keyword indicates that this is a device function that can be called from
 * the host via <<< ... >> kernel launch extension.
 * @param n Array length
 * @param alpha "A" in SAXPY
 * @param x Device "X" operand array
 * @param y Device "Y" operand array
 */
__global__
void SaxpyKernel(int n, float alpha, float* x, float* y) {
  // TODO: Implement the SAXPY kernel assuming 1-D thread blocks. Make sure to account for mismatch
  // between the total thread count and the size of the input
}

// The number of threads in the block (implemented here with BLOCK_SIZE) is often a
// fixed parameter. Here we specify it with a constant variable, you will also often 
// see preprocessor defines or template parameters.
static const int BLOCK_SIZE=256;

void SaxpyCUDA(int n, float alpha, float x[], float y[]) {
  // Define and allocate memory on device for input and output
  float* d_x;
  float* d_y;
  
  cudaErrorCheck( cudaMalloc(&d_x, n*sizeof(float)) );
  cudaErrorCheck( cudaMalloc(&d_y, n*sizeof(float)) );

  // Time total execution time, including the data transfers between host and device
  double start_time = CycleTimer::currentSeconds();
  
  // TODO: Copy the data from the host to the device with the appropriate cuda runtime call

  double kernel_start_time = CycleTimer::currentSeconds();
  
  // TODO: Implement the kernel launch. You will need to create enough blocks to compute
  // all n values (with blocks of BLOCK_SIZE threads)

  // Kernel launches are asynchronous. To ensure that we can accurately time the kernel execution
  // wait until all kernels are finished executing.
  cudaErrorCheck( cudaDeviceSynchronize() );
  
  double kernel_end_time = CycleTimer::currentSeconds();

  // Copy array of results back to host from GPU
  cudaErrorCheck( cudaMemcpy(y, d_y, n*sizeof(float), cudaMemcpyDeviceToHost) );
  double end_time = CycleTimer::currentSeconds();

  double transfer_time = (end_time-start_time) - (kernel_end_time-kernel_start_time);
  double approx_device_bw = n * 3 * sizeof(float) / (kernel_end_time-kernel_start_time) / 1000 / 1000 / 1000;
  double approx_transfer_bw = n * 3 * sizeof(float) / transfer_time / 1000 / 1000 / 1000;

  printf("[saxpy cuda total]:\t%.3f ms\t%.3f GB/s host-device bandwidth\t%.3f GB/s device memory bandwidth\n", (end_time-start_time) * 1000, approx_transfer_bw, approx_device_bw);
  printf("[saxpy cuda kernel]:\t%.3f ms\n", (kernel_end_time-kernel_start_time) * 1000);
  printf("[saxpy cuda transfer]:\t%.3f ms\n", transfer_time * 1000);

  cudaErrorCheck( cudaFree(d_x) );
  cudaErrorCheck( cudaFree(d_y) );
}

void SaxpyCUDABLAS(int n, float alpha, float x[], float y[]) {
  float* d_x;
  float* d_y;

  cudaMalloc(&d_x, n*sizeof(float));
  cudaMalloc(&d_y, n*sizeof(float));

  // cublas has its own copy functions (in place of cudaMemcpy)
  cublasSetVector(n, sizeof(float), x, 1, d_x, 1);
  cublasSetVector(n, sizeof(float), y, 1, d_y, 1);

  // Actually execute the cublas SAXPY kernel
  cublasHandle_t handle;
  cublasCreate(&handle);
  cublasSaxpy(handle, n, &alpha, d_x, 1, d_y, 1);
  cublasDestroy(handle);

  // Copy array of results back to host from GPU
  cublasGetVector(n, sizeof(float), d_y, 1, y, 1);

  cudaFree(d_x);
  cudaFree(d_y);
}
