#include "chess/node_move_optimized.h"

#include <algorithm>
#include <iostream>
#include <string>

namespace chess {

// =========================================================================
// [B1] Mate / stalemate helpers
// =========================================================================

bool NodeMoveOptimized::isCheckmate(const Board& board) {
    Movelist moves;
    movegen::legalmoves(moves, board);
    return moves.size() == 0 && board.inCheck();
}

bool NodeMoveOptimized::isStalemate(const Board& board) {
    Movelist moves;
    movegen::legalmoves(moves, board);
    return moves.size() == 0 && !board.inCheck();
}

// =========================================================================
// Constructor / tree building
// =========================================================================

NodeMoveOptimized::NodeMoveOptimized(TreeContext& context, Board* board)
    : last_move_(), children_(&context.pool) {
    buildChildren(board, 0, &context.pool);
}

NodeMoveOptimized::NodeMoveOptimized(Board* board, int depth, Move last_move,
                                     std::pmr::memory_resource* resource)
    : last_move_(last_move), children_(resource) {
    buildChildren(board, depth, resource);
}

void NodeMoveOptimized::buildChildren(
    Board* board, int depth, std::pmr::memory_resource* resource) {
    children_.clear();

    if (depth >= MAX_DEPTH) {
        return;
    }

    Movelist moves;
    movegen::legalmoves(moves, *board);

    const size_t child_count = std::min<size_t>(static_cast<size_t>(moves.size()), MAX_BRANCH);
    children_.reserve(child_count);

    for (size_t index = 0; index < child_count; ++index) {
        const Move move = moves[static_cast<int>(index)];
        board->makeMove(move);
        children_.push_back(NodeMoveOptimized(board, depth + 1, move,
                                              resource));
        board->unmakeMove(move);
    }
}

bool NodeMoveOptimized::isSearchTerminal(const Board& board) noexcept {
    if (board.isHalfMoveDraw()) {
        return true;
    }
    if (board.occ().count() <= 4 && board.isInsufficientMaterial()) {
        return true;
    }
    return board.halfMoveClock() >= 4 && board.isRepetition();
}

void NodeMoveOptimized::rebuildUntilDepth(Board* board, int current_depth) {
    std::pmr::memory_resource* resource = children_.get_allocator().resource();

    if (current_depth >= MAX_DEPTH - 1) {
        buildChildren(board, current_depth, resource);
        return;
    }

    for (NodeMoveOptimized& child : children_) {
        board->makeMove(child.last_move_);
        child.rebuildUntilDepth(board, current_depth + 1);
        board->unmakeMove(child.last_move_);
    }
}

void NodeMoveOptimized::addChild(Board* board, Move move, int current_depth) {
    if (current_depth >= MAX_DEPTH || children_.size() >= MAX_BRANCH) {
        return;
    }

    children_.push_back(NodeMoveOptimized(
        board, current_depth + 1, move, children_.get_allocator().resource()));
}

// =========================================================================
// [B3] Move ordering — MVV-LVA for captures, killers, history
// =========================================================================

int NodeMoveOptimized::mvvLvaScore(const Board& board, const Move& move) {
    if (move.typeOf() == Move::CASTLING) return -1000;

    // Detect captures: destination square occupied (except castling).
    if (board.at(move.to()) == Piece() && move.typeOf() != Move::ENPASSANT) {
        return -1000; // Not a capture
    }

    PieceType victimType = (move.typeOf() == Move::ENPASSANT)
        ? PieceType::PAWN
        : board.at<PieceType>(move.to());
    PieceType attackerType = board.at<PieceType>(move.from());

    int victimVal   = PIECE_VALUES[static_cast<int>(victimType)];
    int attackerVal = PIECE_VALUES[static_cast<int>(attackerType)];
    return victimVal * 10 - attackerVal;
}

void NodeMoveOptimized::orderMoves(Movelist& moves, Board& board, int ply) {
    // Build a scored index list, then reconstruct the movelist in order.
    const int n = moves.size();
    std::vector<std::pair<int, int>> scored;
    scored.reserve(n);

    for (int i = 0; i < n; ++i) {
        const Move& m = moves[i];
        int key = 0;

        bool isCapture = (m.typeOf() == Move::ENPASSANT) ||
                         (board.at(m.to()) != Piece() && m.typeOf() != Move::CASTLING);
        bool isPromotion = (m.typeOf() == Move::PROMOTION);

        if (isCapture) {
            key = 100000 + mvvLvaScore(board, m);
        }
        if (isPromotion) {
            key += 95000;
        }

        // Killer moves: if this move is a killer at this ply, promote it.
        if (ply >= 0 && ply < MAX_PLY) {
            if (m == killers_[ply][0]) key += 90000;
            else if (m == killers_[ply][1]) key += 80000;
        }

        // History heuristic.
        key += history_[m.from().index()][m.to().index()];

        scored.push_back({key, i});
    }

    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });

    Movelist ordered;
    for (int i = 0; i < n; ++i) {
        ordered.add(moves[scored[i].second]);
    }
    moves = ordered;
}

// =========================================================================
// [B2] Quiescence search — extend captures/promotions at leaf nodes
// =========================================================================

float NodeMoveOptimized::quiescence(GeneralEvaluator* evaluator,
                                    Board& board, Color root_color,
                                    float alpha, float beta,
                                    int qdepth, int max_qdepth) {
    // Stand-pat: evaluate the current position without any move.
    float stand_pat = evaluator->evaluate(&board, root_color);

    if (root_color == board.sideToMove()) {
        if (stand_pat >= beta) return beta;
        if (stand_pat > alpha) alpha = stand_pat;
    } else {
        if (stand_pat <= alpha) return alpha;
        if (stand_pat < beta) beta = stand_pat;
    }

    if (qdepth >= max_qdepth) {
        return stand_pat;
    }

    // Check for terminal positions.
    if (isCheckmate(board)) {
        return (board.sideToMove() == root_color)
            ? -eval::MATE_SCORE + static_cast<float>(qdepth)
            :  eval::MATE_SCORE - static_cast<float>(qdepth);
    }
    if (isStalemate(board) || isSearchTerminal(board)) {
        return 0.0f;
    }

    Movelist moves;
    movegen::legalmoves(moves, board);

    // Filter to captures and promotions only.
    Movelist captures;
    for (int i = 0; i < moves.size(); ++i) {
        const Move& m = moves[i];
        bool isCapture = (m.typeOf() == Move::ENPASSANT) ||
                         (board.at(m.to()) != Piece() && m.typeOf() != Move::CASTLING);
        bool isPromotion = (m.typeOf() == Move::PROMOTION);
        if (isCapture || isPromotion) {
            captures.add(m);
        }
    }

    if (captures.size() == 0) {
        return stand_pat;
    }

    // Sort captures by MVV-LVA for better pruning.
    {
        const int cn = captures.size();
        std::vector<std::pair<int, int>> cscored;
        cscored.reserve(cn);
        for (int i = 0; i < cn; ++i) {
            cscored.push_back({mvvLvaScore(board, captures[i]), i});
        }
        std::sort(cscored.begin(), cscored.end(),
                  [](const auto& a, const auto& b) { return a.first > b.first; });
        Movelist sorted_captures;
        for (int i = 0; i < cn; ++i) {
            sorted_captures.add(captures[cscored[i].second]);
        }
        captures = sorted_captures;
    }

    float best = stand_pat;
    const bool maximizing = (board.sideToMove() == Color::WHITE);

    for (int i = 0; i < captures.size(); ++i) {
        const Move& move = captures[i];
        board.makeMove(move);
        float score = quiescence(evaluator, board, root_color,
                                 alpha, beta, qdepth + 1, max_qdepth);
        board.unmakeMove(move);

        if (maximizing) {
            if (score > best) best = score;
            if (score > alpha) alpha = score;
        } else {
            if (score < best) best = score;
            if (score < beta) beta = score;
        }

        if (alpha >= beta) break;
    }

    return best;
}

// =========================================================================
// Minimax (kept for backward compatibility)
// =========================================================================

float NodeMoveOptimized::minimax(GeneralEvaluator* evaluator, Board* board,
                                 Color root_color, int depth_limit) {
    return minimaxImpl(evaluator, *board, root_color, depth_limit, 0);
}

float NodeMoveOptimized::minimaxImpl(GeneralEvaluator* evaluator, Board& board,
                                     Color root_color, int depth_limit,
                                     int searched_depth) {
    // [B1] Checkmate / stalemate / draw detection.
    if (isCheckmate(board)) {
        return (board.sideToMove() == root_color)
            ? -eval::MATE_SCORE + static_cast<float>(searched_depth)
            :  eval::MATE_SCORE - static_cast<float>(searched_depth);
    }
    if (isStalemate(board)) {
        return 0.0f;
    }

    if (searched_depth >= depth_limit || searched_depth >= MAX_DEPTH ||
        children_.empty() || isSearchTerminal(board)) {
        eval_ = evaluator->evaluate(&board, root_color);
        return eval_;
    }

    const bool maximizing = board.sideToMove() == Color::WHITE;
    float best_value = maximizing ? NEGATIVE_INFINITY : POSITIVE_INFINITY;

    for (NodeMoveOptimized& child : children_) {
        board.makeMove(child.last_move_);
        const float value =
            child.minimaxImpl(evaluator, board, root_color, depth_limit,
                              searched_depth + 1);
        board.unmakeMove(child.last_move_);

        best_value = maximizing ? std::max(best_value, value)
                                : std::min(best_value, value);
    }

    eval_ = best_value;
    return best_value;
}

// =========================================================================
// [B8] Alpha-beta returning SearchResult
// =========================================================================

SearchResult NodeMoveOptimized::alphaBeta(GeneralEvaluator* evaluator,
                                          Color root_color, Board* board,
                                          int depth_limit) {
    float score = alphaBetaImpl(evaluator, *board, root_color, depth_limit,
                                NEGATIVE_INFINITY, POSITIVE_INFINITY, 0);

    // Find the child with the matching eval to extract the best move.
    Move best;
    for (const NodeMoveOptimized& child : children_) {
        if (child.eval_ == score) {
            best = child.last_move_;
            break;
        }
    }

    return {best, score, depth_limit};
}

// Compatibility overload for tests.
float NodeMoveOptimized::alphaBeta(
    GeneralEvaluator* evaluator, float* alpha, float* beta, Color root_color,
    Board* board, std::mutex* alphaBetaMutex, int depth_limit) {
    (void)alphaBetaMutex;

    const float initial_alpha = alpha ? *alpha : NEGATIVE_INFINITY;
    const float initial_beta = beta ? *beta : POSITIVE_INFINITY;
    const float score = alphaBetaImpl(evaluator, *board, root_color,
                                       depth_limit, initial_alpha, initial_beta,
                                       0);

    if (board->sideToMove() == Color::WHITE && alpha) {
        *alpha = std::max(*alpha, score);
    } else if (board->sideToMove() == Color::BLACK && beta) {
        *beta = std::min(*beta, score);
    }

    return score;
}

// =========================================================================
// Alpha-beta implementation
// =========================================================================

float NodeMoveOptimized::alphaBetaImpl(GeneralEvaluator* evaluator,
                                       Board& board, Color root_color,
                                       int depth_limit, float alpha,
                                       float beta, int searched_depth) {
    // [B1] Checkmate / stalemate detection before anything else.
    if (isCheckmate(board)) {
        return (board.sideToMove() == root_color)
            ? -eval::MATE_SCORE + static_cast<float>(searched_depth)
            :  eval::MATE_SCORE - static_cast<float>(searched_depth);
    }
    if (isStalemate(board)) {
        return 0.0f;
    }

    // Terminal conditions.
    if (searched_depth >= depth_limit || searched_depth >= MAX_DEPTH ||
        children_.empty() || isSearchTerminal(board)) {
        // [B2] At leaf nodes, run quiescence search instead of a hard cutoff.
        if (searched_depth >= depth_limit || searched_depth >= MAX_DEPTH) {
            eval_ = quiescence(evaluator, board, root_color,
                               alpha, beta, 0, 8);
            return eval_;
        }
        eval_ = evaluator->evaluate(&board, root_color);
        return eval_;
    }

    const bool maximizing = board.sideToMove() == Color::WHITE;
    float best_value = maximizing ? NEGATIVE_INFINITY : POSITIVE_INFINITY;
    Move best_move;

    for (NodeMoveOptimized& child : children_) {
        board.makeMove(child.last_move_);
        const float value = child.alphaBetaImpl(
            evaluator, board, root_color, depth_limit, alpha, beta,
            searched_depth + 1);
        board.unmakeMove(child.last_move_);

        if (maximizing) {
            if (value > best_value) {
                best_value = value;
                best_move = child.last_move_;
            }
            alpha = std::max(alpha, best_value);
        } else {
            if (value < best_value) {
                best_value = value;
                best_move = child.last_move_;
            }
            beta = std::min(beta, best_value);
        }

        if (alpha >= beta) {
            // [B3] Record killer move for quiet moves that caused a cutoff.
            bool isQuiet = (best_move.typeOf() != Move::ENPASSANT &&
                            board.at(best_move.to()) == Piece() &&
                            best_move.typeOf() != Move::PROMOTION);
            if (isQuiet && searched_depth < MAX_PLY) {
                if (killers_[searched_depth][0] != best_move) {
                    killers_[searched_depth][1] = killers_[searched_depth][0];
                    killers_[searched_depth][0] = best_move;
                }
                // Update history heuristic.
                history_[best_move.from().index()][best_move.to().index()] += depth_limit * depth_limit;
            }
            break;
        }
    }

    eval_ = best_value;
    return best_value;
}

// =========================================================================
// Best move helpers
// =========================================================================

Move NodeMoveOptimized::getBestMove(float best_score) const {
    for (const NodeMoveOptimized& child : children_) {
        if (child.eval_ == best_score) {
            return child.last_move_;
        }
    }
    return Move();
}

NodeMoveOptimized* NodeMoveOptimized::getChild(size_t index) {
    return index < children_.size() ? &children_[index] : nullptr;
}

const NodeMoveOptimized* NodeMoveOptimized::getChild(size_t index) const {
    return index < children_.size() ? &children_[index] : nullptr;
}

NodeMoveOptimized* NodeMoveOptimized::getChildByMove(const Move& move) {
    for (NodeMoveOptimized& child : children_) {
        if (child.last_move_ == move) {
            return &child;
        }
    }
    return nullptr;
}

const NodeMoveOptimized* NodeMoveOptimized::getChildByMove(
    const Move& move) const {
    for (const NodeMoveOptimized& child : children_) {
        if (child.last_move_ == move) {
            return &child;
        }
    }
    return nullptr;
}

// =========================================================================
// Debug / print utilities
// =========================================================================

void NodeMoveOptimized::printTree(int indent) const {
    printTreeImpl(indent, 0);
}

void NodeMoveOptimized::printTreeImpl(int indent, int depth) const {
    std::cout << std::string(indent, ' ') << "Nodo profundidad "
              << depth << " - Movimiento: "
              << uci::moveToUci(last_move_) << " - Hijos: "
              << children_.size() << " - Eval: " << eval_ << '\n';

    for (const NodeMoveOptimized& child : children_) {
        child.printTreeImpl(indent + 4, depth + 1);
    }
}

void NodeMoveOptimized::printEvaluationsOfChildren() const {
    std::cout << "Child move evaluations:\n";
    for (const NodeMoveOptimized& child : children_) {
        std::cout << "Move: " << uci::moveToUci(child.last_move_)
                  << " | Eval: " << child.eval_ << '\n';
    }
}

void NodeMoveOptimized::printBoard(Board& board) const {
    std::cout << "  +-----------------+\n";
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << rank + 1 << " | ";
        for (int file = 0; file < 8; ++file) {
            const Square square(static_cast<File>(file),
                                static_cast<Rank>(rank));
            std::cout << static_cast<std::string>(board.at(square)) << ' ';
        }
        std::cout << "|\n";
    }
    std::cout << "  +-----------------+\n";
    std::cout << "    a b c d e f g h\n";
}

} // namespace chess
