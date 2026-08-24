#ifndef SEARCH_H
#define SEARCH_H

#include "chess.hpp"
#include "chess/evaluator.h"
#include "chess/transposition_table.h"
#include "chess/node_move_optimized.h" // for SearchResult

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>

namespace chess {

// [U2] Time control limits for a search.
struct SearchLimits {
    int      maxDepth     = MAX_DEPTH;
    uint64_t maxNodes     = 0;          // 0 = unlimited.
    int      movestogo    = 0;          // Moves to go until next time control.
    int      wtime_ms     = 0;          // White clock in milliseconds.
    int      btime_ms     = 0;          // Black clock in milliseconds.
    int      winc_ms      = 0;          // White increment per move.
    int      binc_ms      = 0;          // Black increment per move.
    int      movetime_ms  = 0;          // Exact time per move (if set).
};

// [B6] On-demand search: generates moves lazily instead of pre-building the
//      entire tree.  Combined with [B4] TT, [B5] iterative deepening.
// [PERF] Negamax formulation — scores always from the side-to-move's
//        perspective, which eliminates the rootColor branching per node and
//        enables standard PVS / null-move pruning.
class Search {
public:
    explicit Search(Evaluator* evaluator, size_t ttMB = 16);

    // Search the given position and return the best result.
    SearchResult search(Board& board, const SearchLimits& limits = {});

    // Stop the search asynchronously (for time control).
    void stop() { stopped_ = true; }

    // Statistics.
    uint64_t nodesSearched() const { return nodes_; }
    int      selDepth() const { return selDepth_; }
    uint64_t ttHits() const { return tt_.hits(); }
    uint64_t ttMisses() const { return tt_.misses(); }
    uint64_t ttStores() const { return tt_.stores(); }

private:
    Evaluator* evaluator_;
    TranspositionTable tt_;
    std::atomic<bool> stopped_{false};
    uint64_t nodes_ = 0;
    int selDepth_ = 0;

    // Killer moves: 2 per ply.
    static constexpr int MAX_SEARCH_PLY = 64;
    std::array<Move, 2> killers_[MAX_SEARCH_PLY] = {};

    // History heuristic: indexed by [from][to].
    static constexpr int HISTORY_MAX = 1 << 20;
    int history_[64][64] = {};

    // MVV-LVA piece values.
    static constexpr int PIECE_VALUES[7] = {0, 100, 300, 315, 500, 900, 0};

    // Time tracking.
    std::chrono::steady_clock::time_point startTime_;
    int allocatedMs_ = 0;

    // Core search functions (negamax, on-demand generation).
    float alphaBeta(Board& board, int depth, int ply,
                    float alpha, float beta, bool isPV, bool allowNull);

    float quiescence(Board& board, int depth, int ply,
                     float alpha, float beta);

    // Move ordering with TT move, killers, history (alloc-free).
    void orderMoves(Movelist& moves, Board& board, int ply, Move ttMove);

    // Time check — throttled to every 2048 nodes.
    bool shouldStop() const;

    // Helpers.
    bool isDraw(const Board& board) const;
    bool hasNonPawnMaterial(const Board& board, Color c) const;

    static bool isCapture(const Board& board, const Move& move);
    static bool isQuiet(const Board& board, const Move& move);
    static int mvvLvaScore(const Board& board, const Move& move);
};

} // namespace chess

#endif // SEARCH_H
