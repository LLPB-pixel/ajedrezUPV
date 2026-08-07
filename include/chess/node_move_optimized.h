#ifndef NODE_MOVE_OPTIMIZED_H
#define NODE_MOVE_OPTIMIZED_H

#include "chess/node_move.h"

#include <memory>
#include <memory_resource>
#include <mutex>
#include <vector>

namespace chess {

// A compact, allocation-friendly tree used by alphabeta_optimized.
// The original NodeMove API is intentionally left untouched so both
// implementations can be benchmarked in the same executable.
class NodeMoveOptimized {
private:
    struct Storage {
        std::pmr::unsynchronized_pool_resource pool;
    };

    static constexpr float NEGATIVE_INFINITY = -99999.0f;
    static constexpr float POSITIVE_INFINITY = 99999.0f;

    // Only the root owns the storage. Every descendant points at it, so child
    // vectors use the same allocator without paying for a resource per node.
    std::unique_ptr<Storage> storage_owner_;
    Storage* storage_;

    int current_depth_;
    Move last_move_;
    float eval_ = 0.0f;
    std::pmr::vector<NodeMoveOptimized> children_;

    NodeMoveOptimized(Board* board, int depth, Move last_move,
                      Storage* storage);

    void buildChildren(Board* board);
    float minimaxImpl(GeneralEvaluator* evaluator, Board& board,
                      Color root_color, int depth_limit);
    float alphaBetaImpl(GeneralEvaluator* evaluator, Board& board,
                        Color root_color, int depth_limit, float alpha,
                        float beta);

    static bool isSearchTerminal(const Board& board) noexcept;

public:
    explicit NodeMoveOptimized(Board* board,
                               NodeMoveOptimized* parent = nullptr);
    ~NodeMoveOptimized() = default;

    NodeMoveOptimized(const NodeMoveOptimized&) = delete;
    NodeMoveOptimized& operator=(const NodeMoveOptimized&) = delete;
    NodeMoveOptimized(NodeMoveOptimized&&) noexcept = default;
    NodeMoveOptimized& operator=(NodeMoveOptimized&&) noexcept = default;

    void addChild(Board* board, Move move);
    void rebuildUntilDepth(Board* board);

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
    int getCurrentDepth() const noexcept { return current_depth_; }

    void printTree(int indent = 0) const;
    void printEvaluationsOfChildren() const;
    void printBoard(Board& board) const;
};

} // namespace chess

#endif // NODE_MOVE_OPTIMIZED_H
