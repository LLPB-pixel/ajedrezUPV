#include "NodeMove.h"
#include <omp.h>
#include <iostream>
#include <algorithm>

using namespace chess;

NodeMove::NodeMove(Board board, NodeMove* parent) : 
    board_(std::move(board)),
    parent_(parent),
    current_depth_(parent ? parent->current_depth_ + 1 : 0) {
    
    if (current_depth_ < MAX_DEPTH) {
        Movelist moves;
        movegen::legalmoves(moves, board_);

        for (const Move& move : moves) {
            if (child_count_ >= MAX_BRANCH) break;
            
            Board new_board = board_;
            new_board.makeMove(move);
            children_[child_count_] = std::make_unique<NodeMove>(new_board, this);
            children_[child_count_]->last_move_ = move;
            child_count_++;
        }
    }
}

void NodeMove::addChild(Board board, Move move) {
    if (child_count_ < MAX_BRANCH) {
        children_[child_count_] = std::make_unique<NodeMove>(board, this);
        children_[child_count_]->last_move_ = move;
        child_count_++;
    }
}

void NodeMove::printBoard() const {
    std::cout << "  +-----------------+\n";
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << rank + 1 << " | ";
        for (int file = 0; file < 8; ++file) {
            Square square(static_cast<File>(file), static_cast<Rank>(rank));
            std::cout << static_cast<std::string>(board_.at(square)) << " ";
        }
        std::cout << "|\n";
    }
    std::cout << "  +-----------------+\n";
    std::cout << "    a b c d e f g h\n";
}

int NodeMove::evaluateBoard() const {
    GeneralEvaluator evaluator;
    return evaluator.evaluate(&board_, board_.sideToMove());
}

float NodeMove::minimax(GeneralEvaluator* evaluator, Color root_color) {
    if (auto [reason, result] = board_.isGameOver(); result != GameResult::NONE) {
        if (result == GameResult::WIN) {
            return (board_.sideToMove() == Color::WHITE) ? -10000.0f : 10000.0f;
        }
        return 0.0f; // DRAW
    }

    if (current_depth_ == MAX_DEPTH) {
        eval_ = evaluator->evaluate(&board_, root_color);
        return eval_;
    }

    if (child_count_ == 0) return 0.0f;

    if (board_.sideToMove() == Color::WHITE) {
        float best_value = -99999.0f;
        for (size_t i = 0; i < child_count_; ++i) {
            float value = children_[i]->minimax(evaluator, root_color);
            best_value = std::max(best_value, value);
        }
        eval_ = best_value;
        return best_value;
    } else {
        float best_value = 99999.0f;
        for (size_t i = 0; i < child_count_; ++i) {
            float value = children_[i]->minimax(evaluator, root_color);
            best_value = std::min(best_value, value);
        }
        eval_ = best_value;
        return best_value;
    }
}

chess::Move NodeMove::getBestMove(float best_score) const {
    for (size_t i = 0; i < child_count_; ++i) {
        if (children_[i]->eval_ == best_score) {
            return children_[i]->last_move_;
        }
    }
    return Move(); // Return null move if not found
}

void NodeMove::printEvaluationsOfChildren() const {
    std::cout << "Child move evaluations:\n";
    for (size_t i = 0; i < child_count_; ++i) {
        std::cout << "Move: " << children_[i]->last_move_
                  << " | Eval: " << children_[i]->eval_ << "\n";
    }
}

void NodeMove::printBoardsAndEvaluationsOfChildren() const {
    std::cout << "Child positions:\n";
    for (size_t i = 0; i < child_count_; ++i) {
        GeneralEvaluator evaluator;
        float evaluation = evaluator.evaluate(&children_[i]->board_, children_[i]->board_.sideToMove());
        
        std::cout << "Move: " << children_[i]->last_move_
                  << " | Eval: " << evaluation << "\n";
        children_[i]->printBoard();
        std::cout << "----------------------------------------\n";
    }
}
