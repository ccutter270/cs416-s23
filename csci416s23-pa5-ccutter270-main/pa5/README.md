# CS416 Programming Assignment 5

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

    These commands configure the environment so that you can access the tools we use in class. You will need to execute these commands every time you log in. Doing so is tedious, so instead you can add the two lines above to your `.bash_profile` file (at `~/.bash_profile`) so they execute every time you login.

4. In this assignment all of our programs will be implemented in Python so no compilation is required. 

5. Change to the assignment directory

    ```
    cd pa5
    ```

6. Submit a job to the cluster to test your programs

    Apache Spark programs can be run locally on one or more cores or distributed across a cluster. We will use both modes and so there are two cluster submission scripts for use on *ada*. `ada-submit-local` will run Spark on a single node, while `ada-submit-cluster` will run your Spark program on multiple nodes. To do so, the submission script needs to start up a Spark cluster within *ada* and so this takes slightly longer. In the following example I submitted the cluster script with the `sbatch` command. SLURM (the cluster job queueing system) responded that it created job 7397.
    
    ```
    [mlinderman@ada pa1]$ sbatch ada-submit-cluster 
    Submitted batch job 7397
    ```

    I then checked the cluster status to see if that job is running (or pending). If you don't see your job listed, it has already completed. Notice that in this case, my job is running across two nodes.

    ```
    [mlinderman@ada pa5]$ squeue
             JOBID PARTITION     NAME     USER ST       TIME  NODES NODELIST(REASON)
              7397 short-spa      pa5 mlinderm  R       0:10      2 node[005-006]
    ```

    And when it is done, I reviewed the results by examining the output file `pa5-7397.out` created by SLURM (i.e. `pa5-<JOB ID>.out`). Review the guide on Canvas for more details on working with *ada*.

Each subsequent time you work on the assignment you will need start with steps 1 and 3 above, then resubmit your cluster job each time you change your program.

## Introduction

In this assignment we will implement two programs using the Python interface for Apache Spark. Spark has extensive online [documentation](https://spark.apache.org/docs/latest/index.html); the parts that will likely be most useful to you are the:
* [RDD programming guide](https://spark.apache.org/docs/latest/rdd-programming-guide.html)
* [Python API documentation](https://spark.apache.org/docs/latest/api/python/index.html)

We launch Spark programs with the `spark-submit` script provided in the Spark distribution. Spark can be run in both "local" and "cluster" mode. The following command would execute the `wordcount.py` program in "local" mode (with a single argument "test/hanselandgretel.txt") using a single core (controlled by the `local` argument to `--master`):
```
spark-submit --master local wordcount.py test/hanselandgretel.txt
```
while the following would run the same program in "local" mode using all of the available cores (indicated by `'local[*]'`):
```
spark-submit --master 'local[*]' wordcount.py test/hanselandgretel.txt
```

As with previous assignments, the skeleton includes an job submission script for use with *ada*. For this assignment there are two scripts: `ada-submit-local`, which runs a program in "local" mode on a single node (like described above), and `ada-submit-cluster`, which runs a program in "cluster" mode on multiple *ada* nodes. Check out the script itself to learn about how this is done, in short, we launch Spark's native built-in cluster manager within the resources allocated to the job by Slurm. You should not need to modify any of that infrastructure, all you should need to change is the program executed at the end of the `ada-submit-cluster` script.

While the above commands are suitable for testing smaller datasets, or testing your program on your local computer, running larger Spark applications on *ada* requires additional configuration (to ensure sufficient storage, efficient use of memory, etc.). The two "ada-submit" scripts includes the necessary configuration, so make sure to use those scripts when testing.

## Part 1: Word Count

Word counting is the "Hello World" of distributed computing. Here we will implement a Spark program to print the top-10 words in large text documents (a familiar task for former CS150 students!). Your program should take a single file name as an argument and print the top-10 most common words in descending order of count (i.e. most common first). For simplicity we will treat anything separated by white space as a word (i.e., we can break a string into words with `split`) and will ignore punctuation and case (i.e. you do not need to strip out punctuation and should not change the case of any words).

When you run a Spark program there is extensive logging, most of which you can ignore. Your output will embedded among all that logging towards the end. The expected output for the test document (the story of Hansel and Gretel, obtained from [Project Gutenberg](https://www.gutenberg.org)) is shown below. Your output formatting should match what is shown below, i.e. word (with colon) and count separated by a tab, so the automated tests can interpret your results. We will use the timing data printed by the program as the "official" execution time.

```
$ spark-submit wordcount.py test/hanselandgretel.txt 
...
the:	159
and:	154
to:	72
a:	43
they:	42
of:	41
had:	40
was:	33
in:	32
little:	30
Total program time: 2.16 seconds
...
```

Alongside the test input included in the skeleton, there are larger text documents available on *ada*. You can access these files via the `$TEXTS` environment variable (set in the course module) which points to the directory containing the text files, e.g. executing `spark-submit wordcount.py $TEXTS/gutenberg-small.txt` should produce:

```
...
the:    52775
of:     29633
and:    29002
to:     22959
a:      15730
in:     14552
was:    9469
I:      9443
that:   8557
he:     7954
Total program time: 2.85 seconds
...
```

The following inputs are available (constructed by concatenating text files downloaded from Project Gutenberg):

* `gutenberg-small.txt` is a concatenation of 10 books, totalling ~ 4.7MB
* `gutenberg-all.txt` is a combination of all 53367 downloadable books, totalling 20GB!

#### Implementation Notes and Suggestions

1. Recall from class that Spark applications are implemented as a set of data parallel operations over RDDs. "Word Count" is very similar to a histogram operation, except instead of the entire histogram we only want the top-10 values.
1. Spark only actually performs the computations when an *action* (an operation returns values to the application or saves data to persistent storage) is executed. What *action* is most useful/relevant for this problem? Note that there are more actions [available](https://spark.apache.org/docs/latest/rdd-programming-guide.html#actions) than listed in the lecture slides.
1. A common trick when trying to sort positive numbers in descending order is to order the values based on the negation of the value.

### What to turn in

You should have updated `wordcount.py` to implement the Spark program. Answer the following questions by editing this README (i.e. place your answer after the question, update any tables, etc.).

1. Run your program with the large `gutenberg-all.txt` file local mode on the entire *ada* node (i.e. with `ada-submit-local`) and on two nodes (i.e. with `ada-submit-cluster`) and report the execution times. For context, the reference implementation took close to an hour to analyze `gutenberg-all.txt` with one core.

    | Resources   | Execution Time |
    | ----------- | -------------- |
    | Single node |                |
    | Two nodes   |                |

2. Based on your results what aspect(s) of the system do you think could be limiting performance and why?


## Part 2: PageRank

PageRank is another frequent benchmark for distributed systems (for example, we saw it in the COST paper). PageRank is a graph algorithm utilized in the Google search engine to assess the relative importance of a node in the graph (e.g. a web page) based on the weight of its incoming edges (links). We will implement the iterative, matrix-based, version of the algorithm.

Assume the graph has *n* vertices. We express the ranks R, a vector of length *n*. The iterative computation of the "next" ranks R<sub>t+1</sub> is:

R<sub>t+1</sub>= 𝛃⋅M⋅R<sub>t</sub>+(1-𝛃)/n

where 𝛃 is the "damping factor", which we will set to 0.80. *M* is a sparse matrix describing the connectivity of the graph and M⋅R is a matrix-vector product (think of R as a column vector). The M<sub>j,i</sub> (j-th row, i-th column) entry in *M* is defined as 1/L(i), where L(i) is the out degree of vertex *i*, if there is an edge from vertex *i* to vertex *j*, or 0 otherwise. The initial value of all *n* elements in R is (1-𝛃)/n. Any duplicate edges in the graph should be treated as a single edge between those nodes.

The pseudo-code for the algorithm would be:
```
R = n-length array with values set to (1-𝛃)/n
for t in range(iterations):
    R = 𝛃⋅M⋅R+(1-𝛃)/n
```
When we add a scalar (single value) to a vector we add that value to all elements in the vector, i.e. we add (1-𝛃)/n to all elements in the vector computed by 𝛃⋅M⋅R. The same is true for multiplying by a scalar.

In `pagerank.py`, implement the iterative PageRank algorithm using Spark for any number of iterations. Your program should print out the top-10 rank nodes and their scores (similar to "Word Count"). The skeleton includes code for loading the graph file (one edge per line, represented as source and destination vertices) and timing the program. Although all vertex identifiers are integers, you *cannot* assume the identifiers form a contiguous range from 0 to *n-1*. You can assume that the R vector fits in memory on the driver or executors and thus can be stored as either an RDD or as a Python data structure transmitted to the executors. You should assume that matrix M is too large to be stored in memory on any one node and so needs to be maintained as an RDD. You are expected to use Spark for all computations (i.e., not just implement the computation with another library) and are not permitted to use GraphX.

The `test` directory includes several test graphs. Your program should produce similar results (within numerical error) to what is shown below for 100 iterations. Your output formatting should match what is shown below, i.e. vertex identifier (with colon) and rank separated by a tab, so the automated tests can interpret your results.
* `small_graph.txt`
    ```
    173:    0.01673849720556171
    618:    0.016584381121521937
    121:    0.014592872462170194
    731:    0.014450982653880525
    628:    0.014361889304199958
    42:     0.014252316391446368
    293:    0.014123585256246535
    785:    0.014077464626957612
    443:    0.013859589818720142
    896:    0.0137779579557391
    ```
* `benchmark_graph.txt`
    ```
    7906:   0.0020762568528372854
    40:     0.0019597435167152642
    1620:   0.00184822500640023
    7507:   0.0017474922889297773
    9427:   0.0017393088138535236
    6219:   0.0017291230158542744
    9280:   0.0016962516118980491
    9332:   0.0016870428824123098
    5953:   0.001681307833614258
    2887:   0.0016791291433472767
    ```

#### Implementation Notes and Suggestions

1. The heart of the problem will be implementing matrix-vector multiplication (M⋅R) using Spark RDD operations. The matrix *M* can be very sparse (mostly 0s) so you will want to think about how to express "sparse" matrix-vector multiplication in terms of data parallel operations such as `map`, `reduceByKey`, etc.
1. Spark is designed for iterative computations where there is data reuse. Which RDDs do you reuse? Any RDD that is re-used should be cached (with the `cache` method, which persists the RDD to memory not disk).
1. There are likely many approaches to solving this problem efficiently. One possible approach is to utilize a join. Joins are a very powerful feature, but can be slow due to shuffling data. When one side of the join is small enough to fit in memory, one optimization is to implement a "broadcast" or map-side join. In a broadcast join the small data set is copied to all workers and the join is performed by looking up keys from the large pair-RDD in the small pair-RDD (converted to a dictionary) as part of a `map` operation (without any additional shuffling). For example the follow code adds the values of a "large" RDD and "small" RDD by key via a "broadcast" join.
    ```python
    # Use more efficient broadcast to send small RDD to workers as a map
    small_bcast = spark.sparkContext.broadcast(small_rdd.collectAsMap())
    join_result = large_rdd.map(lambda x: (x[0], small_bcast.value.get(x[0]) + x[1]))
    ```
1. The NumPy package is pre-installed for you on *ada*. You are welcome and encouraged to use that library for dot products and other arithmetic operations, if relevant for your implementation. You should not use that library to implement the matrix-vector multiplication in its entirety. You do not need to use NumPy to achieve performance parity with the reference implementation.

### What to turn in

You should have updated `pagerank.py` to implement the Spark program. Answer the following questions by editing this README (i.e. place your answer after the question, update any tables, etc.).

1. Run your program with the `benchmark_graph.txt` input file in local mode on the entire *ada* node (i.e. with `ada-submit-local`) and on two nodes (i.e. with `ada-submit-cluster`) and report the execution times.

    | Resources   | Execution Time |
    | ----------- | -------------- |
    | Single node |                |
    | Two nodes   |                |

2. In a few short paragraphs, describe your implementation approach. What, if any, approaches did you try but ultimately discard?

3. How would you approach the problem differently if the rank vector was too large to materialize in the memory of a single node (i.e. had to be maintained as an RDD)?

## Grading

There are two performance targets for this assignment (assuming your implementation produces correct results). The first is to achieve a word count execution time within 500% (i.e., 5✖️) of the reference implementation for the benchmark graph. The second, is to achieve execution time within 20% (i.e. 1.2✖️) of the reference implementation. The performance of the reference implementation will be posted to the class message board.

Assessment | Requirements
-------|--------
**R**evision needed | Some but not all components compile. Some but not all components are correct, or some but not all questions are answered, or the program does not meet the first performance target.
**M**eets Expectations | All components are correct, you meet the first performance target and your answers to the embedded questions meet expectations.
**E**xemplary | All requirements for *Meets Expectations* and the program meets the second, more demanding, performance target.

### Acknowledgements

This assignment was adapted from a course taught by Kayvon Fatahalian and Kunle Olukotun.
