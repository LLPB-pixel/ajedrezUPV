#include "NodeMove.h"
#include <omp.h>
#include <iostream>
#include <algorithm>

using namespace chess;

NodeMove::NodeMove(Board *board, NodeMove* parent) : 
    parent_(parent),
    current_depth_(parent ? parent->current_depth_ + 1 : 0) {
    
    if (current_depth_ < MAX_DEPTH) {
        Movelist moves;
        movegen::legalmoves(moves, *board);

        for (const Move& move : moves) {
            if (child_count_ >= MAX_BRANCH) break;
            
            
            board->makeMove(move);
            children_[child_count_] = std::make_unique<NodeMove>(board, this);
            board->unmakeMove(move);
            children_[child_count_]->last_move_ = move;
            child_count_++;
        }
    }
}

void NodeMove::addChild(Board *board, Move move) {
    if (child_count_ < MAX_BRANCH) {
        children_[child_count_] = std::make_unique<NodeMove>(board, this);
        children_[child_count_]->last_move_ = move;
        child_count_++;
    }
}

void NodeMove::printBoard(Board &board) const {
    std::cout << "  +-----------------+\n";
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << rank + 1 << " | ";
        for (int file = 0; file < 8; ++file) {
            Square square(static_cast<File>(file), static_cast<Rank>(rank));
            std::cout << static_cast<std::string>(board.at(square)) << " ";
        }
        std::cout << "|\n";
    }
    std::cout << "  +-----------------+\n";
    std::cout << "    a b c d e f g h\n";
}



float NodeMove::minimax(GeneralEvaluator* evaluator, Board *board, Color root_color) {
    auto [reason, result] = board->isGameOver();
    if(result != GameResult::NONE) {
        eval_ = evaluator.evaluate(board, root_color);
        return eval_;
    }


    if (current_depth_ == MAX_DEPTH) {
        eval_ = evaluator->evaluate(board, root_color);
        return eval_;
    }

    if (child_count_ == 0) return 0.0f;

    if (board->sideToMove() == Color::WHITE) {
        float best_value = -99999.0f;
        for (size_t i = 0; i < child_count_; ++i) {
            board->makeMove(children_[i]->last_move_);
            float value = children_[i]->minimax(evaluator, board, root_color);
            board->unmakeMove(children_[i]->last_move_);
            best_value = std::max(best_value, value);
        }
        eval_ = best_value;
        return best_value;
    } else {
        float best_value = 99999.0f;
        for (size_t i = 0; i < child_count_; ++i) {
            float value = children_[i]->minimax(evaluator, board, root_color);
            best_value = std::min(best_value, value);
        }
        eval_ = best_value;
        return best_value;
    }
}
float NodeMove::alphaBeta(GeneralEvaluator* evaluator, float alpha, float beta, Color root_color, Board *board) {
    auto [reason, result] = board->isGameOver();
    if(result != GameResult::NONE) {
        eval_ = evaluator.evaluate(board, root_color);
        return eval_;
    }

    if (current_depth_ == MAX_DEPTH) {
        eval_ = evaluator->evaluate(board, root_color);
        return eval_;
    }

    if (board->sideToMove() == Color::WHITE) {
        float bestValue = -99999.0f;
        for (size_t i = 0; i < child_count_; ++i) {
            board->makeMove(children_[i]->last_move_);
            float value = children_[i]->alphaBeta(evaluator, alpha, beta, root_color, board);
            board->unmakeMove(children_[i]->last_move_);
            bestValue = std::max(bestValue, value);
            alpha = std::max(alpha, bestValue);

            // Podar rama
            if (alpha >= beta) {
                break;
            }
        }
        eval_ = bestValue;
        return bestValue;
    } else {
        float bestValue = 99999.0f;
        for (size_t i = 0; i < child_count_; ++i) {
            board->makeMove(children_[i]->last_move_);
            float value = children_[i]->alphaBeta(evaluator, alpha, beta, root_color, board);
            board->unmakeMove(children_[i]->last_move_);
            bestValue = std::min(bestValue, value);
            beta = std::min(beta, bestValue);

            // Podar rama
            if (beta <= alpha) {
                break;
            }
        }
        eval_ = bestValue;
        return bestValue;
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


