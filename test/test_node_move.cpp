#include "chess/node_move_optimized.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

class TestFailure : public std::runtime_error {
public:
    explicit TestFailure(const std::string& message)
        : std::runtime_error(message) {}
};

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw TestFailure(message);
    }
}

class CoutSilencer {
public:
    CoutSilencer()
        : old_buffer_(std::cout.rdbuf(sink_.rdbuf())) {}

    ~CoutSilencer() {
        std::cout.rdbuf(old_buffer_);
    }

private:
    std::ofstream sink_{"/dev/null"};
    std::streambuf* old_buffer_;
};

void checkOptimizedDepths(const chess::NodeMoveOptimized& node,
                          int expected_depth) {
    require(node.getChildCount() <= chess::MAX_BRANCH,
            "optimized node contains too many children");

    if (expected_depth == chess::MAX_DEPTH) {
        require(node.getChildCount() == 0,
                "optimized node at MAX_DEPTH must not have children");
        return;
    }

    for (size_t index = 0; index < node.getChildCount(); ++index) {
        const chess::NodeMoveOptimized* child = node.getChild(index);
        require(child != nullptr,
                "optimized child slot is null inside child_count");
        checkOptimizedDepths(*child, expected_depth + 1);
    }
}

void optimizedTreeHasConsistentDepths() {
    chess::Board board(chess::constants::STARTPOS);
    chess::NodeMoveOptimized::TreeContext context;
    chess::NodeMoveOptimized root(context, &board);

    require(root.getChildCount() > 0,
            "optimized root tree was not generated");
    checkOptimizedDepths(root, 0);
}

void optimizedRebuildPreservesDepthInvariants() {
    chess::Board board(chess::constants::STARTPOS);
    chess::NodeMoveOptimized::TreeContext context;
    chess::NodeMoveOptimized root(context, &board);

    chess::NodeMoveOptimized* next = root.getChild(0);
    require(next != nullptr, "optimized root lacks a first child");
    board.makeMove(next->getLastMove());

    next->rebuildUntilDepth(&board, 1);
    checkOptimizedDepths(*next, 1);
}

class CountingEvaluator : public GeneralEvaluator {
public:
    std::size_t evaluations = 0;

    float evaluate(const chess::Board* board, const chess::Color) override {
        ++evaluations;

        // A deterministic score with enough variation to make pruning visible.
        const float material =
            static_cast<float>(board->pieces(chess::PieceType::PAWN,
                                             chess::Color::WHITE).count()) * 100.0f
            - static_cast<float>(board->pieces(chess::PieceType::PAWN,
                                               chess::Color::BLACK).count()) * 100.0f
            + static_cast<float>(board->hash() % 1000ULL) / 1000.0f;
        return material;
    }
};

void optimizedAlphaBetaSearchesAndRestoresBoard() {
    chess::Board alpha_beta_board(chess::constants::STARTPOS);
    CountingEvaluator alpha_beta_evaluator;

    float alpha_beta_score;
    chess::Move alpha_beta_move;

    {
        CoutSilencer silence_output;
        chess::NodeMoveOptimized::TreeContext context;
        chess::NodeMoveOptimized alpha_beta_root(context, &alpha_beta_board);
        alpha_beta_score = alpha_beta_root.alphaBeta(
            &alpha_beta_evaluator, chess::Color::WHITE, &alpha_beta_board);
        alpha_beta_move = alpha_beta_root.getBestMove(alpha_beta_score);

        require(alpha_beta_root.getChildByMove(alpha_beta_move) != nullptr,
                "alpha-beta did not return one of the root moves");
    }

    require(alpha_beta_evaluator.evaluations > 0,
            "alpha-beta did not evaluate any leaf");
    require(alpha_beta_board.getFen() == chess::Board(chess::constants::STARTPOS).getFen(),
            "alpha-beta did not restore the input board");
}

void runTest(const char* name, void (*test)(), int& failures) {
    try {
        test();
        std::cout << "PASS " << name << '\n';
    } catch (const std::exception& error) {
        ++failures;
        std::cerr << "FAIL " << name << ": " << error.what() << '\n';
    }
}

} // namespace

int main() {
    int failures = 0;
    runTest("optimized_tree_has_consistent_depths",
            optimizedTreeHasConsistentDepths, failures);
    runTest("optimized_rebuild_preserves_depth_invariants",
            optimizedRebuildPreservesDepthInvariants, failures);
    runTest("optimized_alpha_beta_searches_and_restores_board",
            optimizedAlphaBetaSearchesAndRestoresBoard, failures);
    return failures == 0 ? 0 : 1;
}
