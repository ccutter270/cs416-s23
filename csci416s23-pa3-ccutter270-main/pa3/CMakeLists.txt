find_package(CBLAS)
if (NOT CBLAS_FOUND)
    message(FATAL_ERROR "Could not find CBLAS library. Make sure you have completed the 'Getting Started' instructions on Canvas. Exiting.")
endif()
include_directories(${CBLAS_INCLUDE_DIRS})

find_package(CUB)
if (NOT CUB_FOUND)
  message(FATAL_ERROR, "Could not CUB library. Make sure you have completed the 'Getting Started' instructions on Canvas. Exiting.")
endif()
include_directories(${CUB_INCLUDE_DIRS})

# Add current directory to include search path (so tests can find headers)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Link threading library to all executables in this assignment (needed for C++ threads)
link_libraries(Threads::Threads)  

add_executable(cusaxpy-main
  cusaxpy-main.cc
  saxpy.cu
)
target_link_libraries(cusaxpy-main ${CBLAS_LIBRARIES} cublas)

set_source_files_properties(
    cusaxpy-main.cc
    PROPERTIES
    COMPILE_FLAGS -Wno-unknown-pragmas
)

add_executable(evens-main
    evens-main.cc
    evens.cu
)

set_source_files_properties(
    evens-main.cc
    PROPERTIES
    COMPILE_FLAGS -Wno-unknown-pragmas
)

add_executable(render-main
    render-main.cc
    render.h
    render.cc
    render.cu
)