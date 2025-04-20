#include "NodeMove.h"
#include <omp.h>
#include <iostream>

using namespace chess;


NodeMove::NodeMove(Board board, NodeMove *parent) : board(board), parent(parent) {
    if (parent == nullptr) {
        currentDepth = 0;
    } else {
        currentDepth = parent->currentDepth + 1;
    }

    if (currentDepth < MAX_DEPTH) {
        Movelist legalMoves;
        movegen::legalmoves(legalMoves, board);

        for (const Move& move : legalMoves) {
            Board newBoard = board;          // Crear copia del tablero actual
            newBoard.makeMove(move);         // Aplicar el movimiento
            NodeMove* child = new NodeMove(newBoard, this); // Pasar el nuevo tablero al hijo
            child->lastMove = move;
            addChild(child);
        }
    }
}


NodeMove::~NodeMove() {
    for (int i = 0; i < MAX_BRANCH; ++i) {
        delete childs[i];
    }
}

void NodeMove::addChild(NodeMove *child) {
    if (childIndex >= MAX_BRANCH) {
        std::cerr << "Error: Maximum number of child nodes reached." << std::endl;
        return;
    }
    this->childs[childIndex] = child;
    childIndex++;
}
void NodeMove::printBoard() {
    std::cout << "  +-----------------+" << std::endl;
    for (int rank = 7; rank >= 0; --rank) { 
        std::cout << rank + 1 << " | "; 
        for (int row = 0; row < 8; ++row) { 
            chess::Square square(static_cast<chess::File::underlying>(row), static_cast<chess::Rank::underlying>(rank));
            chess::Piece piece = board.at(square);
            std::cout << static_cast<std::string>(piece) << " "; 
        }
        std::cout << "|" << std::endl;
    }
    std::cout << "  +-----------------+" << std::endl;
    std::cout << "    a b c d e f g h" << std::endl;
}
int NodeMove::evaluateBoard() {
    int score = 0;
    GeneralEvaluator* evaluator = new GeneralEvaluator();
    score = evaluator->evaluate(&board, board.sideToMove());
    delete evaluator;
    return score;
}
float NodeMove::minimax(GeneralEvaluator *evaluator, Color rootColor) {

    auto [reason, result] = board.isGameOver();
    if (result != chess::GameResult::NONE) {
        if (result == chess::GameResult::WIN) {
            return (board.sideToMove() == chess::Color::WHITE) ? -10000 : 10000;
        } else if (result == chess::GameResult::DRAW) {
            return 0;
        }
    }


    if (currentDepth == MAX_DEPTH) {
        eval = evaluator->evaluate(&board, rootColor);
        return eval;
    }


    if (this->board.sideToMove() == chess::Color::WHITE) {
        float bestValue = -99999;
        for (int i = 0; i < childIndex; ++i) {
            if (childs[i] != nullptr) {
                float value = childs[i]->minimax(evaluator, rootColor); 
                bestValue = std::max(bestValue, value);
            }
        }
        eval = bestValue;
        return bestValue;
    }

    else {
        float bestValue = 99999;
        for (int i = 0; i < childIndex; ++i) {
            if (childs[i] != nullptr) {
                float value = childs[i]->minimax(evaluator, rootColor); 
                bestValue = std::min(bestValue, value);
            }
        }
        eval = bestValue;
        return bestValue;
    }
}
chess::Move NodeMove::getBestMove(float bestScore) {
    chess::Move bestMove;
    for (int i = 0; i < MAX_BRANCH; ++i) {
        NodeMove* child = this->childs[i];
        if (child != nullptr && child->eval == bestScore) {
            bestMove = child->lastMove; // Movimiento que llevó al mejor nodo
            break;
        }
    }
    return bestMove;
}
void NodeMove::printEvaluationsOfChildren() const {
    std::cout << "Evaluaciones de los movimientos generados desde este nodo:" << std::endl;

    for (int i = 0; i < childIndex; ++i) {
        if (childs[i] != nullptr) {
            std::cout << "Movimiento: " << childs[i]->lastMove
                      << " | Evaluación: " << childs[i]->eval << std::endl;
        }
    }
}
void NodeMove::printBoardsAndEvaluationsOfChildren() const {
    std::cout << "Tableros generados por los movimientos de este nodo:" << std::endl;

    for (int i = 0; i < childIndex; ++i) {
        if (childs[i] != nullptr) {
            // Crear un evaluador para calcular la evaluación del hijo
            GeneralEvaluator evaluator;
            float evaluation = evaluator.evaluate(&childs[i]->board, childs[i]->board.sideToMove());

            // Imprimir el movimiento, evaluación y tablero
            std::cout << "Movimiento: " << childs[i]->lastMove
                      << " | Evaluación: " << evaluation << std::endl;
            childs[i]->printBoard();
            std::cout << "----------------------------------------" << std::endl;
        }
    }
}
float NodeMove::getEval() const {
    return eval;
}

int NodeMove::getChildIndex() const {
    return childIndex;
}

NodeMove* NodeMove::getChild(int index) const {
    if (index >= 0 && index < MAX_BRANCH) {
        return childs[index];
    }
    return nullptr;
}

chess::Move NodeMove::getLastMove() const {
    return lastMove;
}