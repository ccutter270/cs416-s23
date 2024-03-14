#include <cstdio>
#include <cuda.h>
#include <cuda_runtime.h>
#include <cub/block/block_reduce.cuh>
#include <cub/block/block_scan.cuh>
#include <cub/device/device_scan.cuh>
#include <cub/device/device_select.cuh>
#include <cub/iterator/counting_input_iterator.cuh>
#include <cub/iterator/transform_input_iterator.cuh>

#include "cuda-util.h"

#define SCAN_BLOCK_SIZE 512

__global__ void EvensCount(int n, int* values, int* block_counts) {
  // Specialize int BlockReduce for a 1D block of SCAN_BLOCK_SIZE threads
  typedef cub::BlockReduce<int, SCAN_BLOCK_SIZE> BlockReduce;

  // Allocate shared memory for BlockReduce
  __shared__ typename BlockReduce::TempStorage reduce_temp;

  // Load value for this thread
  int thread_idx = blockIdx.x * SCAN_BLOCK_SIZE + threadIdx.x;

  int thread_value = 1; // Make sure values past input don't contribute to the count
  if (thread_idx < n) {
    thread_value = values[thread_idx];
  }

  // Assign to be 1 if value is even
  int thread_is_even = static_cast<int>((thread_value % 2) == 0); 

  // Collectively compute the block-wide reduction to determine number of evens
  int total_evens = BlockReduce(reduce_temp).Sum(thread_is_even);

  if (threadIdx.x == 0)
    block_counts[blockIdx.x] = total_evens;
}

__global__ void EvensScatter(int n, int* values, int* block_boundaries, int* indices) {
  // Specialize int BlockScan for a 1D block of SCAN_BLOCK_SIZE threads
  typedef cub::BlockScan<int, SCAN_BLOCK_SIZE> BlockScan;

  // Allocate shared memory for BlockScan
  __shared__ typename BlockScan::TempStorage scan_temp;

  // Load value for this thread
  int thread_idx = blockIdx.x * SCAN_BLOCK_SIZE + threadIdx.x;
  
  int thread_value = 1; // Make sure values past input don't contribute to the count
  if (thread_idx < n) {
    thread_value = values[thread_idx];
  } 

  // Assign to be 1 if value is even
  int thread_is_even = static_cast<int>((thread_value % 2) == 0);

  // Since each thread has its own copy of local variables, each variable is
  // like an int[THREAD_COUNT] array. CUB uses local variables for input and output.
  int thread_scatter_index;

  // Collectively compute the block-wide exclusive prefix sum
  BlockScan(scan_temp).ExclusiveSum(thread_is_even, thread_scatter_index);

  // Scatter indices based on block-wise exclusive prefix sum
  int block_start = block_boundaries[blockIdx.x];
  if (thread_idx < n && thread_is_even) {
    indices[block_start + thread_scatter_index] = thread_idx;
  }
}

void EvensBlockCUB(int n, int values[], int indices[], int* indices_count) {
  int* d_values;
  int* d_indices;
  int* d_block_counts;
  int* d_block_boundaries;

  int blocks = (n + SCAN_BLOCK_SIZE - 1) / SCAN_BLOCK_SIZE;

  // Allocate device memory
  cudaErrorCheck(cudaMalloc(&d_values, n * sizeof(int)));
  cudaErrorCheck(cudaMalloc(&d_indices, n * sizeof(int)));
  cudaErrorCheck(cudaMalloc(&d_block_counts, blocks * sizeof(int)));
  cudaErrorCheck(cudaMalloc(&d_block_boundaries, (blocks + 1) * sizeof(int)));
  cudaErrorCheck(cudaMemset(d_block_boundaries, 0, (blocks + 1) * sizeof(int)));
  
  // Transfer values to device
  cudaErrorCheck(cudaMemcpy(d_values, values, n * sizeof(int), cudaMemcpyHostToDevice));

  // 1. Compute counts of evens in each block
  EvensCount<<<blocks, SCAN_BLOCK_SIZE>>>(n, d_values, d_block_counts);

  // 2. Perform global prefix sum to determine global block boundaries

  // Determine temporary device storage requirements for scan
  void *d_temp_storage = NULL;
  size_t temp_storage_bytes = 0;
  cub::DeviceScan::InclusiveSum(d_temp_storage, temp_storage_bytes, d_block_counts, d_block_boundaries + 1, blocks);

  // Allocate temporary storage, run device prefix sum, then free temporary storage
  cudaErrorCheck(cudaMalloc(&d_temp_storage, temp_storage_bytes));
  cub::DeviceScan::InclusiveSum(d_temp_storage, temp_storage_bytes, d_block_counts, d_block_boundaries + 1, blocks);
  cudaErrorCheck(cudaFree(d_temp_storage));

  // 3. Perform block-wise scan and scatter to write out indices
  EvensScatter<<<blocks, SCAN_BLOCK_SIZE>>>(n, d_values, d_block_boundaries, d_indices);

  // Transfer indices back from device
  cudaErrorCheck(cudaMemcpy(indices, d_indices, n*sizeof(int), cudaMemcpyDeviceToHost));
  cudaErrorCheck(cudaMemcpy(indices_count, d_block_boundaries+blocks, sizeof(int), cudaMemcpyDeviceToHost));

  // Free device memory
  cudaErrorCheck(cudaFree(d_values));
  cudaErrorCheck(cudaFree(d_indices));
  cudaErrorCheck(cudaFree(d_block_counts));
  cudaErrorCheck(cudaFree(d_block_boundaries));
}

/**
 * @brief Functor for determining if a number is even
 *
 * A functor (function object) is a object that overloads the function call operator
 * so that it can be called like a function
 */
struct IsEven {
    __host__ __device__ __forceinline__
    bool operator()(const int& value) const {
        return (value % 2) == 0;
    }
};

void EvensDeviceCUB(int n, int values[], int indices[], int* indices_count) {
  int* d_values;
  int* d_indices;
  int* d_num_evens;

  cudaErrorCheck(cudaMalloc(&d_values, n * sizeof(int)));
  cudaErrorCheck(cudaMalloc(&d_indices, n * sizeof(int)));
  cudaErrorCheck(cudaMalloc(&d_num_evens, sizeof(int)));

  cudaErrorCheck(cudaMemcpy(d_values, values, n * sizeof(int), cudaMemcpyHostToDevice));

  // Create an iterator wrapper for indices
  cub::CountingInputIterator<int> index_itr(0);
  
  // Create an iterator wrapper for computing "is even" from d_values
  IsEven is_even_op;
  cub::TransformInputIterator<bool, IsEven, int*> is_even_itr(d_values, is_even_op);

  // Determine temporary device storage requirements
  void *d_temp_storage = NULL;
  size_t temp_storage_bytes = 0;
  cub::DeviceSelect::Flagged(d_temp_storage, temp_storage_bytes, index_itr, is_even_itr, d_indices, d_num_evens, n);
  
  // Allocate temporary storage, run selection, free temporary storage
  cudaErrorCheck(cudaMalloc(&d_temp_storage, temp_storage_bytes));
  cub::DeviceSelect::Flagged(d_temp_storage, temp_storage_bytes, index_itr, is_even_itr, d_indices, d_num_evens, n);
  cudaErrorCheck(cudaFree(d_temp_storage));

  // Transfer indices back from device
  cudaErrorCheck(cudaMemcpy(indices, d_indices, n*sizeof(int), cudaMemcpyDeviceToHost));
  cudaErrorCheck(cudaMemcpy(indices_count, d_num_evens, sizeof(int), cudaMemcpyDeviceToHost));

  cudaErrorCheck(cudaFree(d_values));
  cudaErrorCheck(cudaFree(d_indices));
  cudaErrorCheck(cudaFree(d_num_evens));
}