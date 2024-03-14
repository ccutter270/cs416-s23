#include "mandelbrot.h"
#include <algorithm>
#include <thread>
#include "CycleTimer.h"
#include <stdio.h>

namespace {


/**
 * @brief Wrapper function for the work performed by a single thread.
 *
 * You are encouraged to modify this function however you need to improve Mandelbrot performance.
 */
void MandelbrotThread(float x0, float y0, float x1, float y1, int width, int height,
                      int maxIterations, int output[], int start_row, int end_row, int start_col,
                      int end_col, int threadID, int totalThreads) {

                        // threat id and total threads

  // double startTime = CycleTimer::currentSeconds();

  // Split up image distribution as 123 123 123 instead of 111 222 333
  for (int i = threadID; i < height; i += totalThreads) {

  // Don't really need start/end row/col
  MandelbrotSerial(x0, y0, x1, y1, width, height, maxIterations, output, i, i + 1,
                   0, width);
  }

    // MandelbrotSerial(x0, y0, x1, y1, width, height, maxIterations, output, start_row, end_row,
    //                start_col, end_col);

  
  // TIMING CODE 
  
  // double endTime = CycleTimer::currentSeconds();

  // double time = endTime - startTime;

  // printf("Start Row: %d\n", start_row);
  // printf("TIME: %f\n", time);
}


}  // namespace



void MandelbrotThreads(float x0, float y0, float x1, float y1, int width, int height,
                       int maxIterations, int output[], int num_threads) {

  // TODO: Implement the computation using C++ threads with num_threads. Be mindful of edge conditions
  
  // Initiate workers threads
  std::thread workers[num_threads];

  // Calculate rows per thread & current 
  int rows_per_thread = height / num_threads;     // this stays constant 
  int curr_height = 0;                            // this increments each iteration

  
  // Iterate to create num_threads - 1 (master thread after) (bottom --> up )
  for (int i = 0; i < num_threads - 1; i++) {

        // Create threads

        // i is thread id, and num thread
        workers[i] = std::thread(MandelbrotThread, x0, y0, x1, y1, width, height, maxIterations, output,
                           curr_height, (curr_height + rows_per_thread), 0, width, i, num_threads);

        // Increment curr_height
        curr_height = curr_height + rows_per_thread;
    }

  // Main thread is top part (curr_height --> height)
    MandelbrotThread(x0, y0, x1, y1, width, height, maxIterations, output, curr_height, height, 0,
                    width, num_threads - 1, num_threads);

  // Join the worker threads 
  for (int i = 0; i < num_threads - 1; i++) {
    workers[i].join();
  }

// PROF LINDERMAN STARRT CODE W/ 2 THREADS

  //   // Split the image into 2 halves
//   int rows_per_thread = height / 2;

//   // Create a thread that launches the MandelbrotThread function with these arguments (the top half
//   // of the image)
//   workers[1] = std::thread(MandelbrotThread, x0, y0, x1, y1, width, height, maxIterations, output,
//                            rows_per_thread, height, 0, width);

//   // Perform one work chunk on the "master" thread so it doesn't sit idle
//   MandelbrotThread(x0, y0, x1, y1, width, height, maxIterations, output, 0, rows_per_thread, 0,
//                    width);

//   // Wait until all the work is complete
//   workers[1].join();

}