# Add current directory to include search path (so tests can find headers)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Link threading library to all executables in this assignment (needed for C++ threads)
link_libraries(Threads::Threads)  

add_executable(pa2-main
  main.cc
  tasksys.cc
  test/tasks.h
)