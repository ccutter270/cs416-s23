#include <getopt.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <string>
#include "CycleTimer.h"

#include "render.h"

const int kRuns = 3;
int gImageSize = 1024;

// Specify expected options and usage
const char* kShortOptions = "s:n:lh";
const struct option kLongOptions[] = {{"size", required_argument, nullptr, 's'},
                                      {"name", required_argument, nullptr, 'n'},
                                      {"help", no_argument, nullptr, 'h'},
                                      {nullptr, 0, nullptr, 0}};

void PrintUsage(const char* program_name) {
  printf("Usage: %s [options]\n", program_name);
  printf("Options:\n");
  printf("  -s --size <INT>      Produce <INT>x<INT> sized image\n");
  printf("  -n --name <NAME>     Run the test with <NAME>\n");
  printf("  -l --list            List the available tests and exit\n");
  printf("  -h  --help           Print this message\n");
}

enum class CirclesKind : int {
  kRGB = 0,
  kRGB10k,
  kRGB100k,
  kMaxKind,
};

Circles* CirclesFactory(CirclesKind kind) {
  switch (kind) {
    case CirclesKind::kRGB: {
      Circles* circles = new Circles(3);

      circles->position_[0] = {.4f, .75f};
      circles->position_[1] = {.5f, .5f};
      circles->position_[2] = {.6f, .25f};

      circles->radius_[0] = .3f;
      circles->radius_[1] = .3f;
      circles->radius_[2] = .3f;

      circles->color_[0] = {1.f, 0.f, 0.f, .5f};
      circles->color_[1] = {0.f, 1.f, 0.f, .5f};
      circles->color_[2] = {0.f, 0.f, 1.f, .5f};

      return circles;
    }
    case CirclesKind::kRGB10k: {
      Circles* circles = new Circles(10000);
      circles->GenerateRandomCircles();
      return circles;
    }
    case CirclesKind::kRGB100k: {
      Circles* circles = new Circles(100000);
      circles->GenerateRandomCircles();
      return circles;
    }
    default:
      return nullptr;
  }
}

const char* CirclesName(CirclesKind kind) {
  switch (kind) {
    case CirclesKind::kRGB:
      return "RGB";
    case CirclesKind::kRGB10k:
      return "RGBRandom10k";
    case CirclesKind::kRGB100k:
      return "RGBRandom100k";
    default:
      return nullptr;
  }
}

/**
 * @brief Run function num_runs times, reporting minimum time
 *
 * @param num_runs Number of benchmark runs
 * @param fn Function to execute
 * @param args Arguments to be forwarded to fn
 * @return double Minimum time measured
 */
template <class Fn, class... Args>
double RenderBenchmark(int num_runs, Fn&& fn, Image& image, Args&&... args) {
  double min_time = std::numeric_limits<double>::max();
  for (int i = 0; i < num_runs; ++i) {
    image.Clear();
    double start_time = CycleTimer::currentSeconds();
    fn(std::forward<Args>(args)...);
    double end_time = CycleTimer::currentSeconds();
    min_time = std::min(min_time, end_time - start_time);
  }
  return min_time;
}

int main(int argc, char** argv) {
  std::string test_name;
  {
    int opt;
    while ((opt = getopt_long(argc, argv, kShortOptions, kLongOptions, nullptr)) != -1) {
      switch (opt) {
        case 's':
          gImageSize = atoi(optarg);
          break;
        case 'n':
          test_name = optarg;
          break;
        case 'l':
          for (int i = 0; i < static_cast<int>(CirclesKind::kMaxKind); i++) {
            printf("%s\n", CirclesName(static_cast<CirclesKind>(i)));
          }
          return 0;
        case 'h':
          PrintUsage(argv[0]);
          return 0;
        case '?':  // Unrecognized option
        default:
          PrintUsage(argv[0]);
          return 1;
      }
    }
  }

  for (int i = 0; i < static_cast<int>(CirclesKind::kMaxKind); i++) {
    std::string current_name(CirclesName(static_cast<CirclesKind>(i)));
    if (!test_name.empty() && test_name != current_name) {
      continue;
    }
    printf("Test: %s\n", current_name.c_str());
    Circles* circles = CirclesFactory(static_cast<CirclesKind>(i));

    Image serial_image(gImageSize, gImageSize);

    double min_serial = RenderBenchmark(kRuns, RenderSerial, serial_image, serial_image, *circles);
    printf("[render serial]:\t%.3f ms\t%.3fX speedup\n", min_serial * 1000, 1.f);
    serial_image.Save(current_name + "-serial.ppm");

    Image cuda_image(gImageSize, gImageSize);

    double min_cuda = RenderBenchmark(kRuns, RenderCUDA, cuda_image, cuda_image, *circles);
    printf("[render cuda]:\t%.3f ms\t%.3fX speedup\n", min_cuda * 1000, min_serial / min_cuda);
    cuda_image.Save(current_name + "-cuda.ppm");
    if (!serial_image.Compare(cuda_image)) {
      fprintf(stderr, "CUDA image does not match serial image\n");
      return 1;
    }

    delete circles;
  }

  return 0;
}