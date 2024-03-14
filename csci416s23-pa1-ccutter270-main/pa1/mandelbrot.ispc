/*
  Copyright (c) 2010-2012, Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.


   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
   TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

static inline int mandel(float c_re, float c_im, int count) {
  float z_re = c_re, z_im = c_im;
  int i;
  for (i = 0; i < count; ++i) {
    if (z_re * z_re + z_im * z_im > 4.f) {
        break;
    }

    unmasked {
      float new_re = z_re*z_re - z_im*z_im;
      float new_im = 2.f * z_re * z_im;
      z_re = c_re + new_re;
      z_im = c_im + new_im;
    }
  }

  return i;
}

/**
 * @brief Compute iterations needed to determine if each pixel is in the Mandelbrot set using
 * the ISPC SPMD-on-SIMD model
 *
 * @param x0 Mandelbrot set parameters
 * @param y0 Mandelbrot set parameters
 * @param x1 Mandelbrot set parameters
 * @param y1 Mandelbrot set parameters
 * @param width Image width
 * @param height Image height
 * @param maxIterations Max iterations to allow
 * @param output Iteration count result in row-major order
 */
export void MandelbrotISPC(uniform float x0, uniform float y0,
                           uniform float x1, uniform float y1,
                           uniform int width, uniform int height,
                           uniform int maxIterations,
                           uniform int output[]) {
  uniform float dx = (x1 - x0) / width;
  uniform float dy = (y1 - y0) / height;

  for (uniform int j = 0; j < height; j++) {
    foreach (i = 0 ... width) {
      // Figure out the position on the complex plane. Note that the x 
      // values are different across different program instances, since its
      // initializer incorporates the value of the foreach loop.
      float x = x0 + i * dx;
      float y = y0 + j * dy;

      int index = j * width + i;
      output[index] = mandel(x, y, maxIterations);
    }
  }
}

// Alternate task implementation that computes a `span` of rows in the image as determined
// by the taskIndex, a special variable set by ISPC to his task's unique zero-index ID
task void MandelbrotISPCTask(uniform float x0, uniform float dx,
                             uniform float y0, uniform float dy,
                             uniform int width, uniform int height,
                             uniform int maxIterations, uniform int output[],
                             uniform int span) {

  uniform int row_start = taskIndex * span;
  uniform int row_end = min(row_start+span, height);

  for (uniform int j = row_start; j < row_end; j++) {
    foreach(i=0 ... width) {
      float x = x0 + i * dx;
      float y = y0 + j * dy;

      int index = j * width + i;
      output[index] = mandel(x, y, maxIterations);
    }
  }
}

/**
 * @brief Compute iterations needed to determine if each pixel is in the Mandelbrot set using
 * the both ISPC SPMD-on-SIMD model and tasks
 *
 * @param x0 Mandelbrot set parameters
 * @param y0 Mandelbrot set parameters
 * @param x1 Mandelbrot set parameters
 * @param y1 Mandelbrot set parameters
 * @param width Image width
 * @param height Image height
 * @param maxIterations Max iterations to allow
 * @param output Iteration count result in row-major order
 * @param num_tasks Number of tasks to use for computation
 */
export void MandelbrotISPCTasks(uniform float x0, uniform float y0,
                                uniform float x1, uniform float y1,
                                uniform int width, uniform int height,
                                uniform int maxIterations, uniform int output[],
                                uniform int num_tasks) {
  
  uniform float dx = (x1 - x0) / width;
  uniform float dy = (y1 - y0) / height;
  
  // TODO: Update this implementation to launch num_tasks instead of just 1

  // Creates upper limit of rows  
  uniform int rows_per_task = (height + num_tasks - 1) / num_tasks; 

  // Create launches
  launch[num_tasks] MandelbrotISPCTask(x0, dx, y0, dy, width, height, maxIterations, output, rows_per_task);


}
