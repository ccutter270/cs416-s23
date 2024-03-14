#include <getopt.h>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "graph.h"

// Specify expected options and usage
const char* kShortOptions = "h";
const struct option kLongOptions[] = {{"help", no_argument, nullptr, 'h'},
                                      {nullptr, 0, nullptr, 0}};

void PrintUsage(const char* program_name) {
  printf("Usage: %s [options] COMMAND\n", program_name);
  printf("Commands:\n");
  printf("  dot <GRAPH FILE> <DOT FILE>\n");
  printf("  stats <GRAPH FILE>\n");
  printf("Options:\n");
  printf("  -h --help           Print this message\n");
}

int main(int argc, char** argv) {
  {
    int opt;
    while ((opt = getopt_long(argc, argv, kShortOptions, kLongOptions, nullptr)) != -1) {
      switch (opt) {
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

  if (optind == argc) {
    fprintf(stderr, "Error: Missing command argument\n");
    PrintUsage(argv[0]);
    return 1;
  }

  std::string command(argv[optind]);
  if (command == "dot") {
    if (optind + 3 > argc) {
      fprintf(stderr, "Error: Missing command positional arguments\n");
      PrintUsage(argv[0]);
      return 1;
    }

    const char* graph_file = argv[optind + 1];
    const char* dot_file = argv[optind + 2];
    
    Graph graph(graph_file);
    graph.WriteDOTFile(dot_file);
  } else if (command == "stats") {
    if (optind + 2 > argc) {
      fprintf(stderr, "Error: Missing command positional arguments\n");
      PrintUsage(argv[0]);
      return 1;
    }

    const char* graph_file = argv[optind + 1];
    Graph graph(graph_file);

    int total_outgoing = 0;
    int total_incoming = 0;

    for (int i=0; i<graph.vertices(); i++) {
      total_outgoing += graph.outgoing_size(i);
      total_incoming += graph.incoming_size(i);
    }

    printf("Statistics for '%s'\n", graph_file);
    printf("Vertices: %d\n", graph.vertices());
    printf("Edges: %d\n", graph.edges());
    printf("Average out-degree: %f\n", (float)total_outgoing / graph.vertices());
    printf("Average in-degree: %f\n", (float)total_incoming / graph.vertices());
  } else {
    fprintf(stderr, "Error: Invalid command %s\n", command.c_str());
    PrintUsage(argv[0]);
    return 1;
  }

}