#include "NodeMove.h"
#include <omp.h>
#include <iostream>
#include "classGeneralEvaluator.hpp"


NodeMove::NodeMove(Board board, NodeMove *parent) : board(board), parent(parent) {
    if(parent == nullptr){
        currentDepth = 0;
    }
    else{
        currentDepth = parent->currentDepth + 1;
    }
    if (currentDepth < MAX_DEPTH){
        Movelist legalMoves;
        movegen::legalmoves(legalMoves, board);
        #pragma omp parallel for
        for (const Move& move : legalMoves) {
            NodeMove *child = new NodeMove(board, this);
            child->board.makeMove(move); // Apply the move to the child node
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
    score = evaluator->evaluate(&board);
    delete evaluator;
    return score;
}
float NodeMove::minimax(GeneralEvaluator *evaluator) {

    auto [reason, result] = board.isGameOver();
    if (result != chess::GameResult::NONE) {
        if (result == chess::GameResult::WIN) {
            return (board.sideToMove() == chess::Color::WHITE) ? -10000 : 10000;
        } else if (result == chess::GameResult::DRAW) {
            return 0;
        }
    }


    if (currentDepth == MAX_DEPTH) {
        eval = evaluator->evaluate(&board, board.sideToMove());
        return evaluator->evaluate(&board, board.sideToMove());
    }


    if (this->board.sideToMove() == chess::Color::WHITE) {
        float bestValue = -99999;
        for (int i = 0; i < childIndex; ++i) {
            if (childs[i] != nullptr) {
                float value = childs[i]->minimax(evaluator); 
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
                float value = childs[i]->minimax(evaluator); 
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

