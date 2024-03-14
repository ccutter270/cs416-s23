#pragma once

#include <cstdint>
#include <vector>
#include "graph.h"

/// A distance for vertices that have not been visited
const int kNOT_VISITED = -1;

/// Convenience typedef for the solution type
typedef std::vector<int> Solution;

///@{
/*
 * The different BFS implementations. Each takes the same arguments:
 * @param graph Input graph
 * @param solution Vector of distances for each vertex. This vector must be at least vertices long.
 * @param root Source vertex for BFS
 */

/// Serial top-down BFS reference implementation
void BFSTopDown(const Graph& graph, Solution& solution, int root);

/// Serial bottom-up BFS reference implementation
void BFSBottomUp(const Graph& graph, Solution& solution, int root);

/// Parallel top-down BFS
void ParallelBFSTopDown(const Graph& graph, Solution& solution, int root);

/// Parallel bottom-up BFS
void ParallelBFSBottomUp(const Graph& graph, Solution& solution, int root);

/// Parallel hybrid BFS
void ParallelBFSHybrid(const Graph& graph, Solution& solution, int root);
///@}

/**
 * @brief Return true if solution passes basic validations
 *
 * @param graph Input graph
 * @param solution Distances computed by BFS algorithms
 * @param root Source vertex for BFS
 */
bool ValidateSolution(const Graph& graph, const Solution& solution, int root);

// Forward declarations to break cycle in declarations
class VertexQueue;
class VertexBitMap;

/**
 * @brief Queue of vertices to maintain the frontier
 *
 * This data structure was designed to support multiple synchronization strategies
 * and so may not be optimal for your particular strategy. If you choose to modify this
 * data structure, make a new version, e.g. `ParallelVertexQueue`.
 */
class VertexQueue {
 public:
  typedef int const* const_iterator;

  VertexQueue(int max_size) : max_size_(max_size), next_head_(0) {
    vertex_queue_ = new int[max_size_];
  }
  // Provide move constructor to enable efficient return-by-value
  VertexQueue(VertexQueue&& other);
  ~VertexQueue() { delete[] vertex_queue_; }


  // Iterators for accessing vertices in the queue. Compatible with range-based for loops.
  const_iterator begin() const { return vertex_queue_; }
  const_iterator end() const { return vertex_queue_ + next_head_; }

  /// Current size of the queue
  int size() const { return next_head_; }

  /// Maximum allowed size of the queue
  int max_size() const { return max_size_; }

  /// Access vertex by index in the queue
  int& operator[](int index) { return vertex_queue_[index]; }
  int operator[](int index) const { return vertex_queue_[index]; }

  /// Push a vertex onto the back of the queue
  void push_back(int vertex) { vertex_queue_[next_head_++] = vertex; }

  /// Push all the vertices from `other` VertexQueue onto this queue
  void push_back(const VertexQueue& other);

  /// Clear all entries in this queue (afterwards size is 0)
  void clear() { next_head_ = 0; }

  int max_size_;
  int next_head_;
  int* vertex_queue_;

 private:
  VertexQueue();
};

/**
 * @brief Queue of vertices to maintain the frontier
 *
 * This data structure was designed to support multiple synchronization strategies
 * and so may not be optimal for your particular strategy. If you choose to modify this
 * data structure, make a new version, e.g. `ParallelVertexBitMap`.
 *
 * Note that although this is described as a bit map, it is not implemented that way. The
 * std::vector<bool> specialization would be a space efficient alternative, however that container
 * does not guarantee that different elements in the same container can be modified concurrently by
 * different threads.
 */
class VertexBitMap {
  // Underlying storage type in "bit" map
  typedef uint8_t word_type;
 public:
  VertexBitMap(int vertices);
  // Provide move constructor to enable efficient return-by-value
  VertexBitMap(VertexBitMap&& other);
  ~VertexBitMap() { delete[] vertex_bits_; }


  /// Total size of the bitmap
  int size() const { return size_; }

  /// Return value for `index`
  bool operator[](int index) const { return vertex_bits_[index]; }

  /// Set value at `index` to be true
  void set(int index) { vertex_bits_[index] = 1; }

  
  /// Return true if any values are true
  bool any() const {
    for (int i = 0; i < size_; i++) {
      if (vertex_bits_[i]) return true;
    }
    return false;
  }

  /// Clear all entries (set all "bits" to be false)
  void clear();

  int size_;
  word_type* vertex_bits_;

 private:
  VertexBitMap();
};

/// Specialize swap for VertexQueue. Efficiently swaps data members in objects.
void swap(VertexQueue& lhs, VertexQueue& rhs);

/// Specialize swap for VertexBitMap. Efficiently swaps data members in objects.
void swap(VertexBitMap& lhs, VertexBitMap& rhs);
