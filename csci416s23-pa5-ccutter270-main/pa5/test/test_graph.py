# Generate random test graphs for the PageRank program. Adapted from the
# cs149 graph generator.

import random, sys

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Missing one or more required arguments", file=sys.stderr)
        print("Usage: test_graph.py <vertices>", file=sys.stderr)
        sys.exit(-1)

    n = int(sys.argv[1])

    vertices = random.sample(range(1, 10*n+1), n)

    for i in range(n):
        # Ensure there exists a cycle containing all nodes
        print(f"{vertices[i]} {vertices[(i+1) % n]}")

    # Generate additional random edges
    for i in range(9*n):
        edge = random.sample(vertices, 2)
        if edge[0] == 1:
          continue
        print(*edge)

