cmake_minimum_required(VERSION 3.16)
project(ajedrezUPV
        VERSION 0.1.0
        DESCRIPTION "Artificial intelligence chess engine (UPV)"
        LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# [Q4] Enable strict compiler warnings.
add_compile_options(-Wall -Wextra)

# [PERF] Native arch + LTO for Release (P2) — 5-15% on chess engines.
option(AJEDREZ_NATIVE "Build with -march=native" ON)
if(AJEDREZ_NATIVE)
  add_compile_options(-march=native)
endif()
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
if(ipo_supported)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---------------------------------------------------------------------------
# Third-party dependencies
# ---------------------------------------------------------------------------
# nlohmann/json  : single-header library, vendored under third_party/json.hpp
# chess-library  : header-only library (Disservin), fetched on first configure

include(FetchContent)
FetchContent_Declare(
  chess-library
  GIT_REPOSITORY https://github.com/Disservin/chess-library
  GIT_TAG        53e6a84  # pinned for reproducible builds (was: master)
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(chess-library)

add_library(chess_lib INTERFACE)
target_include_directories(chess_lib INTERFACE ${chess-library_SOURCE_DIR}/include)

find_package(OpenMP REQUIRED)

# ---------------------------------------------------------------------------
# Chess evaluator library
# ---------------------------------------------------------------------------
add_library(ajedrez_engine STATIC
  src/engine/evaluation/general_evaluator.cpp
  src/engine/evaluation/opening_evaluator.cpp
  src/engine/evaluation/endgame_evaluator.cpp
)
target_include_directories(ajedrez_engine PUBLIC include third_party src)
target_link_libraries(ajedrez_engine PUBLIC chess_lib OpenMP::OpenMP_CXX)

# Optimized tree and search implementation. It reuses the evaluator
# implementation from the engine library.
add_library(ajedrez_engine_optimized STATIC
  src/engine/node_move_optimized.cpp
  src/engine/transposition_table.cpp
  src/engine/search.cpp
)
target_include_directories(ajedrez_engine_optimized PUBLIC include third_party src)
target_link_libraries(ajedrez_engine_optimized PUBLIC ajedrez_engine)

add_executable(alphabeta_optimized src/engine/alphabeta_optimized.cpp)
target_link_libraries(alphabeta_optimized PRIVATE ajedrez_engine_optimized)

# ---------------------------------------------------------------------------
# Neural network tools
# ---------------------------------------------------------------------------
set(NEURAL_TOOLS
  src/neural/square_rook_network.cpp
  src/tools/json_extractor.cpp
  src/tools/json_extractor_junior.cpp
)

foreach(src IN LISTS NEURAL_TOOLS)
  get_filename_component(target_name ${src} NAME_WE)
  add_executable(${target_name} ${src})
  target_include_directories(${target_name} PRIVATE include third_party src)
  target_link_libraries(${target_name} PRIVATE chess_lib OpenMP::OpenMP_CXX)
endforeach()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(BUILD_TESTING)
  add_executable(test_node_move test/test_node_move.cpp)
  target_include_directories(test_node_move PRIVATE include third_party src)
  target_link_libraries(test_node_move PRIVATE ajedrez_engine_optimized)
  add_test(NAME node_move_tests COMMAND test_node_move)

  add_executable(test_search_benchmark test/test_search_benchmark.cpp)
  target_include_directories(test_search_benchmark PRIVATE include third_party src)
  target_link_libraries(test_search_benchmark PRIVATE ajedrez_engine_optimized)
  add_test(NAME search_benchmark_tests COMMAND test_search_benchmark)

  add_executable(test_json_extractor test/test_json_extractor.cpp)
  target_include_directories(test_json_extractor PRIVATE include third_party src)
  target_link_libraries(test_json_extractor PRIVATE chess_lib OpenMP::OpenMP_CXX)
  add_test(NAME json_extractor_tests COMMAND test_json_extractor)

  add_executable(test_json_extractor_junior test/test_json_extractor_junior.cpp)
  target_include_directories(test_json_extractor_junior PRIVATE include third_party src)
  target_link_libraries(test_json_extractor_junior PRIVATE chess_lib OpenMP::OpenMP_CXX)
  add_test(NAME json_extractor_junior_tests COMMAND test_json_extractor_junior)

  add_executable(test_phase2 test/test_phase2.cpp)
  target_include_directories(test_phase2 PRIVATE include third_party src)
  target_link_libraries(test_phase2 PRIVATE ajedrez_engine_optimized)
  add_test(NAME phase2_tests COMMAND test_phase2)

  add_executable(test_e2e test/test_e2e.cpp)
  target_include_directories(test_e2e PRIVATE include third_party src)
  target_link_libraries(test_e2e PRIVATE ajedrez_engine_optimized)
  add_test(NAME e2e_tests COMMAND test_e2e)
endif()
