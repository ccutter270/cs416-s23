# CS416 Programming Assignment 3

This assignment consists of several implementation parts and several questions to answer. Modify this README file with your answers. Only modify the specified files in the specified way. Other changes will break the programs and/or testing infrastructure.

You are encouraged to develop and perform correctness testing on the platform of your choice. *However, you will need access to a CUDA-compatible GPU to test your programs for this assignment.* It is unlikely that your laptop has such a GPU, and thus you will likely need to do all of your testing on *ada*. While you can install the CUDA compiler on a computer without a CUDA-compatible GPU, all you will be able to do is compile. 

However you test your programs, you should answer the questions below based on your testing on the *ada* cluster, and specifically the GPU node of the cluster. Recall that you don't access the compute nodes directly, instead you submit jobs to the queueing system (SLURM) which executes jobs on your behalf. Thus you will test your program by submitting cluster jobs and reviewing the output. 

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

    In the root directory of the skeleton, prepare for compilation by invoking `cmake3 .`. Note the *3*. You will need to explicitly invoke `cmake3` on *ada* to get the correct version, on other computers it will just be `cmake`. You only need to do this once.

5. Change to the assignment directory and compile the assignment programs

    ```
    cd pa3
    make
    ```

6. Submit a job to the cluster to test your programs

    The assignment directory includes `ada-submit`, a cluster submission script that invokes the assignment program. This script will specifically submit your jobs to the "GPU" queue (and request the entire node). In the following example I submitted the script with the `sbatch` command. SLURM (the cluster job queueing system) responded that it created job 4238.
    
    ```
    [mlinderman@ada pa3]$ sbatch ada-submit 
    Submitted batch job 4238
    ```

    I then checked the cluster status to see if that job is running (or pending). If you don't see your job listed, it has already completed. 

    ```
    [mlinderman@ada pa3]$ squeue
             JOBID PARTITION     NAME     USER ST       TIME  NODES NODELIST(REASON)
              4238     gpu-short      pa3 mlinderm  R       0:09      1 node002
    ```

    And when it is done, I reviewed the results by examining the output file `pa3-4238.out` created by SLURM (i.e. `pa3-<JOB ID>.out`). Review the guide on Canvas for more details on working with *ada*.

Each subsequent time you work on the assignment you will need start with steps 1 and 3 above, then recompile the test program(s) and resubmit your cluster job each time you change your program.

## Warming Up

Recall that SAXPY is a level-1 routine in the widely used and heavily optimized BLAS (Basic Linear Algebra Subproblems) library. BLAS provides functions for a variety of linear algebra operations. SAXPY implements "Single precision A*X+Y" where A is a scalar and X and Y are arrays.

The file `saxpy.cu` contains the skeleton for a CUDA implementation of SAXPY. Modify that file to complete a working implementation. You can compile this part with `make cusaxpy-main`.

As comparison points and for future, the skeleton includes implementations using optimized BLAS libraries: one for the host CPU and the other for the GPU (using the cuBLAS library distributed by NVIDIA). You don't need to modify these implementations in any way.
 
### What to turn in for this part

You should have modified `saxpy.cu`. Answer the following questions by editing this README (i.e. place your answer after the question, update any tables, etc.). Your answers should be on the order of several sentences.

1. Fill in the table below with speedup you observed for your CUDA implementation?

    | Implementation  | Speedup |
    | --------------- | ------- |
    | CUDA            |         |

2. Were you surprised by the result? What is the limiting factor? Are the bandwidth results consistent with your expectations? For context, the GPU is connected to the host via PCIe 4.0, which has a peak bandwidth of 31.5 GB/s. Look up the peak memory bandwidth for the [RTX A5000 GPU](https://nvdam.widen.net/s/wrqrqt75vh/nvidia-rtx-a5000-datasheet/) we are using.

## Circle Rendering

We will implement a program for rendering an image of layered partially transparent circles. An example output for 10,000 random circles is shown below:

![10k circles](assets/RGBRandom10k-serial.png)

Each circle has a x, y position, a radius and a color expressed as RGBA, i.e. the RGB color components and the alpha channel that specifies the opacity (1.0 is fully opaque).

The color is computed via alpha blending. The pseudo-code for computing the resulting color of blending a circle "over" an existing pixel (in the `pixel` variable) is

```
red = circle.alpha * circle.red + (1 - circle.alpha) * pixel.red;
green = circle.alpha * circle.green + (1 - circle.alpha) * pixel.green;
blue = circle.alpha * circle.blue + (1 - circle.alpha) * pixel.blue;
```

This computation is not commutative and so the order in which we apply this operator matters. To generate the correct image you need to apply the color of the circles in the order in which they are stored in the array. 

In `render.cu` file includes an initial but incorrect of implementation of the circle rendering. The skeleton is a nearly direct translation of the serial implementation, but parallelized across circles (each thread block blends one circle). Because the thread blocks can be executed in any order, there is no guarantee that the circles are applied in the correct order. You should observe that the resulting image is incorrect (and the test program will report an error). 

Modify the `RenderCUDAKernel` and the `RenderCUDA` function to produce the correct result and achieve the best possible performance. Do not modify the interface of the `RenderCUDA` function (or the test infrastructure). All other optimizations are permitted as long your program will produce correct results. You can compile this part with `make render-main`.

### Implementation Notes

1. There are two distinct coordinate systems in use. The circle center and radius are described in a normalized [0,1] coordinate space (i.e. the x,y coordinates are the range [0,1]), instead of pixel indices. This ensures that the description of the circles is independent of the size of the rendered image. We can compute the width and height of a pixel in the normalized coordinate space via `1.f / image_width` and `1.f / image_height` respectively. Most comparisons with circle regions, i.e. determining if a circle overlaps a rectangle are performed in the normalized coordinate space.
1. Whether a circle overlaps a pixel (and thus whether the circle coloring should be applied to the pixel) is determined based on the center of the pixel. That is a pixel is considered within a circle if the center of that pixel (in the normalized [0,1] coordinate space) is within the circle, and specifically if the distance between the center of the pixel and the center of the circle is less than the radius of the circle. The center of the pixel is obtained by adding 0.5 to the indices of that pixel in the image. 
1. The skeleton makes extensive use of the `float2` and `float4` types provided by CUDA. These simple `structs` contain two or four floats named x, y, z and w. For example:

    ```cpp
    struct float4 {
      float x, y, z, w;
    };
    ```

1. You are welcome and encouraged to use the [CUB](https://nvlabs.github.io/cub/) library in your implementation. This library provides optimized implementations of various data parallel primitives, e.g. scan, at the device and thread block-level, implemented as templated C++ functions or classes.

    The `evens.cu` file (part of the `evens-main` program ) includes two different examples of using CUB's block and device-level functions to implement stream compaction.

1. Several helper functions are provided in `render.cu` for checking if a circle overlaps a given rectangle. You are not required to use these functions but you may find them helpful.
1. There are three tests included:
    * "RGB": A simple set of three circles just used for correctness testing
    * "RGBRandom10k": A random set of 10,000 circles
    * "RGBRandom100k": A random set of 100,000 circles
    
    You can run just a single test at a time with the `-n` argument to `render-main`, e.g. `./render-main -n RGB`.

1. While you ultimately need to generate the full-sized 1024x1024 image, during testing you may find it helpful to reduce the image size and focus on the simpler set of three circles. You can do so with the `-s` and `-n` arguments to `render-main`, e.g. `./render-main -s 40 -n RGB` to render a simple 40x40 image.
1. All of the CUDA calls are wrapped in the `cudaErrorCheck` macro. This macro will report any errors in CUDA runtime functions and terminate the program. Note that you can't directly catch errors in CUDA kernel launches, instead the error is reported at the next runtime function, e.g. `cudaErrorCheck( cudaDeviceSynchronize() );`. If you don't wrap a function and it has an error, that error will be reported at the next wrapped function. Thus you are encouraged to wrap every function (as should be the case for all the code in the skeleton).
1. If needed for debugging, it is possible to [print from CUDA kernels](https://docs.nvidia.com/cuda/archive/11.1.1/cuda-c-programming-guide/index.html#formatted-output). Keep in mind that all CUDA threads will execute the print, so you will likely want to restrict the printing to just a subset of your threads.

### Suggestions

1. Start by understanding why the skeleton implementation is incorrect, then make the simplest modifications you can to achieve the correct results. Specifically, can you restructure your application to ensure that the circles are "applied" in the correct order and there are no race-conditions when updating a pixel color?
1. Once you have a correct implementation, start to think about which aspects may be limiting performance. For example, are you exploiting all of the available parallelism? Aim to go from working program to slightly more optimized working program. With each optimization keep a record of your performance (so that you know you are making progress and have data for your writeup).
1. There are two forms of parallelism available: parallelism across circles and parallelism across pixels. You will likely need to exploit both, albeit in different phases of the computation.
1.  Note that not all circles overlap all pixels. And thus while circles must be applied in the correct order, the relevant subset of that order will be different for different pixels. As a suggestion, can you quickly and efficiently figure out which circles are not relevant for a given pixel(s)?
1. Where is there data reuse in your renderer and how can you exploit that reuse? As a general rule we want to minimize accesses to device main memory. Can you turn accesses to main memory into accesses to a thread-local variable or block shared memory?
1. There are likely several implementation strategies that will achieve the necessary performance. One possible approach utilizes the CUB library's within-thread block exclusive scan operation.
1. Keep in mind that there is a finite amount of block shared memory available on a SM (and thus within a thread block). For example, you will not be able store all of circles (of which there can be an arbitrary number) in block shared memory. One approach to navigating this limitation is to design your thread blocks to process a fixed-size chunk of data at one time. These chunks only need to be big enough to keep all of the threads in the thread block busy.

### What to turn in

You should have modified the `render.cu` file. Answer the following questions with a few sentences or paragraphs by editing this README (i.e. place your answer after the question, update any tables, etc.).

1. Describe how you implemented your renderer, including how you partitioned work among the CUDA threads throughout the computation.

2. What optimizations did you attempt during development? Why did those optimizations work or not? I am looking for you to concisely describe how your approach evolved over time.

## Grading

There are two performance targets for this assignment (assuming your implementation produces correct results). The first is to achieve **15✖️** on the `RGBRandom10k` and `RGBRandom100k` benchmarks with the A5000 GPUs (node020). This corresponds to exploiting one of the two forms of parallelism in the application. The second, is to achieve performance within 20% (i.e. 1.2✖️) of the reference implementation (ignore the performance of the simple "RGB" image, it is only used for correctness testing). You can run the reference implementation on *ada* by executing `pa3-render-ref` (if you have loaded the class module, the reference program will automatically be in your PATH). 

*Bonus competition* The fastest renderer (as run on *ada*) will win a small prize! Unfortunately we can't track this with Gradescope, it will be evaluated separately.

Assessment | Requirements
-------|--------
**R**evision needed | Some but not all components compile. Some but not all components are correct, or some but not all questions are answered, or the render does not meet the first performance target.
**M**eets Expectations | All components are correct, you meet the first performance target for the renderer and your answers to the embedded questions meet expectations.
**E**xemplary | All requirements for *Meets Expectations* and the renderer meets the second, more demanding, performance target (tied to the reference implementation).

### Acknowledgements

This assignment was adapted from a course taught by Kayvon Fatahalian and Kunle Olukotun.
