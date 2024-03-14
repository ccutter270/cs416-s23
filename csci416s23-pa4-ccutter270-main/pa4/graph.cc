#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "graph.h"

static const int kGRAPH_HEADER_TOKEN = 0xDEADBEEF;

void Graph::ReadBinaryFile(const char* filename) {
  std::ifstream file(filename, std::ios::binary);
  assert(file);

  int header[3];
  file.read(reinterpret_cast<char*>(header), 3 * sizeof(int));
  assert(file.gcount() == 3 * sizeof(int));
  assert(header[0] == kGRAPH_HEADER_TOKEN);

  vertices_ = header[1];
  edges_ = header[2];

  // Initialize data structures
  outgoing_edge_starts_.resize(vertices_ + 1);
  outgoing_edges_.resize(edges_);

  // Read the edge data
  file.read(reinterpret_cast<char*>(outgoing_edge_starts_.data()), vertices_ * sizeof(int));
  assert(file.gcount() == static_cast<std::streamsize>(vertices_ * sizeof(int)));
  file.read(reinterpret_cast<char*>(outgoing_edges_.data()), edges_ * sizeof(int));
  assert(file.gcount() == static_cast<std::streamsize>(edges_ * sizeof(int)));

  outgoing_edge_starts_[vertices_] = edges_;  // Set extra start to avoid special-casing last vertex
}

void Graph::ReadTextFile(const char* filename) {
  std::ifstream file(filename);
  std::string buffer;

  // The first line should be "AdjacencyGraph"
  std::getline(file, buffer);
  assert(buffer == "AdjacencyGraph");

  // Vertex count is first, then edge count
  do {
    std::getline(file, buffer);
  } while (buffer.size() == 0 || buffer[0] == '#');
  vertices_ = std::stoi(buffer);

  do {
    std::getline(file, buffer);
  } while (buffer.size() == 0 || buffer[0] == '#');
  edges_ = std::stoi(buffer);

  // Initialize data structures
  outgoing_edge_starts_.assign(vertices_ + 1, 0);
  outgoing_edges_.clear();
  outgoing_edges_.reserve(edges_);

  // Read the remainder of the file
  int src = 0;
  while (std::getline(file, buffer)) {
    if (buffer.size() > 0 && buffer[0] == '#') continue;

    outgoing_edge_starts_[src++] = static_cast<int>(outgoing_edges_.size());

    // Effectively split the string on whitespace
    std::stringstream parse(buffer);
    while (!parse.fail()) {
      int dst;
      parse >> dst;
      if (parse.fail()) {
        break;
      }
      outgoing_edges_.push_back(dst);
    }
  }
  assert(src == vertices_);
  outgoing_edge_starts_[vertices_] = edges_;  // Set extra start to avoid special-casing last vertex
}

Graph::Graph(const char* filename) {
  // Try to read as binary file first (look for magic number at the beginning of the file)
  std::ifstream file(filename, std::ios::binary);
  if (!file.good()) {
    fprintf(stderr, "Could not open file '%s'. Is the path correct>\n", filename);
    exit(1);
  }
  assert(file);

  int magic_number = 0;
  file.read(reinterpret_cast<char*>(&magic_number), sizeof(int));
  assert(file.gcount() == sizeof(int));
  file.close();

  if (magic_number == kGRAPH_HEADER_TOKEN) {
    ReadBinaryFile(filename);
  } else {
    ReadTextFile(filename);
  }
  assert(static_cast<int>(outgoing_edge_starts_.size()) == vertices_ + 1 &&
         outgoing_edge_starts_[vertices_] == edges_);
  assert(static_cast<int>(outgoing_edges_.size()) == edges_);

  // Construct the incoming edges from the outgoing edges
  SetIncomingEdges();
  assert(static_cast<int>(incoming_edge_starts_.size()) == vertices_ + 1 &&
         incoming_edge_starts_[vertices_] == edges_);
  assert(static_cast<int>(incoming_edges_.size()) == edges_);
}

void Graph::SetIncomingEdges() {
  incoming_edge_starts_.assign(vertices_ + 1, 0);
  incoming_edges_.clear();
  incoming_edges_.reserve(edges_);

  std::unordered_map<int, std::vector<int> > edges;

  for (int i = 0; i < vertices_; i++) {
    for (int dst : outgoing_range(i)) {
      edges[dst].push_back(i);  // Create or update vector of incoming vertices
    }
  }

  // Extract map into CSR representation
  for (int i = 0; i < vertices_; i++) {
    incoming_edge_starts_[i] = static_cast<int>(incoming_edges_.size());
    incoming_edges_.insert(incoming_edges_.end(), edges[i].begin(), edges[i].end());
  }
  assert(static_cast<int>(incoming_edges_.size()) == edges_);
  incoming_edge_starts_[vertices_] = edges_;
}

void Graph::WriteDOTFile(const char* filename, const char* name) const {
  std::ofstream file(filename);
  assert(file);
  file << "digraph " << name << " {" << std::endl;
  for (int src = 0; src < vertices_; src++) {
    for (int dst : outgoing_range(src)) {
      file << "\t" << src << " -> " << dst << ";" << std::endl;
    }
  }
  file << "}" << std::endl;
}