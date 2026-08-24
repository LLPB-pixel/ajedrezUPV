#ifndef NODE_MOVE_OPTIMIZED_H
#define NODE_MOVE_OPTIMIZED_H

#include "chess.hpp"
#include "chess/general_evaluator.h"

#include <array>
#include <cstdint>
#include <memory_resource>
#include <mutex>
#include <vector>

namespace chess {

constexpr int MAX_DEPTH = 3;
constexpr int MAX_BRANCH = 100;

// [B8] Structured search result returned by the search — replaces the
// fragile getBestMove(best_score) float comparison.
struct SearchResult {
    Move  move;
    float score;
    int   depth;
};

// A compact, allocation-friendly tree used by alphabeta_optimized.
// TreeContext owns the allocation arena; nodes do not own a context or storage
// pointer beyond the allocator already held by their child vector.
class NodeMoveOptimized {
public:
    // The context must outlive every NodeMoveOptimized created with it.
    struct TreeContext {
        std::pmr::unsynchronized_pool_resource pool;
    };

private:
    static constexpr float NEGATIVE_INFINITY = -eval::INFINITY;
    static constexpr float POSITIVE_INFINITY =  eval::INFINITY;

    Move last_move_;
    float eval_ = 0.0f;
    std::pmr::vector<NodeMoveOptimized> children_;

    NodeMoveOptimized(Board* board, int depth, Move last_move,
                      std::pmr::memory_resource* resource);

    void buildChildren(Board* board, int depth,
                       std::pmr::memory_resource* resource);
    float minimaxImpl(GeneralEvaluator* evaluator, Board& board,
                      Color root_color, int depth_limit, int searched_depth);
    float alphaBetaImpl(GeneralEvaluator* evaluator, Board& board,
                        Color root_color, int depth_limit, float alpha,
                        float beta, int searched_depth);

    // [B2] Quiescence search: extend leaf nodes only with captures/promotions
    // to avoid the horizon effect.
    float quiescence(GeneralEvaluator* evaluator, Board& board,
                     Color root_color, float alpha, float beta,
                     int qdepth, int max_qdepth);

    void printTreeImpl(int indent, int depth) const;

    static bool isSearchTerminal(const Board& board) noexcept;

    // [B1] Detect checkmate and stalemate directly.
    static bool isCheckmate(const Board& board);
    static bool isStalemate(const Board& board);

    // [B3] Move ordering helpers
    static constexpr int PIECE_VALUES[7] = {0, 100, 300, 315, 500, 900, 0}; // indexed by PieceType

    // MVV-LVA score for a capture: higher = more valuable victim, cheaper attacker.
    static int mvvLvaScore(const Board& board, const Move& move);

    // Killer moves: two per ply — moves that caused a beta cutoff at the same depth.
    static constexpr int MAX_PLY = 64;
    std::array<Move, 2> killers_[MAX_PLY] = {};

    // History heuristic: indexed by [from][to], counts cutoffs.
    int history_[64][64] = {};

    // [B3] Sort a movelist in-place for better pruning.
    void orderMoves(Movelist& moves, Board& board, int ply);

public:
    explicit NodeMoveOptimized(TreeContext& context, Board* board);
    ~NodeMoveOptimized() = default;

    NodeMoveOptimized(const NodeMoveOptimized&) = delete;
    NodeMoveOptimized& operator=(const NodeMoveOptimized&) = delete;
    NodeMoveOptimized(NodeMoveOptimized&&) noexcept = default;
    NodeMoveOptimized& operator=(NodeMoveOptimized&&) noexcept = default;

    void addChild(Board* board, Move move, int current_depth);
    void rebuildUntilDepth(Board* board, int current_depth);

    float minimax(GeneralEvaluator* evaluator, Board* board, Color root_color,
                  int depth_limit = MAX_DEPTH);

    // This overload is the hot-path API.
    SearchResult alphaBeta(GeneralEvaluator* evaluator, Color root_color,
                           Board* board, int depth_limit = MAX_DEPTH);

    // Compatibility overload kept for tests using the original signature.
    float alphaBeta(GeneralEvaluator* evaluator, float* alpha, float* beta,
                    Color root_color, Board* board, std::mutex* alphaBetaMutex,
                    int depth_limit = MAX_DEPTH);

    // [B8] Legacy helper — prefer alphaBeta() returning SearchResult.
    Move getBestMove(float best_score) const;

    NodeMoveOptimized* getChild(size_t index);
    const NodeMoveOptimized* getChild(size_t index) const;
    NodeMoveOptimized* getChildByMove(const Move& move);
    const NodeMoveOptimized* getChildByMove(const Move& move) const;

    size_t getChildCount() const noexcept { return children_.size(); }
    float getEval() const noexcept { return eval_; }
    Move getLastMove() const noexcept { return last_move_; }

    void printTree(int indent = 0) const;
    void printEvaluationsOfChildren() const;
    void printBoard(Board& board) const;
};

} // namespace chess

#endif // NODE_MOVE_OPTIMIZED_H
