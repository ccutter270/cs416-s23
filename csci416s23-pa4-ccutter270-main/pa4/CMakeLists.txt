# Add current directory to include search path (so tests can find headers)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINE_VERBOSE)
  message("Adding VERBOSE define flag...")
  add_compile_definitions(VERBOSE)
endif(DEFINE_VERBOSE)

add_executable(pa4-main
  main.cc
  graph.cc
  bfs.cc
)
target_link_libraries(pa4-main PUBLIC OpenMP::OpenMP_CXX)
set_source_files_properties(
    bfs.cc
    PROPERTIES
    COMPILE_FLAGS "-Wno-unused-function -Wno-unused-but-set-variable"
)

add_executable(graph-main
  graph-main.cc
  graph.cc
)