find_package(CBLAS)
if (NOT CBLAS_FOUND)
    message(FATAL_ERROR "Could not find CBLAS library. Make sure you have completed the 'Getting Started' instructions on Canvas. Exiting.")
endif()
include_directories(${CBLAS_INCLUDE_DIRS})

if (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
  set(ISPC_TARGET "neon-i32x8")
else()
  # Target 8-wide AVX2 SIMD extensions
  set(ISPC_TARGET "avx2-i32x8")
endif()

# Link threading library to all executables in this assignment (needed for ISPC tasks)
link_libraries(Threads::Threads)  

# Add binary directory where ISPC header files are generated to search path
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Mandelbrot
add_ISPC_object(NAME mandelbrot_ispc 
  SRC_FILE mandelbrot.ispc
  ARCH ${ISPC_ARCH}
  TARGET ${ISPC_TARGET}
  FLAGS "${ISPC_FLAGS};--opt=disable-fma"  # FMA triggers difference in Mandelbrot results
)

add_executable(mandelbrot-main
    mandelbrot-main.cc
    mandelbrot.h
    mandelbrot-support.cc
    mandelbrot.cc
    ${mandelbrot_ispc_OBJECTS}
    $<TARGET_OBJECTS:common_objs>
)

if (HAVE_INTRINSICS)
  # SQRT
  add_ISPC_object(NAME sqrt_ispc 
    SRC_FILE sqrt.ispc
    ARCH ${ISPC_ARCH}
    TARGET ${ISPC_TARGET}
    FLAGS ${ISPC_FLAGS}
  )

  add_executable(sqrt-main
      sqrt-main.cc
      sqrt.cc
      ${sqrt_ispc_OBJECTS}
      $<TARGET_OBJECTS:common_objs>
  )

  # To use intrinsics we need to specify `arch`, however we don't want this is a
  # global option because it can cause arithmetic differences for Mandelbrot and
  # other benchmarks
  set_source_files_properties(
      sqrt.cc
      PROPERTIES
      COMPILE_FLAGS "-march=native -Wno-unknown-pragmas -Wno-unused-function"
  )
else()
  message(STATUS "Can't build sqrt-main on non-X86 systems.")
endif()

# SAXPY
add_ISPC_object(NAME saxpy_ispc 
  SRC_FILE saxpy.ispc
  ARCH ${ISPC_ARCH}
  TARGET ${ISPC_TARGET}
  FLAGS ${ISPC_FLAGS}
)

add_executable(saxpy-main
    saxpy-main.cc
    ${saxpy_ispc_OBJECTS}
    $<TARGET_OBJECTS:common_objs>
)
target_link_libraries(saxpy-main ${CBLAS_LIBRARIES})

set_source_files_properties(
    saxpy-main.cc
    PROPERTIES
    COMPILE_FLAGS -Wno-unknown-pragmas
)