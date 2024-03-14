#pragma once
#include <vector_types.h>
#include <string>

/**
 * @brief Store image data along with image size
 */
class Image {
 public:
  int width_;
  int height_;
  float4* data_;  // Vector of RGBA pixels

  Image(int width, int height) : width_(width), height_(height) {
    data_ = new float4[width * height];
    Clear();
  }
  ~Image() { delete[] data_; }

  /// Reset image to all white
  void Clear();

  /// Save image as PPM file
  void Save(const std::string& filename) const;

  /// Return true if two images are identical
  bool Compare(const Image& other) const;
};

class Circles {
 public:
  int n_;
  float2* position_;
  float* radius_;
  float4* color_;

  Circles(int n) : n_(n) {
    position_ = new float2[n];
    radius_ = new float[n];
    color_ = new float4[n];
  }

  ~Circles() {
    delete[] position_;
    delete[] radius_;
    delete[] color_;
  }

  /// Generate a random array of circles
  void GenerateRandomCircles();
};

/**
 * @brief Render image using baseline serial implementation
 *
 * @param image Image to populate with circles
 * @param circles Circles to render
 */
void RenderSerial(Image& image, const Circles& circles);

/**
 * @brief Render image using CUDA implementation
 *
 * @param image Image to populate with circles
 * @param circles Circles to render
 */
void RenderCUDA(Image& image, const Circles& circles);