#include "render.h"
#include "cuda-util.h"


namespace {
  /**
   * @brief Return v clamped to be within the range (min, max)
   * 
   * If v is smaller than min, return min, if it is larger than max, return max, otherwise
   * return v.
   */
  template<typename T> __device__ __host__ inline 
  T clamp(T v, T min, T max) {
    return (v < min) ? min : ((v > max) ? max : v);
  }
  
  /// Return 1 if a circle of radius centered on x,y may overlap a rectangle. All arguments
  /// are in the normalized [0,1] coordinate space.
  ///
  /// This function is sensitive and may return 1 even if the circle does not
  /// overlap the rectangle but if it returns 0, the circle is guaranteed to not 
  /// overlap the rectangle.
  __device__ __inline__ int
  CircleOverlapsBoxFast(float x, float y, float radius, float left, float right, float top, float bottom) {
    // Adapted from CS149 routine
    if (x >= (left - radius) && x <= (right + radius) && 
        y >= (bottom - radius) && y <= (top + radius)) {
      return 1;
    } else {
      return 0;
    }
  }

  /// Return 1 if a circle of radius centered on x,y overlaps a rectangle. All arguments
  /// are in the normalized [0,1] coordinate space.
  ///
  /// This function is precise and will only return 1 if the circle overlaps
  /// the rectangle.
  __device__ __inline__ int
  CircleOverlapsBoxPrecise(float x, float y, float radius, float left, float right, float top, float bottom) {
    // Adapted from CS149 routine
    
    // Find the closest point in the box to the circle
    float closest_x = clamp(x, left, right);
    float closest_y = clamp(y, bottom, top);
    
    // Compute the distance between closest point and circle center. If that distance is less than 
    // radius then return 1.
    float delta_x = closest_x - x;
    float delta_y = closest_y - y;
    if ((delta_x * delta_x + delta_y * delta_y) <= (radius * radius)) {
      return 1;
    } else {
      return 0;
    }
  }
}


__global__ void RenderCUDAKernel(int n_circles, float2* circles_position, float* circles_radius, float4* circles_color, int image_width, int image_height, float4* image_data) {
  // Determine the circle to be computed by this thread
  int c = blockIdx.x * blockDim.x + threadIdx.x;

  // Compute the width and height of each pixel in normalized [0,1] coordinates
  float x_width = 1.f / image_width;
  float y_width = 1.f / image_height;

  float2 center = circles_position[c];
  float radius = circles_radius[c];
  float4 color = circles_color[c];
  
  // Compute the bounding box for the circle
  float min_x = center.x - radius;
  float max_x = center.x + radius;
  float min_y = center.y - radius;
  float max_y = center.y + radius;

  // Determine the pixels within the bounding box by translating normalized coordinate space
  // to pixel indexes
  int beg_x = clamp(static_cast<int>(min_x * image_width), 0, image_width);
  int end_x = clamp(static_cast<int>(max_x * image_width) + 1, 0, image_width);
  int beg_y = clamp(static_cast<int>(min_y * image_height), 0, image_height);
  int end_y = clamp(static_cast<int>(max_y * image_height) + 1, 0, image_height);

  // Iterate through all of the pixels in the bounding box
  for (int y=beg_y; y<end_y; y++) {
    for (int x=beg_x; x<end_x; x++) {
      // Determine pixel center (in the "middle" of the pixel) in normalized [0,1] coordinate space
      float pix_x = x_width * (static_cast<float>(x) + 0.5f);
      float pix_y = y_width * (static_cast<float>(y) + 0.5f);

      // Check that the pixel center is actually within the circle
      float delta_x = center.x - pix_x;
      float delta_y = center.y - pix_y;
      if ((delta_x * delta_x + delta_y * delta_y) > (radius * radius))
        continue; // Pixel is outside the circle

      // Obtain the actual pixel color data assuming row-major storage. Use a pointer
      // so that we can modify the underlying data. 
      float4* pixel_color = image_data + y*image_width + x;
      float alpha = color.w;

      // Update pixel colors with alpha blending. This must be an atomic
      // read-modify-write operation, and must be performed for each circle in 
      // correct order to produce the correct colors.
      pixel_color->x = alpha * color.x + (1 - alpha) * pixel_color->x;
      pixel_color->y = alpha * color.y + (1 - alpha) * pixel_color->y;
      pixel_color->z = alpha * color.z + (1 - alpha) * pixel_color->z;
      pixel_color->w = alpha + (1 - alpha) * pixel_color->w;
    }  
  }

}

// Do not modify this interface. All other modifications, such as to the interface of RenderCUDAKernel, are permitted.
void RenderCUDA(Image& image, const Circles& circles) {
  // Allocate device data
  float2* d_circles_position;
  float* d_circles_radius;
  float4* d_circles_color;
  float4* d_image_data;

  int n_circles = circles.n_;
  cudaErrorCheck( cudaMalloc(&d_circles_position, n_circles*sizeof(float2)) );
  cudaErrorCheck( cudaMalloc(&d_circles_radius, n_circles*sizeof(float)) );
  cudaErrorCheck( cudaMalloc(&d_circles_color, n_circles*sizeof(float4)) );
  cudaErrorCheck( cudaMalloc(&d_image_data, image.width_*image.height_*sizeof(float4)) );
  
  // Copy circle and image data to the device
  cudaErrorCheck( cudaMemcpy(d_circles_position, circles.position_, n_circles*sizeof(float2), cudaMemcpyHostToDevice) );
  cudaErrorCheck( cudaMemcpy(d_circles_radius, circles.radius_, n_circles*sizeof(float), cudaMemcpyHostToDevice) );
  cudaErrorCheck( cudaMemcpy(d_circles_color, circles.color_, n_circles*sizeof(float4), cudaMemcpyHostToDevice) );
  cudaErrorCheck( cudaMemcpy(d_image_data, image.data_, image.width_*image.height_*sizeof(float4), cudaMemcpyHostToDevice) );


  // Launch kernel with one block per circle, with one thread per-block
  RenderCUDAKernel<<<circles.n_, 1>>>(n_circles, d_circles_position, d_circles_radius, d_circles_color, image.width_, image.height_, d_image_data);

  // Copy rendered image back to host
  cudaErrorCheck( cudaMemcpy(image.data_, d_image_data, image.width_*image.height_*sizeof(float4), cudaMemcpyDeviceToHost) );

  // Cleanup device data
  cudaErrorCheck( cudaFree(d_circles_position) );
  cudaErrorCheck( cudaFree(d_circles_radius) );
  cudaErrorCheck( cudaFree(d_circles_color) );
  cudaErrorCheck( cudaFree(d_image_data) );
}
