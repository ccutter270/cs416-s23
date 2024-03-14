#include <algorithm>
#include <getopt.h>
#include <cstdio>
#include <cstdlib>
#include <omp.h>
#include "CycleTimer.h"
#include "graph.h"
#include "bfs.h"

const int kRuns = 3;
int gThreads = 1;

const int kRoot = 0;

// Specify expected options and usage
const char* kShortOptions = "t:h";
const struct option kLongOptions[] = {{"threads", required_argument, nullptr, 't'},
                                      {"help", no_argument, nullptr, 'h'},
                                      {nullptr, 0, nullptr, 0}};

void PrintUsage(const char* program_name) {
  printf("Usage: %s [options] <GRAPH FILE>\n", program_name);
  printf("Options:\n");
  printf(
      "  -t  --threads <INT>  Run OpenMP implementation with specified threads, default: %d\n",
      gThreads);
  printf("  -h  --help           Print this message\n");
}

template <class Fn, class... Args>
double BFSBenchmark(int num_runs, Fn&& fn, const Graph& graph, Solution& solution, Args&&... args) {
  double min_time = std::numeric_limits<double>::max();
  for (int i = 0; i < num_runs; i++) {
    solution.assign(graph.vertices(), kNOT_VISITED);
    double start_time = CycleTimer::currentSeconds();
    fn(graph, solution, std::forward<Args>(args)...);
    double end_time = CycleTimer::currentSeconds();
    min_time = std::min(min_time, end_time - start_time);
  }
  return min_time;
}

bool CompareBFSResults(const Solution& solution_ref, const Solution& solution) {
  return std::equal(solution_ref.begin(), solution_ref.end(), solution.begin());
}


int main(int argc, char** argv) {
  {
    int opt;
    while ((opt = getopt_long(argc, argv, kShortOptions, kLongOptions, nullptr)) != -1) {
      switch (opt) {
        case 't':
          gThreads = std::min(omp_get_max_threads(), atoi(optarg));
          break;
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
  
  omp_set_num_threads(gThreads);

  if (optind == argc) {
    fprintf(stderr, "Error: Missing positional arguments\n");
    PrintUsage(argv[0]);
    return 1;
  }

  fprintf(stderr, "Performing BFS on graph from %s\n", argv[optind]);
  Graph graph(argv[optind]);
  Solution reference_solution, solution;

  double min_serial_top = BFSBenchmark(kRuns, BFSTopDown, graph, reference_solution, kRoot);
  printf("[topdown serial]:\t%.3f ms\t%.3fX speedup\n", min_serial_top * 1000, 1.f);
  if (!ValidateSolution(graph, reference_solution, kRoot)) {
    fprintf(stderr, "Top down solution distance is inconsistent with graph\n");
    return 1;
  }

  double min_parallel_top = BFSBenchmark(kRuns, ParallelBFSTopDown, graph, solution, kRoot);
  printf("[topdown %d threads]:\t%.3f ms\t%.3fX speedup (vs. serial top-down)\n", gThreads, min_parallel_top * 1000, min_serial_top / min_parallel_top);
  if (!ValidateSolution(graph, solution, 0)) {
    fprintf(stderr, "Parallel top down solution distance is inconsistent with graph\n");
    return 1;
  }
  if (!CompareBFSResults(reference_solution, solution)) {
    fprintf(stderr, "Parallel top down BFS doesn't match reference implementation\n");
    return 1;
  }

  double min_serial_bot = BFSBenchmark(kRuns, BFSBottomUp, graph, solution, kRoot);
  printf("[bottomup serial]:\t%.3f ms\t%.3fX speedup\n", min_serial_bot * 1000, 1.f);
  if (!ValidateSolution(graph, solution, 0)) {
    fprintf(stderr, "Bottom up Solution distance is inconsistent with graph\n");
    return 1;
  }
  if (!CompareBFSResults(reference_solution, solution)) {
    fprintf(stderr, "Bottom up BFS doesn't match reference implementation\n");
    return 1;
  }

  double min_parallel_bot = BFSBenchmark(kRuns, ParallelBFSBottomUp, graph, solution, kRoot);
  printf("[bottomup %d threads]:\t%.3f ms\t%.3fX speedup (vs. serial bottom-up)\n", gThreads, min_parallel_bot * 1000, min_serial_bot / min_parallel_bot);
  if (!ValidateSolution(graph, solution, 0)) {
    fprintf(stderr, "Parallel bottom up Solution distance is inconsistent with graph\n");
    return 1;
  }
  if (!CompareBFSResults(reference_solution, solution)) {
    fprintf(stderr, "Parallel bottom up BFS doesn't match reference implementation\n");
    return 1;
  }

  double min_parallel_hybrid = BFSBenchmark(kRuns, ParallelBFSHybrid, graph, solution, kRoot);
  printf("[hybrid %d threads]:\t%.3f ms\t%.3fX speedup (vs. serial top-down)\n", gThreads, min_parallel_hybrid * 1000, min_serial_top / min_parallel_hybrid);
  if (!ValidateSolution(graph, solution, 0)) {
    fprintf(stderr, "Parallel hybrid solution distance is inconsistent with graph\n");
    return 1;
  }
  if (!CompareBFSResults(reference_solution, solution)) {
    fprintf(stderr, "Parallel hybrid BFS doesn't match reference implementation\n");
    return 1;
  }

  return 0;
}