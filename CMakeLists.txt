cmake_minimum_required(VERSION 3.16)
project(ajedrezUPV
        VERSION 0.1.0
        DESCRIPTION "Artificial intelligence chess engine (UPV)"
        LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
  GIT_TAG        master
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(chess-library)

add_library(chess_lib INTERFACE)
target_include_directories(chess_lib INTERFACE ${chess-library_SOURCE_DIR}/include)

find_package(OpenMP REQUIRED)

# ---------------------------------------------------------------------------
# Chess engine library
# ---------------------------------------------------------------------------
add_library(ajedrez_engine STATIC
  src/engine/node_move.cpp
  src/engine/evaluation/general_evaluator.cpp
  src/engine/evaluation/opening_evaluator.cpp
  src/engine/evaluation/endgame_evaluator.cpp
)
target_include_directories(ajedrez_engine PUBLIC include third_party src)
target_link_libraries(ajedrez_engine PUBLIC chess_lib OpenMP::OpenMP_CXX)

add_executable(alphabeta_bot src/engine/alphabeta_bot.cpp)
target_link_libraries(alphabeta_bot PRIVATE ajedrez_engine)

add_executable(minimax_bot src/engine/minimax_bot.cpp)
target_link_libraries(minimax_bot PRIVATE ajedrez_engine)

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
  target_link_libraries(test_node_move PRIVATE ajedrez_engine)
  add_test(NAME node_move_tests COMMAND test_node_move)

  add_executable(test_search_benchmark test/test_search_benchmark.cpp)
  target_include_directories(test_search_benchmark PRIVATE include third_party src)
  target_link_libraries(test_search_benchmark PRIVATE ajedrez_engine)
  add_test(NAME search_benchmark_tests COMMAND test_search_benchmark)

  add_executable(test_json_extractor test/test_json_extractor.cpp)
  target_include_directories(test_json_extractor PRIVATE include third_party src)
  target_link_libraries(test_json_extractor PRIVATE chess_lib OpenMP::OpenMP_CXX)
  add_test(NAME json_extractor_tests COMMAND test_json_extractor)

  add_executable(test_json_extractor_junior test/test_json_extractor_junior.cpp)
  target_include_directories(test_json_extractor_junior PRIVATE include third_party src)
  target_link_libraries(test_json_extractor_junior PRIVATE chess_lib OpenMP::OpenMP_CXX)
  add_test(NAME json_extractor_junior_tests COMMAND test_json_extractor_junior)
endif()
