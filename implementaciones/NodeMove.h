#ifndef NODEMOVE_H
#define NODEMOVE_H

#include "chess.hpp"
#include "GeneralEvaluator.h"
#include <array>
#include <memory>
#include <thread>
#include <mutex>

namespace chess {

constexpr int MAX_DEPTH = 3;
constexpr int MAX_BRANCH = 100;

class NodeMove {
private:
    //datos importantes
    int current_depth_;
    NodeMove* parent_;
    Move last_move_;
    float eval_;
    
    //hijos
    std::array<std::unique_ptr<NodeMove>, MAX_BRANCH> children_;
    size_t child_count_ = 0;

public:
    // Constructores y destructor
    NodeMove(Board *board, NodeMove* parent = nullptr);
    ~NodeMove() = default;
    
    // 
    void addChild(Board *board, Move move);
    
    // Funciones varias para depuracion
    void printEvaluationsOfChildren() const;
    void printBoard(Board &board) const;
    int evaluateBoard() const;
    float minimax(GeneralEvaluator* evaluator, Board *board, Color root_color);
    float alphaBeta(GeneralEvaluator* evaluator, float *alpha, float *beta, Color root_color, Board* board, std::mutex* alphaBetaMutex);
    chess::Move getBestMove(float best_score) const;
    
    // Getters varios
    float getEval() const { 
        return eval_; 
    }
    size_t getChildCount() const { 
        return child_count_; 
    }

    NodeMove* getChild(size_t index) const { 
        return (index < child_count_) ? children_[index].get() : nullptr; 
    }
    NodeMove* getChildByMove(const Move& move);
    void rebuildUntilDepth(Board* board);

    chess::Move getLastMove() const { 
        return last_move_; 
    }
    int getCurrentDepth() const { 
        return current_depth_; 
    }

    // si copias un unique ptr puede hacer pum
    NodeMove(const NodeMove&) = delete;
    NodeMove& operator=(const NodeMove&) = delete;
    
    // 
    NodeMove(NodeMove&&) = default;
    NodeMove& operator=(NodeMove&&) = default;
};

} // namespace chess
#endif // NODEMOVE_H