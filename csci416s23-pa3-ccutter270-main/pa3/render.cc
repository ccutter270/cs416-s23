#include "render.h"
#include <algorithm>
#include <cstdio>
#include <random>

// Anonymous namespaces restrict visibility to this compilation unit (.cc file), like the
// static keyword
namespace {
constexpr unsigned char ppm_clamp(float v) {
  return (v < 0.f) ? 0 : ((v > 1.f) ? 255 : static_cast<char>(255.f * v));
}
}  // anonymous namespace

void Image::Clear() {
  float4 white = {1.f, 1.f, 1.f, 1.f};
  std::fill(data_, data_ + width_ * height_, white);
}

void Image::Save(const std::string& filename) const {
  FILE* fp = fopen(filename.c_str(), "wb");
  fprintf(fp, "P6\n");
  fprintf(fp, "%d %d\n", width_, height_);
  fprintf(fp, "255\n");
  for (int i = 0; i < width_ * height_; ++i) {
    const auto& pixel = data_[i];
    fputc(ppm_clamp(pixel.x), fp);  // r component
    fputc(ppm_clamp(pixel.y), fp);  // g component
    fputc(ppm_clamp(pixel.z), fp);  // b component
  }
  fclose(fp);
  fprintf(stderr, "Wrote image file %s\n", filename.c_str());
}

bool Image::Compare(const Image& other) const {
  for (int i = 0; i < width_ * height_; ++i) {
    const auto& expected = data_[i];
    const auto& actual = other.data_[i];
    // Compare the resulting image files by using the same clamping functionality
    if (ppm_clamp(expected.x) != ppm_clamp(actual.x) ||
        ppm_clamp(expected.y) != ppm_clamp(actual.y) ||
        ppm_clamp(expected.z) != ppm_clamp(actual.z)) {
      fprintf(stderr, "Error at (%d,%d): Expected RGB(%u, %u, %u), Actual RGB(%u %u, %u)\n",
              i % width_, i / width_, ppm_clamp(expected.x), ppm_clamp(expected.y),
              ppm_clamp(expected.z), ppm_clamp(actual.x), ppm_clamp(actual.y), ppm_clamp(actual.z));
      return false;
    }
  }
  return true;
}

void Circles::GenerateRandomCircles() {
  std::default_random_engine rand_engine;

  // These coordinates are adapted from CS149 assignment
  auto rad_dist = std::uniform_real_distribution<float>(0.02, 0.08);
  auto pos_dist = std::uniform_real_distribution<float>(0, 1);

  auto red_dist = std::uniform_real_distribution<float>(.1f, 1.f);
  auto green_dist = std::uniform_real_distribution<float>(.2f, .7f);
  auto blue_dist = std::uniform_real_distribution<float>(.5f, .5f);

  for (int i = 0; i < n_; i++) {
    radius_[i] = rad_dist(rand_engine);
    position_[i] = {pos_dist(rand_engine), pos_dist(rand_engine)};
    color_[i] = {red_dist(rand_engine), green_dist(rand_engine), blue_dist(rand_engine), 0.5f};
  }
}

namespace {
// By making this function templated, it can be used with any comparable type
// such as int or float

/**
 * @brief Return v clamped to be within the range (min, max)
 * 
 * If v is smaller than min, return min, if it is larger than max, return max, otherwise
 * return v.
 */
template <typename T>
constexpr T clamp(T v, T min, T max) {
  return (v < min) ? min : ((v > max) ? max : v);
}
}  // namespace

void RenderSerial(Image& image, const Circles& circles) {
  // Compute the width and height of each pixel in normalized [0,1] coordinates
  float x_width = 1.f / image.width_;
  float y_width = 1.f / image.height_;

  // Iterate through each circle in order
  for (int c = 0; c < circles.n_; c++) {
    float2 center = circles.position_[c];
    float radius = circles.radius_[c];
    float4 color = circles.color_[c];

    // Compute the bounding box for the circle in normalized [0,1] coordinates
    float min_x = center.x - radius;
    float max_x = center.x + radius;
    float min_y = center.y - radius;
    float max_y = center.y + radius;

    // Determine the pixels within the bounding box by translating normalized coordinate space
    // to pixel indexes
    int beg_x = clamp(static_cast<int>(min_x * image.width_), 0, image.width_);
    int end_x = clamp(static_cast<int>(max_x * image.width_) + 1, 0, image.width_);
    int beg_y = clamp(static_cast<int>(min_y * image.height_), 0, image.height_);
    int end_y = clamp(static_cast<int>(max_y * image.height_) + 1, 0, image.height_);

    // Iterate through all of the pixels in the bounding box
    for (int y = beg_y; y < end_y; y++) {
      for (int x = beg_x; x < end_x; x++) {
        // Determine the pixel center (in the "middle" of the pixel) in normalized [0,1] coordinate
        // space
        float pix_x = x_width * (static_cast<float>(x) + 0.5f);
        float pix_y = y_width * (static_cast<float>(y) + 0.5f);

        // Check that the pixel center is actually within the circle
        float delta_x = center.x - pix_x;
        float delta_y = center.y - pix_y;
        if ((delta_x * delta_x + delta_y * delta_y) > (radius * radius))
          continue;  // Pixel is outside the circle

        // Obtain the actual pixel color data assuming row-major storage. Use a pointer
        // so that we can modify the underlying data.
        float4* pixel_color = image.data_ + y * image.width_ + x;
        float alpha = color.w;

        // Update pixel colors with alpha blending. This must be an atomic
        // read-modify-write operation, and must be performed for each circle in the
        // correct order to produce the correct colors.
        pixel_color->x = alpha * color.x + (1 - alpha) * pixel_color->x;
        pixel_color->y = alpha * color.y + (1 - alpha) * pixel_color->y;
        pixel_color->z = alpha * color.z + (1 - alpha) * pixel_color->z;
        pixel_color->w = alpha + (1 - alpha) * pixel_color->w;
      }
    }
  }
}