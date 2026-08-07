#include "chess/node_move_optimized.h"

#include <algorithm>
#include <iostream>
#include <string>

namespace chess {

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

    // Do not construct nodes that cannot fit in the fixed branch limit.
    const size_t child_count = std::min<size_t>(moves.size(), MAX_BRANCH);
    children_.reserve(child_count);

    for (size_t index = 0; index < child_count; ++index) {
        const Move move = moves[index];
        board->makeMove(move);
        children_.push_back(NodeMoveOptimized(board, depth + 1, move,
                                              resource));
        board->unmakeMove(move);
    }
}

bool NodeMoveOptimized::isSearchTerminal(const Board& board) noexcept {
    // isGameOver() regenerates legal moves at every node. Legal children are
    // already stored in this tree, so only the inexpensive draw conditions
    // need to be checked here.
    if (board.isHalfMoveDraw()) {
        return true;
    }

    // Insufficient material is only possible with four or fewer pieces.
    if (board.occ().count() <= 4 && board.isInsufficientMaterial()) {
        return true;
    }

    // Threefold repetition needs at least four reversible half-moves.
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

float NodeMoveOptimized::minimax(GeneralEvaluator* evaluator, Board* board,
                                 Color root_color, int depth_limit) {
    return minimaxImpl(evaluator, *board, root_color, depth_limit, 0);
}

float NodeMoveOptimized::minimaxImpl(GeneralEvaluator* evaluator, Board& board,
                                     Color root_color, int depth_limit,
                                     int searched_depth) {
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

float NodeMoveOptimized::alphaBeta(GeneralEvaluator* evaluator,
                                   Color root_color, Board* board,
                                   int depth_limit) {
    return alphaBetaImpl(evaluator, *board, root_color, depth_limit,
                         NEGATIVE_INFINITY, POSITIVE_INFINITY, 0);
}

float NodeMoveOptimized::alphaBeta(
    GeneralEvaluator* evaluator, float* alpha, float* beta, Color root_color,
    Board* board, std::mutex* alphaBetaMutex, int depth_limit) {
    (void)alphaBetaMutex;

    const float initial_alpha = alpha ? *alpha : NEGATIVE_INFINITY;
    const float initial_beta = beta ? *beta : POSITIVE_INFINITY;
    const float score = alphaBetaImpl(evaluator, *board, root_color,
                                       depth_limit, initial_alpha, initial_beta,
                                       0);

    // Match the observable part of the original pointer-based API without
    // carrying pointers through every recursive call.
    if (board->sideToMove() == Color::WHITE && alpha) {
        *alpha = std::max(*alpha, score);
    } else if (board->sideToMove() == Color::BLACK && beta) {
        *beta = std::min(*beta, score);
    }

    return score;
}

float NodeMoveOptimized::alphaBetaImpl(GeneralEvaluator* evaluator,
                                       Board& board, Color root_color,
                                       int depth_limit, float alpha,
                                       float beta, int searched_depth) {
    if (searched_depth >= depth_limit || searched_depth >= MAX_DEPTH ||
        children_.empty() || isSearchTerminal(board)) {
        eval_ = evaluator->evaluate(&board, root_color);
        return eval_;
    }

    const bool maximizing = board.sideToMove() == Color::WHITE;
    float best_value = maximizing ? NEGATIVE_INFINITY : POSITIVE_INFINITY;

    for (NodeMoveOptimized& child : children_) {
        board.makeMove(child.last_move_);
        const float value = child.alphaBetaImpl(
            evaluator, board, root_color, depth_limit, alpha, beta,
            searched_depth + 1);
        board.unmakeMove(child.last_move_);

        if (maximizing) {
            best_value = std::max(best_value, value);
            alpha = std::max(alpha, best_value);
        } else {
            best_value = std::min(best_value, value);
            beta = std::min(beta, best_value);
        }

        if (alpha >= beta) {
            break;
        }
    }

    eval_ = best_value;
    return best_value;
}

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
