#include "chess/node_move.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <mutex>
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

void checkDepths(const chess::NodeMove& node, int expected_depth) {
    require(node.getCurrentDepth() == expected_depth,
            "node depth does not match its position in the tree");
    require(node.getChildCount() <= chess::MAX_BRANCH,
            "node contains more children than MAX_BRANCH");

    if (expected_depth == chess::MAX_DEPTH) {
        require(node.getChildCount() == 0,
                "a node at MAX_DEPTH must not have children");
        return;
    }

    for (size_t index = 0; index < node.getChildCount(); ++index) {
        const chess::NodeMove* child = node.getChild(index);
        require(child != nullptr, "child slot is null inside child_count");
        checkDepths(*child, expected_depth + 1);
    }
}

void treeHasConsistentDepths() {
    chess::Board board(chess::constants::STARTPOS);
    CoutSilencer silence_output;
    chess::NodeMove root(&board);

    require(root.getCurrentDepth() == 0, "root must start at depth zero");
    require(root.getChildCount() > 0, "root tree was not generated");
    checkDepths(root, 0);
}

void rebuildPreservesDepthInvariants() {
    chess::Board board(chess::constants::STARTPOS);
    CoutSilencer silence_output;
    chess::NodeMove root(&board);

    chess::NodeMove* next = root.getChild(0);
    require(next != nullptr, "root does not contain a first child");
    board.makeMove(next->getLastMove());

    next->rebuildUntilDepth(&board);
    checkDepths(*next, 1);
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

void alphaBetaMatchesMinimaxAndPrunes() {
    chess::Board minimax_board(chess::constants::STARTPOS);
    chess::Board alpha_beta_board(chess::constants::STARTPOS);
    CountingEvaluator minimax_evaluator;
    CountingEvaluator alpha_beta_evaluator;

    float minimax_score;
    float alpha_beta_score;
    chess::Move minimax_move;
    chess::Move alpha_beta_move;

    {
        CoutSilencer silence_output;
        chess::NodeMove minimax_root(&minimax_board);
        minimax_score = minimax_root.minimax(
            &minimax_evaluator, &minimax_board, chess::Color::WHITE);
        minimax_move = minimax_root.getBestMove(minimax_score);
    }

    {
        CoutSilencer silence_output;
        chess::NodeMove alpha_beta_root(&alpha_beta_board);
        float alpha = -99999.0f;
        float beta = 99999.0f;
        std::mutex alpha_beta_mutex;
        alpha_beta_score = alpha_beta_root.alphaBeta(
            &alpha_beta_evaluator, &alpha, &beta, chess::Color::WHITE,
            &alpha_beta_board, &alpha_beta_mutex);
        alpha_beta_move = alpha_beta_root.getBestMove(alpha_beta_score);

        require(alpha_beta_root.getChildByMove(alpha_beta_move) != nullptr,
                "alpha-beta did not return one of the root moves");
    }

    require(std::fabs(minimax_score - alpha_beta_score) < 0.00001f,
            "alpha-beta and minimax returned different scores");
    require(minimax_evaluator.evaluations > 0,
            "minimax did not evaluate any leaf");
    require(alpha_beta_evaluator.evaluations > 0,
            "alpha-beta did not evaluate any leaf");
    require(alpha_beta_evaluator.evaluations < minimax_evaluator.evaluations,
            "alpha-beta did not reduce the number of leaf evaluations");
    require(minimax_board.getFen() == chess::Board(chess::constants::STARTPOS).getFen(),
            "minimax did not restore the input board");
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
    runTest("tree_has_consistent_depths", treeHasConsistentDepths, failures);
    runTest("rebuild_preserves_depth_invariants", rebuildPreservesDepthInvariants,
            failures);
    runTest("alpha_beta_matches_minimax_and_prunes",
            alphaBetaMatchesMinimaxAndPrunes, failures);
    return failures == 0 ? 0 : 1;
}
