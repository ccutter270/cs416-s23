#include "bfs.h"
#include <omp.h>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>
#include "CycleTimer.h"


VertexQueue::VertexQueue(VertexQueue&& other) : max_size_(other.max_size_), next_head_(other.next_head_), vertex_queue_(other.vertex_queue_) {
  // "Clear" out other, while leaving it in a valid state
  other.max_size_ = other.next_head_ = 0;
  other.vertex_queue_ = nullptr;
}


void VertexQueue::push_back(const VertexQueue& other) {
  memcpy(vertex_queue_ + next_head_, other.vertex_queue_, other.next_head_ * sizeof(int));
  next_head_ += other.next_head_;
}

VertexBitMap::VertexBitMap(int vertices) : size_(vertices) {
  vertex_bits_ = new word_type[size_];
  memset(vertex_bits_, 0, size_ * sizeof(word_type));
}

VertexBitMap::VertexBitMap(VertexBitMap&& other) : size_(other.size_), vertex_bits_(other.vertex_bits_) {
  // "Clear" out other, while leaving it in a valid state
  other.size_ = 0;
  other.vertex_bits_ = nullptr;  
}


void VertexBitMap::clear() { memset(vertex_bits_, 0, size_ * sizeof(word_type)); }

void swap(VertexQueue& lhs, VertexQueue& rhs) {
  using std::swap;
  assert(lhs.max_size_ == rhs.max_size_);
  swap(lhs.next_head_, rhs.next_head_);
  swap(lhs.vertex_queue_, rhs.vertex_queue_);
}

void swap(VertexBitMap& lhs, VertexBitMap& rhs) {
  using std::swap;
  assert(lhs.size_ == rhs.size_);
  swap(lhs.vertex_bits_, rhs.vertex_bits_);
}

// C++ note: Functions, etc. declared in an anonymous namespace are only accessible in the
// containing file, i.e. this file.
namespace {

/// Perform a single step in BFS top-down algorithm
/// C++ note: Types followed by & are references, e.g. Solution&. References have value syntax, e.g.
/// '.' to call methods, but pointer semantics. That is modifications to solution are reflected in
/// the caller's object (much like passing a pointer). Specifying a const reference, e.g. `const
/// Graph&` prevents the callee from modifying the object (it is effectively read-only).
void BFSTopDownStep(const Graph& graph, Solution& solution, const VertexQueue& current_frontier,
                    VertexQueue& next_frontier) {
  for (int i = 0; i < current_frontier.size(); i++) {
    int current_vertex = current_frontier[i];
    int neighbor_distance = solution[current_vertex] + 1;

    // Example use of range methods and the C++11 range-based for loop to automatically iterate
    // through the range of elements (in this case vertices) defined by the begin and end iterators
    for (int neighbor : graph.outgoing_range(current_vertex)) {
      if (solution[neighbor] == kNOT_VISITED) {
        solution[neighbor] = neighbor_distance;
        next_frontier.push_back(neighbor);
      }
    }
  }
}
}  // namespace

void BFSTopDown(const Graph& graph, Solution& solution, int root) {
  assert(static_cast<int>(solution.size()) == graph.vertices());

  // Initialize current and next frontier to have size |vertices|
  VertexQueue current_frontier(graph.vertices());
  VertexQueue next_frontier(graph.vertices());

  // Initialize current frontier with the root
  current_frontier.push_back(root);
  solution[root] = 0;

  while (current_frontier.size() > 0) {    
#ifdef VERBOSE
    double start_time = CycleTimer::currentSeconds();
#endif

    // Single BFS step
    BFSTopDownStep(graph, solution, current_frontier, next_frontier);

#ifdef VERBOSE
    double end_time = CycleTimer::currentSeconds();
    printf("Top-down frontier=%-10d %.4f sec\n", next_frontier.size(), end_time - start_time);
#endif

    // Swap frontiers for the next step
    swap(current_frontier, next_frontier);
    next_frontier.clear();
  }
}

namespace {

/// Perform a single step in BFS bottom-up algorithm
/// When you assign to a non-const reference, e.g. frontier_edges, you update the variable
/// in the calling context.
void BFSBottomUpStep(const Graph& graph, Solution& solution, const VertexBitMap& current_frontier,
                    VertexBitMap& next_frontier, int& frontier_edges) {
  // Maintain node statistics
  int next_nf = 0;

  for (int v = 0; v < graph.vertices(); v++) {
    if (solution[v] == kNOT_VISITED) {
      for (int neighbor : graph.incoming_range(v)) {
        if (current_frontier[neighbor]) {
          // If any neighbor is in the current frontier, add current vertex to the next frontier
          // and stop examining additional neighbors
          next_frontier.set(v);
          solution[v] = solution[neighbor] + 1;
          next_nf++;
          break;
        }
      }
    }
  }

  frontier_edges = next_nf;
}
}  // namespace

void BFSBottomUp(const Graph& graph, Solution& solution, int root) {
  VertexBitMap current_frontier(graph.vertices());
  VertexBitMap next_frontier(graph.vertices());

  // Initialize frontier with the root
  current_frontier.set(root);
  solution[root] = 0;
  int nf = 1;

  while (current_frontier.any()) {
#ifdef VERBOSE
    double start_time = CycleTimer::currentSeconds();
#endif

    // Single bottom up step
    int next_nf;
    BFSBottomUpStep(graph, solution, current_frontier, next_frontier, next_nf);

#ifdef VERBOSE
    double end_time = CycleTimer::currentSeconds();
    printf("Bottom-up frontier=%-10d %.4f sec\n", nf, end_time - start_time);
#endif

    // Swap frontiers
    swap(current_frontier, next_frontier);
    next_frontier.clear();
    
    nf = next_nf;  // Update statistics for next iteration
  }
}


void ParallelBFSTopDown(const Graph& graph, Solution& solution, int root) {
}


void ParallelBFSBottomUp(const Graph& graph, Solution& solution, int root) {
}

void ParallelBFSHybrid(const Graph& graph, Solution& solution, int root) {
}

bool ValidateSolution(const Graph& graph, const Solution& solution, int root) {
  if (solution[root] != 0) {
    fprintf(stderr, "solution[root] should have a distance of 0\n");
    return false;
  }

  for (int i = 0; i < graph.vertices(); i++) {
    EdgeRange incoming = graph.incoming_range(i);
    if (i == root) continue;

    if (solution[i] == kNOT_VISITED) {
      // All incoming edges must also be not visited
      for (int src : incoming) {
        if (solution[src] != kNOT_VISITED) {
          fprintf(stderr, "Vertex %d was marked not visited, but a predecessor %d was visited\n", i,
                  src);
          return false;
        }
      }
    } else {
      int min_distance = std::numeric_limits<int>::max();
      for (int src : incoming) {
        int incoming_distance = solution[src];
        if (incoming_distance != kNOT_VISITED) {
          min_distance = std::min(min_distance, incoming_distance);
        }
      }
      int neighbor_distance = solution[i] - 1;
      if (min_distance != neighbor_distance) {
        fprintf(stderr,
                "At least one incoming neighbor of %d should have a distance of %d, but minimum is "
                "%d\n",
                i, neighbor_distance, min_distance);
        return false;
      }
    }
  }

  return true;
}
