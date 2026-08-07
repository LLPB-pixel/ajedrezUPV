#ifndef NODE_MOVE_OPTIMIZED_H
#define NODE_MOVE_OPTIMIZED_H

#include "chess.hpp"
#include "chess/general_evaluator.h"

#include <memory_resource>
#include <mutex>
#include <vector>

namespace chess {

constexpr int MAX_DEPTH = 3;
constexpr int MAX_BRANCH = 100;

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
    static constexpr float NEGATIVE_INFINITY = -99999.0f;
    static constexpr float POSITIVE_INFINITY = 99999.0f;

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

    void printTreeImpl(int indent, int depth) const;

    static bool isSearchTerminal(const Board& board) noexcept;

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

    // This overload is the hot-path API: alpha and beta stay in registers and
    // no synchronization object is required by the recursive search.
    float alphaBeta(GeneralEvaluator* evaluator, Color root_color,
                    Board* board, int depth_limit = MAX_DEPTH);

    // Compatibility overload for callers using the original signature. The
    // mutex is deliberately ignored because this search is sequential.
    float alphaBeta(GeneralEvaluator* evaluator, float* alpha, float* beta,
                    Color root_color, Board* board, std::mutex* alphaBetaMutex,
                    int depth_limit = MAX_DEPTH);

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
