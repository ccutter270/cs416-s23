# CS416 Programming Assignment 4

This assignment consists of several implementation parts and several questions to answer. Modify this README file with your answers. Only modify the specified files in the specified way. Other changes will break the programs and/or testing infrastructure.

You are encouraged to develop and perform correctness testing on the platform of your choice, but you should answer the questions below based on your testing on the *ada* cluster, and specifically the compute nodes of the cluster (which have more cores than the head node). Recall that you don't access the compute nodes directly, instead you submit jobs to the queueing system (SLURM) which executes jobs on your behalf. Thus you will test your program by submitting cluster jobs and reviewing the output.

## Getting Started

1. Connect to *ada* via `ssh username@ada.middlebury.edu` (if you are off-campus you will need to use the VPN), logging in with your regular Middlebury username and password (i.e. replace `username` with your username).

    When you connect to *ada* you are logging into the cluster "head" node. You should use this node for compilation, interacting with GitHub, and other "administrative" tasks, but all testing will occur via submitting jobs to the cluster's queueing system.

2. Clone the assignment skeleton from GitHub to *ada* or copy from the files from your local computer.

3. Load the CS416 environment by executing the following:

    ```
    module use /home/mlinderman/modules/modulefiles
    module load cs416/s23
    ```

    These commands configure the environment so that you can access the tools we use in class. You will need to execute these commands every time you log in. Doing so is tedious, so instead you can add the two lines your `.bash_profile` file (at `~/.bash_profile`) so they execute every time you login.

4. Prepare the skeleton for compilation

    In the root directory of the skeleton, prepare for compilation by invoking `cmake3 .`. Note the *3*. You will need to explicitly invoke `cmake3` on *ada* to get the correct version, on other computers it will just be `cmake`. You only need to do this once for each assignment.

5. Change to the assignment directory and compile the assignment programs

    ```
    cd pa4
    make
    ```

6. Submit a job to the cluster to test your programs

    The assignment directory includes `ada-submit`, a cluster submission script that invokes the assignment program. This script will request the entire node for testing. In the following example I submitted the script with the `sbatch` command, specifying 4 cores. SLURM (the cluster job queueing system) responded that it created job 4238.
    
    ```
    [mlinderman@ada pa1]$ sbatch --cpus-per-task 4 ada-submit 
    Submitted batch job 4238
    ```

    I then checked the cluster status to see if that job is running (or pending). If you don't see your job listed, it has already completed. 

    ```
    [mlinderman@ada pa1]$ squeue
             JOBID PARTITION     NAME     USER ST       TIME  NODES NODELIST(REASON)
              4238     short      pa1 mlinderm  R       0:09      1 node004
    ```

    And when it is done, I reviewed the results by examining the output file `pa4-4238.out` created by SLURM (i.e. `pa4-<JOB ID>.out`). Review the guide on Canvas for more details on working with *ada*.

Each subsequent time you work on the assignment you will need start with steps 1 and 3 above, then recompile the test program(s) and resubmit your cluster job each time you change your program.

## Introduction

In this assignment you will gain further experience developing parallel programs for shared-memory multi-core computers, but using the [OpenMP](http://openmp.org) API and C/C++ extensions instead of C++ threads. Recall that OpenMP is designed for you to easily exploit loop-based parallelism by extending your existing program with `#pragma`s that instruct the compiler to assign loop iterations to different threads, implement locks, etc. To practice this extension approach, you **should not** use any of the C++11 multi-threading features, i.e. do not use `thread`, `atomic`, `mutex`, etc. types.

We will be targeting OpenMP 3.1. Alongside the lecture notes and Google, you may find the following resources helpful:
* [OpenMP 3.1 "cheat sheet"](https://www.openmp.org/wp-content/uploads/OpenMP3.1-CCard.pdf)
* [OpenMP 3.1 Specification](https://www.openmp.org/wp-content/uploads/OpenMP3.1.pdf)

We will use OpenMP to parallelize breadth-first-search (BFS) over a directed graph (hopefully a familiar algorithm!). In our version of BFS you will calculate the distance from a "root" vertex for all vertices.

The starter code includes a serial implementation for a "top-down" approach *and* for a "bottom-up" approach. In the "top-down" approach, the frontier expands outwards in each step to visit all the vertices at the same depth (before visiting any deeper vertices). In the provided "conventional" approach, in each step, each vertex in the current frontier checks all of its neighbors to see if any have not yet been visited. Any unvisited neighbor is added to the next frontier and its distance calculated based on the distance of the current vertex.

An alternate approach is to perform BFS from the "bottom-up", that is each vertex will to add itself to the next frontier if any of its neighbors are in the current frontier. The "bottom-up" approach can have performance and parallelism advantages *in certain situations*. I encourage you to review the following (very readable) paper for a deeper discussion of these two approaches.

Scott Beamer, Krste Asanović, and David Patterson. 2012. [Direction-optimizing breadth-first search](https://www.icsi.berkeley.edu/pubs/arch/directionoptimizing12.pdf). In Proceedings of the International Conference on High Performance Computing, Networking, Storage and Analysis (SC ’12). Article 12, 1–10.

Beamer et al. show that a hybrid algorithm combining the "top-down" and "bottom-up" can accelerate BFS. Your ultimate goal is to implement a fast parallel hybrid algorithm for BFS.

The skeleton includes extensive supporting code for storing graphs and vertices. All vertices are labeled with 0-indexed integers (in a contiguous range). Graphs (defined in <tt>graph.h</tt>) are represented as adjacency matrices stored in a compressed sparse row (CSR) format. In a CSR-format, the graph is represented as an array of `outgoing_edges` and `incoming_edges` that store the destination and source vertices for all edges, respectively. The edges are stored in contiguous chunks for each vertex, sorted in vertex order. That is the `outgoing_edges` array contains all the edges for vertex 0, then all edges for vertex 1, 2, and so on. A separate array stores the starting index for each vertex's edges. To facilitate iterating through a node's edges, the graph provides iterators for the incoming and outgoing edges for each node and a lightweight edge "range" that can be used with the [C++ range-based for loop](https://en.cppreference.com/w/cpp/language/range-for). For example, the following code concisely iterates through the outgoing edges for `current_vertex`.

```cpp
for (int neighbor : graph.outgoing_range(current_vertex)) {
  // Iterates through the outgoing edges of current vertex. In each
  // iteration neighbor is set to the next "outgoing" neighbor
  ...
}
```

The skeleton includes code for reading binary or plain-text graph files and via the <tt>graph-main</tt> helper program, writing a graph in the human-readable [DOT format][1] (that can also be automatically rendered as an image by the [dot](https://en.wikipedia.org/wiki/Graphviz) tool).

Several small test graphs are included in the <tt>test</tt> directory and many more, much larger, graphs are available on *ada* (including some with 100s of millions of edges). You can access these files via the `$GRAPHS` environment variable (set in the course module) which points to the directory containing the graph files, e.g. in the following invocation of the helper "stats" command.

```
[mlinderman@ada pa4]$ ./graph-main stats ${GRAPHS}/ego-twitter_2m.graph
Statistics for '/storage/mlinderman/courses/cs416/s23/graphs/ego-twitter_2m.graph'
Vertices: 81306
Edges: 2420766
Average out-degree: 29.773521
Average in-degree: 29.773521
```

## Part 1: Parallelizing Top-down BFS

In this part of the assignment you will parallelize the top-down BFS algorithm to achieve the best possible performance. Specifically you will implement the `ParallelBFSTopDown` function in <tt>bfs.cc</tt>. You can compile this and all subsequent parts with `make pa4-main` and run all parts by executing:

```
./pa4-main -t 2 test/tiny.graph
```

with an optional number of threads and the required path to graph file (above it is a test file, the same file is also available on *ada* at `${GRAPHS}/tiny.graph`).

I encourage you to familiarize yourself with the support code and the provided serial implementation before getting started. Alongside the `Graph` data structure, the skeleton code includes a `VertexQueue` data structure for maintaining the frontier.

As in previous programming assignments, you will need to identify and exploit potential sources of parallelism while incorporating the necessary synchronization to ensure your program produces correct results.

#### Implementation Notes and Suggestions

1. You are encouraged to optimize every aspect of `ParallelBFSTopDown`, however do not modify `BFSTopDown` or its supporting data structures (so that you maintain the same baseline as the reference implementation). If you want to modify a data structure used in `BFSTopDown`, make a new version of that data structure for use in your parallel code. Get started by copying the implementation for `BFSTopDown` into `ParallelBFSTopDown` and then start modifying `ParallelBFSTopDown` and any supporting functions you have copied (e.g. to introduce OpenMP pragmas). As always aim to go from working implementation to (more optimized) working implementation.
1. OpenMP provides a variety of synchronization primitives including critical regions `#pragma omp critical` and certain atomic operations (via `#pragma omp atomic`). However, for this problem you will likely find that the atomic "compare and swap" (CAS) operation is sufficient for some (or all) of your synchronization needs (and much faster). Unfortunately OpenMP does not have its own CAS, instead we can use the `__sync_bool_compare_and_swap` compiler built-in [function](https://gcc.gnu.org/onlinedocs/gcc/_005f_005fsync-Builtins.html#g_t_005f_005fsync-Builtins) (available in both GCC and Clang) that wraps the atomic instructions provided by the processor. We will use this function or its variants instead of `std::atomic` to practice extending existing code.
1. Although atomic operations can be faster than using a critical region, they still generate substantial coherence traffic (as we saw in class). Think about how you could structure your code to avoid attempting the CAS if you know it will fail (e.g. like our test-test-and-set lock example in class).
1. There is some logging code built into the skeleton to report the time for each step in the top-down BFS algorithm. You can turn this on and off by adding optional arguments to your CMake command, e.g. `cmake3 -DDEFINE_VERBOSE=ON .` to turn on the printing and `cmake3 -DDEFINE_VERBOSE=OFF .` to turn it off again.


## Part 2: Parallelizing Top-down BFS

In the next part of the assignment, you will similarly parallelize the bottom-up BFS algorithm to achieve the best possible performance. Specifically you will implement the `ParallelBFSBottomUp` function in <tt>bfs.cc</tt>.

#### Implementation Notes and Suggestions

1. As in previous parts, you are encouraged to optimize every aspect of `ParallelBFSBottomUp`, but make a new version of any supporting data structures you modify, e.g. create `ParallelVertexBitMap`. Again get started by copying the implementation for `BFSBottomUp` into `ParallelBFSBottomUp` and then start modifying `ParallelBFSBottomUp` and any supporting functions you have copied (e.g. to introduce OpenMP pragmas). As always aim to go from working implementation to (more optimized) working implementation.
1. How do the synchronization needs change for the bottom-up vs. top-down algorithm? What operations, if any, need to be synchronized?

## Part 3: Hybrid BFS

From your testing (and the literature review) you should be able to identify situations where one approach is much faster than the other. This leads us to the same optimization implemented by Beamer et al.: we could improve the performance by dynamically choosing the most efficient implementation based on the size or other properties of the current frontier and the graph as a whole. Implement the `ParallelBFSHybrid` function in <tt>bfs.cc</tt> to achieve the best possible performance. You will likely need to implement the hybrid approach to achieve performance competitive with the reference implementation.

#### Implementation Notes and Suggestions
1. Depending how you implemented your top-down and bottom-up approaches, you may be using different representations for the frontier. How will you convert between them and what is the performance implications of doing so?

### What to turn in

You should have implemented `ParallelBFSTopDown`, `ParallelBFSBottomUp` and `ParallelBFSHybrid` in <tt>bfs.cc</tt> and possibly added supporting data structures in <tt>bfs.h</tt>. Answer the following questions with a few sentences by editing this README (i.e. place your answer after the question, update any tables, etc.).

1. Describe how you implemented synchronization in your top-down and bottom-up implementations. What if any optimizations did you implement to minimize synchronization overhead.

2. How did you implement hybrid BFS. If you switched approaches dynamically, how did you decide to which implementation use at a given moment in the program?

3. What factors you do think limited the speedup of your implementation (and the reference implementation)?

## Grading

There are two performance targets for this assignment (assuming your implementation produces correct results). The first is to achieve speedup of at least **4✖️** for all three parallel functions for the <tt>random_10m.graph</tt>. The second, is to achieve execution time within 20% (i.e. 1.2✖️) of the reference implementation (for the <tt>grid1000x1000</tt>, <tt>soc-livejournal1_68</tt>, <tt>com-orkut_117m</tt>, <tt>random_500m</tt>, and <tt>rmat_200m</tt> graphs). You can run the reference implementation on *ada* by executing `pa4-ref` (if you have loaded the class module, the reference program will automatically be in your PATH). 

ssessment | Requirements
-------|--------
**R**evision needed | Some but not all components compile. Some but not all components are correct, or some but not all questions are answered, or the program does not meet the first performance target.
**M**eets Expectations | All components are correct, you meet the first performance target and your answers to the embedded questions meet expectations.
**E**xemplary | All requirements for *Meets Expectations* and the program meets the second, more demanding, performance target (tied to the reference implementation).

### Acknowledgements

This assignment was adapted from a course taught by Kayvon Fatahalian and Kunle Olukotun. The test datasets are obtained from the [Stanford Large Network Dataset Collection](https://snap.stanford.edu/data/) (via Stanford CS149).

[1]: https://en.wikipedia.org/wiki/DOT_(graph_description_language)
