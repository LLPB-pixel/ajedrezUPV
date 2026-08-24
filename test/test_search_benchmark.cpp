#include "chess/node_move_optimized.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

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

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

class BenchmarkEvaluator : public GeneralEvaluator {
public:
    float evaluate(const chess::Board* board, const chess::Color) override {
        const auto key = board->hash();
        float score = static_cast<float>(key % 100000ULL) / 100.0f;

        // Keep every leaf evaluation equally expensive so the measured
        // difference comes from pruning rather than from the evaluator.
        volatile float work = 0.0f;
        for (int index = 0; index < 256; ++index) {
            work += static_cast<float>((key + index) % 97ULL) * 0.000001f;
        }
        return score + work;
    }
};

struct BenchmarkResult {
    float score;
    std::string move;
    double build_milliseconds;
    double search_milliseconds;
};

BenchmarkResult measureOptimizedAlphaBeta(const chess::Board& position, int depth,
                                          int iterations) {
    chess::Board board = position;
    BenchmarkEvaluator evaluator;
    CoutSilencer silence_output;

    const auto build_start = std::chrono::steady_clock::now();
    chess::NodeMoveOptimized::TreeContext context;
    chess::NodeMoveOptimized root(context, &board);
    const auto build_end = std::chrono::steady_clock::now();

    // Warm up allocation and instruction-cache paths before timing searches.
    root.alphaBeta(&evaluator, chess::Color::WHITE, &board, depth);

    const auto start = std::chrono::steady_clock::now();
    chess::SearchResult result;
    for (int iteration = 0; iteration < iterations; ++iteration) {
        result = root.alphaBeta(&evaluator, chess::Color::WHITE, &board, depth);
    }
    const auto end = std::chrono::steady_clock::now();

    require(board.getFen() == position.getFen(),
            "optimized alpha-beta did not restore the benchmark board");
    require(root.getChildByMove(result.move) != nullptr,
            "optimized alpha-beta returned an invalid best move");

    return {
        result.score,
        chess::uci::moveToUci(result.move),
        std::chrono::duration<double, std::milli>(build_end - build_start)
            .count(),
        std::chrono::duration<double, std::milli>(end - start).count()
    };
}

void benchmarkOptimizedAlphaBetaAtSeveralDepths() {
    const chess::Board position(chess::constants::STARTPOS);
    constexpr int iterations = 3;

    std::cout << std::fixed << std::setprecision(3);
    for (const int depth : {1, 2, 3}) {
        const BenchmarkResult result =
            measureOptimizedAlphaBeta(position, depth, iterations);

        require(!result.move.empty() && result.move != "0000",
                "optimized alpha-beta did not return a best move");
        require(result.search_milliseconds > 0.0,
                "search timing did not produce a positive duration");
        require(result.build_milliseconds > 0.0,
                "tree construction timing did not produce a positive duration");

        std::cout << "depth=" << depth
                  << " move=" << result.move
                  << " build_ms=" << result.build_milliseconds
                  << " alpha_beta_ms=" << result.search_milliseconds << '\n';
    }
}

} // namespace

int main() {
    try {
        benchmarkOptimizedAlphaBetaAtSeveralDepths();
        std::cout << "PASS search_benchmark\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL search_benchmark: " << error.what() << '\n';
        return 1;
    }
}
