#ifndef NODEMOVE_H
#define NODEMOVE_H

#include "chess.hpp"
#include "GeneralEvaluator.h"
#include <array>
#include <memory>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <random>

namespace chess {

constexpr int MAX_DEPTH = 3;
constexpr int MAX_BRANCH = 100;

class Zobrist {
private:
    static std::array<std::array<uint64_t, 64>, 12> piece_keys_;
    static std::array<uint64_t, 16> castling_keys_;
    static std::array<uint64_t, 8> en_passant_keys_;
    static uint64_t side_to_move_key_;

public:
    static void initialize(bool deterministic = false);
    static uint64_t getPieceKey(Piece piece, Square square);
    static uint64_t getCastlingKey(int castling);
    static uint64_t getEnPassantKey(File file);
    static uint64_t getSideToMoveKey();
    static uint64_t computeBoardKey(const Board& board);
    static uint64_t updateKeyForMove(const Board& board, const Move& move, uint64_t current_key);
};

struct Transposition {
    uint64_t zobrist_key;
    float eval;
    int depth;
    Move best_move;
};

class NodeMove {
private:
    int current_depth_;
    NodeMove* parent_;
    Move last_move_;
    float eval_;
    uint64_t zobrist_key_;
    std::array<std::unique_ptr<NodeMove>, MAX_BRANCH> children_;
    size_t child_count_ = 0;

    static std::unordered_map<uint64_t, Transposition> transposition_table_;
    static std::mutex tt_mutex_;

public:
    NodeMove(Board *board, NodeMove* parent = nullptr);
    ~NodeMove() = default;

    void addChild(Board *board, Move move);
    void printEvaluationsOfChildren() const;
    void printBoard(Board &board) const;
    int evaluateBoard() const;
    float minimax(GeneralEvaluator* evaluator, Board *board, Color root_color);
    float alphaBeta(GeneralEvaluator* evaluator, float *alpha, float *beta, Color root_color, Board* board, std::mutex* alphaBetaMutex);
    chess::Move getBestMove(float best_score) const;

    float getEval() const { return eval_; }
    size_t getChildCount() const { return child_count_; }
    NodeMove* getChild(size_t index) const { return (index < child_count_) ? children_[index].get() : nullptr; }
    NodeMove* getChildByMove(const Move& move);
    void printTree(int indent = 0) const;
    void rebuildUntilDepth(Board* board);
    chess::Move getLastMove() const { return last_move_; }
    int getCurrentDepth() const { return current_depth_; }
    uint64_t getZobristKey() const { return zobrist_key_; }

    NodeMove(const NodeMove&) = delete;
    NodeMove& operator=(const NodeMove&) = delete;
    NodeMove(NodeMove&&) = default;
    NodeMove& operator=(NodeMove&&) = default;
};

} // namespace chess
#endif // NODEMOVE_H
