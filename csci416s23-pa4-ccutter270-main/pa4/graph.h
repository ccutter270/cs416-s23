#pragma once

#include <vector>

/**
 * @brief Lightweight range object for edges in Graph to enable use of range-based for loops
 */
class EdgeRange {
 public:
  typedef int const* const_iterator;
  EdgeRange(const_iterator begin, const_iterator end) : begin_(begin), end_(end) {}

  bool empty() const { return end_ == begin_; }
  int size() const { return end_ - begin_; }

  const_iterator begin() const { return begin_; }
  const_iterator end() const { return end_; }

  const_iterator begin_;
  const_iterator end_;
};

class Graph {
 public:
  typedef EdgeRange::const_iterator const_iterator;

  Graph() : vertices_(0), edges_(0) {}
  Graph(const char* filename);

  int vertices() const { return vertices_; }
  int edges() const { return edges_; }

  ///@{
  /**
   * Iterators for outgoing edges from a vertex
   */
  const_iterator outgoing_begin(int vertex) const {
    return outgoing_edges_.data() + outgoing_edge_starts_[vertex];
  }
  const_iterator outgoing_end(int vertex) const {
    // We add an extra element to serve as the end of the last vertex
    return outgoing_edges_.data() + outgoing_edge_starts_[vertex + 1];
  }
  EdgeRange outgoing_range(int vertex) const {
    return EdgeRange(outgoing_begin(vertex), outgoing_end(vertex));
  }
  ///@}

  /// Number of outgoing edges for vertex
  int outgoing_size(int vertex) const {
    // We add an extra element to serve as the end of the last vertex
    return outgoing_edge_starts_[vertex + 1] - outgoing_edge_starts_[vertex];
  }

  ///@{
  /**
   * Iterators for incoming edges from a vertex
   */
  const_iterator incoming_begin(int vertex) const {
    // We add an extra element to serve as the end of the last vertex
    return incoming_edges_.data() + incoming_edge_starts_[vertex];
  }
  const_iterator incoming_end(int vertex) const {
    return incoming_edges_.data() + incoming_edge_starts_[vertex + 1];
  }
  EdgeRange incoming_range(int vertex) const {
    return EdgeRange(incoming_begin(vertex), incoming_end(vertex));
  }
  ///@}

  /// Number of incoming edges for vertex
  int incoming_size(int vertex) const {
    // We add an extra element to serve as the end of the last vertex
    return incoming_edge_starts_[vertex + 1] - incoming_edge_starts_[vertex];
  }

  /**
   * @brief Write graph to text DOT format. Should only be used with small graphs.
   *
   * @param filename File to write to
   * @param name Name for graph in DOT file
   */
  void WriteDOTFile(const char* filename, const char* name = "adigraph") const;

 public:
  int vertices_;
  int edges_;

  std::vector<int> outgoing_edge_starts_;
  std::vector<int> outgoing_edges_;

  std::vector<int> incoming_edge_starts_;
  std::vector<int> incoming_edges_;

 private:
  /// Read binary encoded graph file
  void ReadBinaryFile(const char* filename);

  /// Read plain text file
  void ReadTextFile(const char* filename);

  /// Populate incoming edges from outgoing edges
  void SetIncomingEdges();
};