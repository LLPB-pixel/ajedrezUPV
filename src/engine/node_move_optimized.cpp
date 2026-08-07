#include "chess/node_move_optimized.h"

#include <algorithm>
#include <iostream>
#include <string>

namespace chess {

NodeMoveOptimized::NodeMoveOptimized(Board* board, NodeMoveOptimized* parent)
    : storage_owner_(parent ? nullptr : std::make_unique<Storage>()),
      storage_(parent ? parent->storage_ : storage_owner_.get()),
      current_depth_(parent ? parent->current_depth_ + 1 : 0),
      last_move_(),
      children_(&storage_->pool) {
    buildChildren(board);
}

NodeMoveOptimized::NodeMoveOptimized(Board* board, int depth, Move last_move,
                                     Storage* storage)
    : storage_owner_(),
      storage_(storage),
      current_depth_(depth),
      last_move_(last_move),
      children_(&storage_->pool) {
    buildChildren(board);
}

void NodeMoveOptimized::buildChildren(Board* board) {
    children_.clear();

    if (current_depth_ >= MAX_DEPTH) {
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
        children_.push_back(
            NodeMoveOptimized(board, current_depth_ + 1, move, storage_));
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

void NodeMoveOptimized::rebuildUntilDepth(Board* board) {
    if (current_depth_ >= MAX_DEPTH - 1) {
        buildChildren(board);
        return;
    }

    for (NodeMoveOptimized& child : children_) {
        board->makeMove(child.last_move_);
        child.rebuildUntilDepth(board);
        board->unmakeMove(child.last_move_);
    }
}

void NodeMoveOptimized::addChild(Board* board, Move move) {
    if (current_depth_ >= MAX_DEPTH || children_.size() >= MAX_BRANCH) {
        return;
    }

    children_.push_back(
        NodeMoveOptimized(board, current_depth_ + 1, move, storage_));
}

float NodeMoveOptimized::minimax(GeneralEvaluator* evaluator, Board* board,
                                 Color root_color, int depth_limit) {
    return minimaxImpl(evaluator, *board, root_color, depth_limit);
}

float NodeMoveOptimized::minimaxImpl(GeneralEvaluator* evaluator, Board& board,
                                     Color root_color, int depth_limit) {
    if (current_depth_ >= depth_limit || current_depth_ >= MAX_DEPTH ||
        children_.empty() || isSearchTerminal(board)) {
        eval_ = evaluator->evaluate(&board, root_color);
        return eval_;
    }

    const bool maximizing = board.sideToMove() == Color::WHITE;
    float best_value = maximizing ? NEGATIVE_INFINITY : POSITIVE_INFINITY;

    for (NodeMoveOptimized& child : children_) {
        board.makeMove(child.last_move_);
        const float value =
            child.minimaxImpl(evaluator, board, root_color, depth_limit);
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
                         NEGATIVE_INFINITY, POSITIVE_INFINITY);
}

float NodeMoveOptimized::alphaBeta(
    GeneralEvaluator* evaluator, float* alpha, float* beta, Color root_color,
    Board* board, std::mutex* alphaBetaMutex, int depth_limit) {
    (void)alphaBetaMutex;

    const float initial_alpha = alpha ? *alpha : NEGATIVE_INFINITY;
    const float initial_beta = beta ? *beta : POSITIVE_INFINITY;
    const float score = alphaBetaImpl(evaluator, *board, root_color,
                                      depth_limit, initial_alpha, initial_beta);

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
                                       float beta) {
    if (current_depth_ >= depth_limit || current_depth_ >= MAX_DEPTH ||
        children_.empty() || isSearchTerminal(board)) {
        eval_ = evaluator->evaluate(&board, root_color);
        return eval_;
    }

    const bool maximizing = board.sideToMove() == Color::WHITE;
    float best_value = maximizing ? NEGATIVE_INFINITY : POSITIVE_INFINITY;

    for (NodeMoveOptimized& child : children_) {
        board.makeMove(child.last_move_);
        const float value = child.alphaBetaImpl(
            evaluator, board, root_color, depth_limit, alpha, beta);
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
    std::cout << std::string(indent, ' ') << "Nodo profundidad "
              << current_depth_ << " - Movimiento: "
              << uci::moveToUci(last_move_) << " - Hijos: "
              << children_.size() << " - Eval: " << eval_ << '\n';

    for (const NodeMoveOptimized& child : children_) {
        child.printTree(indent + 4);
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
