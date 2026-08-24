# Makefile para ajedrezUPV
#
# Targets principales:
#   make all      -> compila motor + herramientas de red neuronal
#   make engine   -> compila el bot alphabeta_optimized
#   make neural   -> compila las herramientas de red neuronal
#   make test     -> compila y ejecuta los tests del motor y de tools
#   make setup    -> descarga chess.hpp de la libreria disservin/chess-library
#   make clean    -> elimina binarios y objetos
#
# Alternativa: cmake -S . -B build && cmake --build build

CXX      ?= g++
# [PERF] -O3 + native arch + LTO — typically 5-15% for chess engines (P2).
CXXFLAGS ?= -std=c++17 -O3 -march=native -flto -fopenmp -pthread -Wall -Wextra
CPPFLAGS += -Isrc -Iinclude -Ithird_party
LDFLAGS  += -flto

CHESS_HPP_URL := https://raw.githubusercontent.com/Disservin/chess-library/master/include/chess.hpp

BIN_DIR := bin

ENGINE_OBJS := src/engine/evaluation/general_evaluator.o \
               src/engine/evaluation/opening_evaluator.o \
               src/engine/evaluation/endgame_evaluator.o

ENGINE_OPT_OBJ := src/engine/node_move_optimized.o \
                  src/engine/transposition_table.o \
                  src/engine/search.o

ENGINE_BINS := $(addprefix $(BIN_DIR)/, alphabeta_optimized)
NEURAL_BINS := $(addprefix $(BIN_DIR)/, square_rook_network json_extractor json_extractor_junior)
TEST_BINS := $(addprefix $(BIN_DIR)/, test_node_move test_search_benchmark test_json_extractor test_json_extractor_junior test_phase2 test_e2e)

.PHONY: all setup engine neural test clean

all: engine neural

setup:
	@mkdir -p third_party
	@if [ ! -f third_party/chess.hpp ]; then \
		curl -fsSL $(CHESS_HPP_URL) -o third_party/chess.hpp && \
		echo "chess.hpp descargada en third_party/"; \
	else \
		echo "third_party/chess.hpp ya existe"; \
	fi

engine: setup $(ENGINE_BINS)

neural: setup $(NEURAL_BINS)

test: setup $(ENGINE_BINS) $(TEST_BINS)
	@$(BIN_DIR)/test_node_move
	@$(BIN_DIR)/test_search_benchmark
	@$(BIN_DIR)/test_json_extractor
	@$(BIN_DIR)/test_json_extractor_junior
	@$(BIN_DIR)/test_phase2
	@$(BIN_DIR)/test_e2e

$(BIN_DIR)/alphabeta_optimized: src/engine/alphabeta_optimized.cpp $(ENGINE_OPT_OBJ) $(ENGINE_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

$(BIN_DIR)/test_node_move: test/test_node_move.cpp $(ENGINE_OBJS) $(ENGINE_OPT_OBJ) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

$(BIN_DIR)/test_search_benchmark: test/test_search_benchmark.cpp $(ENGINE_OBJS) $(ENGINE_OPT_OBJ) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

$(BIN_DIR)/test_json_extractor: test/test_json_extractor.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

$(BIN_DIR)/test_json_extractor_junior: test/test_json_extractor_junior.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

$(BIN_DIR)/test_phase2: test/test_phase2.cpp $(ENGINE_OBJS) $(ENGINE_OPT_OBJ) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

$(BIN_DIR)/test_e2e: test/test_e2e.cpp $(ENGINE_OBJS) $(ENGINE_OPT_OBJ) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

$(BIN_DIR)/square_rook_network: src/neural/square_rook_network.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

$(BIN_DIR)/json_extractor: src/tools/json_extractor.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

$(BIN_DIR)/json_extractor_junior: src/tools/json_extractor_junior.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

src/engine/%.o: src/engine/%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

src/engine/evaluation/%.o: src/engine/evaluation/%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -rf $(BIN_DIR)
	find src -name '*.o' -delete
