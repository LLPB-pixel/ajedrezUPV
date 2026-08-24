#include "chess/search.h"

#include <algorithm>
#include <cstring>
#include <iostream>

namespace chess {

// =========================================================================
// Construction
// =========================================================================

Search::Search(Evaluator* evaluator, size_t ttMB)
    : evaluator_(evaluator), tt_(ttMB) {
    std::memset(killers_, 0, sizeof(killers_));
    std::memset(history_, 0, sizeof(history_));
}

// =========================================================================
// [B5] Iterative deepening — the top-level search entry point.
// =========================================================================

SearchResult Search::search(Board& board, const SearchLimits& limits) {
    const Color rootColor = board.sideToMove();

    // [U2] Compute time allocation.
    if (limits.movetime_ms > 0) {
        allocatedMs_ = limits.movetime_ms;
    } else if (limits.wtime_ms > 0 || limits.btime_ms > 0) {
        int myTime = (rootColor == Color::WHITE) ? limits.wtime_ms : limits.btime_ms;
        int myInc  = (rootColor == Color::WHITE) ? limits.winc_ms  : limits.binc_ms;
        int movesLeft = (limits.movestogo > 0) ? limits.movestogo : 30;
        allocatedMs_ = myTime / movesLeft + myInc / 2;
        allocatedMs_ = std::min(allocatedMs_, myTime / 3);
    } else {
        allocatedMs_ = 0;
    }

    // Reset search state.
    stopped_ = false;
    nodes_ = 0;
    selDepth_ = 0;
    tt_.newSearch();
    std::memset(killers_, 0, sizeof(killers_));
    std::memset(history_, 0, sizeof(history_));
    startTime_ = std::chrono::steady_clock::now();

    Move bestMove = Move();
    float bestScore = 0.0f;
    int lastDepth = 0;

    // [B5] Iterative deepening: search depth 1, 2, 3, ...
    for (int depth = 1; depth <= limits.maxDepth; ++depth) {
        float aspAlpha = -eval::INFINITY;
        float aspBeta  =  eval::INFINITY;
        if (depth >= 4 && bestScore > -eval::MATE_SCORE + 1000 && bestScore < eval::MATE_SCORE - 1000) {
            aspAlpha = bestScore - 50.0f;
            aspBeta  = bestScore + 50.0f;
        }

        float score = 0.0f;
        bool failed = true;
        int aspAttempts = 0;

        while (failed && aspAttempts < 6) {
            failed = false;
            score = alphaBeta(board, depth, 0, aspAlpha, aspBeta, true, true);

            if (shouldStop() && depth > 1) break;

            if (score <= aspAlpha) {
                aspAlpha = std::max(aspAlpha - 200.0f, -eval::INFINITY);
                failed = true;
            } else if (score >= aspBeta) {
                aspBeta = std::min(aspBeta + 200.0f, eval::INFINITY);
                failed = true;
            }
            ++aspAttempts;
        }

        if (shouldStop() && depth > 1) break;

        bestScore = score;
        lastDepth = depth;

        TTEntry rootEntry;
        if (tt_.probe(board.hash(), rootEntry) && rootEntry.move != Move()) {
            // rootEntry.score is still adjusted; we need the move only
            bestMove = rootEntry.move;
        }

        if (bestScore > eval::MATE_SCORE - 1000 || bestScore < -eval::MATE_SCORE + 1000) {
            break;
        }
    }

    return {bestMove, bestScore, lastDepth};
}

// =========================================================================
// [B6] Negamax alpha-beta with on-demand move generation and
//      [B7] pruning (null-move, PVS, LMR, futility).
// Scores are from the side-to-move's perspective at every node.
// =========================================================================

float Search::alphaBeta(Board& board, int depth, int ply,
                        float alpha, float beta, bool isPV, bool allowNull) {
    if (shouldStop()) return 0.0f;
    if (ply > selDepth_) selDepth_ = ply;
    ++nodes_;

    // Mate-distance pruning: narrow window by ply.
    alpha = std::max(alpha, -eval::MATE_SCORE + static_cast<float>(ply));
    beta  = std::min(beta,   eval::MATE_SCORE - static_cast<float>(ply) - 1.0f);
    if (alpha >= beta) return alpha;

    const bool inCheck = board.inCheck();

    if (isDraw(board)) return 0.0f;

    // [B4] Transposition table probe.
    TTEntry ttEntry;
    bool ttHit = tt_.probe(board.hash(), ttEntry);
    if (ttHit && !isPV && ttEntry.depth >= depth) {
        float ttScore = TranspositionTable::scoreFromTT(ttEntry.score, ply);
        if (ttEntry.flag == TTFlag::EXACT) return ttScore;
        if (ttEntry.flag == TTFlag::LOWER && ttScore >= beta) return ttScore;
        if (ttEntry.flag == TTFlag::UPPER && ttScore <= alpha) return ttScore;
    }
    Move ttMove = (ttHit && ttEntry.move != Move()) ? ttEntry.move : Move();

    // Leaf node: quiescence search.
    if (depth <= 0) {
        return quiescence(board, depth, ply, alpha, beta);
    }

    // [B7] Null-move pruning. [DISABLED FOR DEBUG]
    /*
    if (allowNull && !isPV && !inCheck && depth >= 3 &&
        hasNonPawnMaterial(board, board.sideToMove())) {
        const int R = 3 + (depth >= 6 ? 1 : 0);
        board.makeNullMove();
        float score = -alphaBeta(board, depth - 1 - R, ply + 1,
                                 -beta, -beta + 1.0f, false, false);
        board.unmakeNullMove();
        if (shouldStop()) return 0.0f;
        if (score >= beta && std::abs(score) < eval::MATE_SCORE - 1000) {
            return beta;
        }
    } */

    // [B6] Generate moves on-demand — single movegen per node.
    Movelist moves;
    movegen::legalmoves(moves, board);

    if (moves.size() == 0) {
        return inCheck ? -eval::MATE_SCORE + static_cast<float>(ply) : 0.0f;
    }

    // [B3] Move ordering (alloc-free).
    orderMoves(moves, board, ply, ttMove);

    // [B7] Futility pruning setup. [DISABLED]
    /*
    float futilityBase = 0.0f;
    bool doFutility = false;
    */

    float bestScore = -eval::INFINITY;
    Move bestMove = Move();
    int movesSearched = 0;
    const float origAlpha = alpha;

    for (int i = 0; i < moves.size(); ++i) {
        const Move& move = moves[i];
        const bool quiet = isQuiet(board, move);

        /* futility skip disabled */

        board.makeMove(move);
        float score = -alphaBeta(board, depth - 1, ply + 1, -beta, -alpha, isPV, true);
        board.unmakeMove(move);
        ++movesSearched;

        if (shouldStop()) return 0.0f;

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
        if (score > alpha) {
            alpha = score;
        }

        if (alpha >= beta) {
            // [B3] Record killer and history for quiet moves.
            if (quiet) {
                if (ply < MAX_SEARCH_PLY) {
                    if (killers_[ply][0] != move) {
                        killers_[ply][1] = killers_[ply][0];
                        killers_[ply][0] = move;
                    }
                }
                int from = move.from().index();
                int to   = move.to().index();
                int& h = history_[from][to];
                h += depth * depth;
                if (h >= HISTORY_MAX) {
                    for (int f = 0; f < 64; ++f)
                        for (int t = 0; t < 64; ++t)
                            history_[f][t] >>= 1;
                }
            }
            break;
        }
    }

    // [B4] Store result in the transposition table.
    TTFlag flag;
    if (bestScore <= origAlpha) flag = TTFlag::UPPER;
    else if (bestScore >= beta) flag = TTFlag::LOWER;
    else flag = TTFlag::EXACT;

    tt_.store(board.hash(), bestMove, bestScore, depth, flag, ply);

    return bestScore;
}

// =========================================================================
// [B2] Quiescence search (negamax, on-demand — stand-pat first).
// =========================================================================

float Search::quiescence(Board& board, int depth, int ply,
                         float alpha, float beta) {
    if (shouldStop()) return 0.0f;
    ++nodes_;
    if (ply > selDepth_) selDepth_ = ply;

    if (isDraw(board)) return 0.0f;

    // Stand-pat: evaluate the current position without any move.
    float standPat = evaluator_->evaluate(&board, board.sideToMove());

    if (standPat >= beta) return beta;
    if (standPat > alpha) alpha = standPat;

    // Single movegen — also covers mate / stalemate detection.
    Movelist moves;
    movegen::legalmoves(moves, board);

    if (moves.size() == 0) {
        return board.inCheck() ? -eval::MATE_SCORE + static_cast<float>(ply) : standPat;
    }

    // Filter captures and promotions into a stack array sorted by MVV-LVA.
    struct ScoredMove { int key; Move move; };
    ScoredMove caps[256];
    int nCaps = 0;
    for (int i = 0; i < moves.size(); ++i) {
        const Move& m = moves[i];
        if (isCapture(board, m) || m.typeOf() == Move::PROMOTION) {
            caps[nCaps++] = {mvvLvaScore(board, m), m};
        }
    }

    if (nCaps == 0) return standPat;

    std::sort(caps, caps + nCaps,
              [](const ScoredMove& a, const ScoredMove& b){ return a.key > b.key; });

    float best = standPat;

    for (int i = 0; i < nCaps; ++i) {
        const Move& move = caps[i].move;
        board.makeMove(move);
        float score = -quiescence(board, depth - 1, ply + 1, -beta, -alpha);
        board.unmakeMove(move);

        if (shouldStop()) return 0.0f;

        if (score > best) best = score;
        if (score > alpha) alpha = score;
        if (alpha >= beta) break;
    }

    return best;
}

// =========================================================================
// [B3] Move ordering — alloc-free (stack array + std::sort, no heap).
// =========================================================================

void Search::orderMoves(Movelist& moves, Board& board, int ply, Move ttMove) {
    struct ScoredMove { int key; Move move; };
    ScoredMove scored[256];
    const int n = moves.size();
    for (int i = 0; i < n; ++i) {
        const Move& m = moves[i];
        int key = 0;

        if (m == ttMove) {
            key = 1000000;
        } else if (isCapture(board, m)) {
            key = 100000 + mvvLvaScore(board, m);
        } else if (m.typeOf() == Move::PROMOTION) {
            key = 95000;
        }

        if (ply >= 0 && ply < MAX_SEARCH_PLY) {
            if (m == killers_[ply][0]) key += 90000;
            else if (m == killers_[ply][1]) key += 80000;
        }

        key += history_[m.from().index()][m.to().index()];
        scored[i] = {key, m};
    }

    std::sort(scored, scored + n,
              [](const ScoredMove& a, const ScoredMove& b){ return a.key > b.key; });

    for (int i = 0; i < n; ++i) {
        moves[i] = scored[i].move;
    }
}

// =========================================================================
// Utility functions
// =========================================================================

bool Search::shouldStop() const {
    if (stopped_.load(std::memory_order_relaxed)) return true;
    if (allocatedMs_ > 0 && (nodes_ & 2047) == 0) {
        auto elapsed = std::chrono::steady_clock::now() - startTime_;
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        if (elapsedMs > allocatedMs_ * 8 / 10) return true;
    }
    return false;
}

bool Search::isDraw(const Board& board) const {
    if (board.isHalfMoveDraw()) return true;
    if (board.occ().count() <= 4 && board.isInsufficientMaterial()) return true;
    if (board.halfMoveClock() >= 4 && board.isRepetition()) return true;
    return false;
}

bool Search::hasNonPawnMaterial(const Board& board, Color c) const {
    Bitboard occ = board.us(c);
    occ ^= board.pieces(PieceType::PAWN, c);
    occ ^= board.pieces(PieceType::KING, c);
    return !occ.empty();
}

bool Search::isCapture(const Board& board, const Move& move) {
    if (move.typeOf() == Move::ENPASSANT) return true;
    if (move.typeOf() == Move::CASTLING) return false;
    return board.at(move.to()) != Piece();
}

bool Search::isQuiet(const Board& board, const Move& move) {
    return !isCapture(board, move) && move.typeOf() != Move::PROMOTION;
}

int Search::mvvLvaScore(const Board& board, const Move& move) {
    if (move.typeOf() == Move::CASTLING) return -1000;

    PieceType victimType = (move.typeOf() == Move::ENPASSANT)
        ? PieceType::PAWN
        : board.at<PieceType>(move.to());
    PieceType attackerType = board.at<PieceType>(move.from());

    int victimVal   = PIECE_VALUES[static_cast<int>(victimType)];
    int attackerVal = PIECE_VALUES[static_cast<int>(attackerType)];
    return victimVal * 10 - attackerVal;
}

} // namespace chess
