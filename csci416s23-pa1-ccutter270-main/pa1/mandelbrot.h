#pragma once

/**
 * @brief Write image to file in PPM format
 *
 * @param buf Iteration count data in row-major order
 * @param width Image width
 * @param height Image height
 * @param filename Filename to write image file
 */
void WritePPM(int *buf, int width, int height, const char *filename);

/**
 * @brief Compute iterations needed to determine if each pixel is in the Mandelbrot set
 *
 * @param x0 Mandelbrot set parameters
 * @param y0 Mandelbrot set parameters
 * @param x1 Mandelbrot set parameters
 * @param y1 Mandelbrot set parameters
 * @param width Image width
 * @param height Image height
 * @param maxIterations Max iterations to allow
 * @param output Iteration count result in row-major order
 * @param start_row Compute pixels starting with this row in the image (inclusive)
 * @param end_row Compute pixels up to this row in the image (exclusive)
 * @param start_col Compute pixels starting with this column in the image (inclusive)
 * @param end_col Compute pixels up to this column in image(exclusive)
 */
void MandelbrotSerial(float x0, float y0, float x1, float y1, int width, int height,
                      int maxIterations, int output[], int start_row, int end_row, int start_col,
                      int end_col);

/**
 * @brief Compute iterations needed to determine if pixel is in the Mandelbrot set using multiple
 * threads
 *
 * @param x0 Mandelbrot set parameters
 * @param y0 Mandelbrot set parameters
 * @param x1 Mandelbrot set parameters
 * @param y1 Mandelbrot set parameters
 * @param width Image width
 * @param height Image height
 * @param maxIterations Max iterations to allow
 * @param output Iteration count result in row-major order
 * @param num_threads Number of threads to use (include master thread)
 */
void MandelbrotThreads(float x0, float y0, float x1, float y1, int width, int height,
                       int maxIterations, int output[], int num_threads);