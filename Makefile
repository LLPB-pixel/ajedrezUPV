# Makefile para ajedrezUPV
#
# Targets principales:
#   make all      -> compila motor + herramientas de red neuronal
#   make engine   -> compila los dos bots (alphabeta_bot, minimax_bot)
#   make neural   -> compila las herramientas de red neuronal
#   make test     -> compila y ejecuta los tests del motor y de tools
#   make setup    -> descarga chess.hpp de la libreria disservin/chess-library
#   make clean    -> elimina binarios y objetos
#
# Alternativa: cmake -S . -B build && cmake --build build

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -fopenmp -pthread
CPPFLAGS += -Isrc -Iinclude -Ithird_party

CHESS_HPP_URL := https://raw.githubusercontent.com/Disservin/chess-library/master/include/chess.hpp

BIN_DIR := bin

ENGINE_OBJS := src/engine/node_move.o \
               src/engine/evaluation/general_evaluator.o \
               src/engine/evaluation/opening_evaluator.o \
               src/engine/evaluation/endgame_evaluator.o

ENGINE_BINS := $(addprefix $(BIN_DIR)/, alphabeta_bot minimax_bot)
NEURAL_BINS := $(addprefix $(BIN_DIR)/, square_rook_network json_extractor json_extractor_junior)
TEST_BINS := $(addprefix $(BIN_DIR)/, test_node_move test_json_extractor test_json_extractor_junior)

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

test: setup $(TEST_BINS)
	@$(BIN_DIR)/test_node_move
	@$(BIN_DIR)/test_json_extractor
	@$(BIN_DIR)/test_json_extractor_junior

$(BIN_DIR)/alphabeta_bot: src/engine/alphabeta_bot.cpp $(ENGINE_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@

$(BIN_DIR)/minimax_bot: src/engine/minimax_bot.cpp $(ENGINE_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@

$(BIN_DIR)/test_node_move: test/test_node_move.cpp $(ENGINE_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@

$(BIN_DIR)/test_json_extractor: test/test_json_extractor.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@

$(BIN_DIR)/test_json_extractor_junior: test/test_json_extractor_junior.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@

$(BIN_DIR)/square_rook_network: src/neural/square_rook_network.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@

$(BIN_DIR)/json_extractor: src/tools/json_extractor.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@

$(BIN_DIR)/json_extractor_junior: src/tools/json_extractor_junior.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

src/engine/%.o: src/engine/%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

src/engine/evaluation/%.o: src/engine/evaluation/%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -rf $(BIN_DIR)
	find src -name '*.o' -delete
