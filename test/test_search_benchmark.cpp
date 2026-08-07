#include "chess/node_move.h"
#include "chess/node_move_optimized.h"

#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
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

struct SearchResult {
    float score;
    std::string move;
    double build_milliseconds;
    double search_milliseconds;
};

SearchResult measureMinimax(const chess::Board& position, int depth,
                            int iterations) {
    chess::Board board = position;
    BenchmarkEvaluator evaluator;
    CoutSilencer silence_output;

    const auto build_start = std::chrono::steady_clock::now();
    chess::NodeMove root(&board);
    const auto build_end = std::chrono::steady_clock::now();

    // Warm up the code path before measuring it.
    root.minimax(&evaluator, &board, chess::Color::WHITE, depth);

    const auto start = std::chrono::steady_clock::now();
    float score = 0.0f;
    chess::Move best_move;
    for (int iteration = 0; iteration < iterations; ++iteration) {
        score = root.minimax(&evaluator, &board, chess::Color::WHITE, depth);
        best_move = root.getBestMove(score);
    }
    const auto end = std::chrono::steady_clock::now();

    require(board.getFen() == position.getFen(),
            "minimax did not restore the benchmark board");

    return {
        score,
        chess::uci::moveToUci(best_move),
        std::chrono::duration<double, std::milli>(build_end - build_start)
            .count(),
        std::chrono::duration<double, std::milli>(end - start).count()
    };
}

SearchResult measureAlphaBeta(const chess::Board& position, int depth,
                              int iterations) {
    chess::Board board = position;
    BenchmarkEvaluator evaluator;
    CoutSilencer silence_output;

    const auto build_start = std::chrono::steady_clock::now();
    chess::NodeMove root(&board);
    const auto build_end = std::chrono::steady_clock::now();

    float warmup_alpha = -99999.0f;
    float warmup_beta = 99999.0f;
    std::mutex warmup_mutex;
    root.alphaBeta(&evaluator, &warmup_alpha, &warmup_beta,
                   chess::Color::WHITE, &board, &warmup_mutex, depth);

    const auto start = std::chrono::steady_clock::now();
    float score = 0.0f;
    chess::Move best_move;
    for (int iteration = 0; iteration < iterations; ++iteration) {
        float alpha = -99999.0f;
        float beta = 99999.0f;
        std::mutex alpha_beta_mutex;
        score = root.alphaBeta(&evaluator, &alpha, &beta,
                               chess::Color::WHITE, &board,
                               &alpha_beta_mutex, depth);
        best_move = root.getBestMove(score);
    }
    const auto end = std::chrono::steady_clock::now();

    require(board.getFen() == position.getFen(),
            "alpha-beta did not restore the benchmark board");

    return {
        score,
        chess::uci::moveToUci(best_move),
        std::chrono::duration<double, std::milli>(build_end - build_start)
            .count(),
        std::chrono::duration<double, std::milli>(end - start).count()
    };
}

SearchResult measureOptimizedAlphaBeta(const chess::Board& position, int depth,
                                       int iterations) {
    chess::Board board = position;
    BenchmarkEvaluator evaluator;
    CoutSilencer silence_output;

    const auto build_start = std::chrono::steady_clock::now();
    chess::NodeMoveOptimized root(&board);
    const auto build_end = std::chrono::steady_clock::now();

    // Warm up allocation and instruction-cache paths before timing searches.
    root.alphaBeta(&evaluator, chess::Color::WHITE, &board, depth);

    const auto start = std::chrono::steady_clock::now();
    float score = 0.0f;
    chess::Move best_move;
    for (int iteration = 0; iteration < iterations; ++iteration) {
        score = root.alphaBeta(&evaluator, chess::Color::WHITE, &board, depth);
        best_move = root.getBestMove(score);
    }
    const auto end = std::chrono::steady_clock::now();

    require(board.getFen() == position.getFen(),
            "optimized alpha-beta did not restore the benchmark board");

    return {
        score,
        chess::uci::moveToUci(best_move),
        std::chrono::duration<double, std::milli>(build_end - build_start)
            .count(),
        std::chrono::duration<double, std::milli>(end - start).count()
    };
}

void compareSearchesAtSeveralDepths() {
    const chess::Board position(chess::constants::STARTPOS);
    constexpr int iterations = 3;

    std::cout << std::fixed << std::setprecision(3);
    for (const int depth : {1, 2, 3}) {
        SearchResult minimax = measureMinimax(position, depth, iterations);
        SearchResult alpha_beta = measureAlphaBeta(position, depth, iterations);
        SearchResult optimized =
            measureOptimizedAlphaBeta(position, depth, iterations);

        require(std::fabs(minimax.score - alpha_beta.score) < 0.00001f,
                "minimax and alpha-beta disagree on the search score");
        require(std::fabs(alpha_beta.score - optimized.score) < 0.00001f,
                "optimized alpha-beta changed the search score");
        require(minimax.move == alpha_beta.move,
                "minimax and alpha-beta return different best moves");
        require(alpha_beta.move == optimized.move,
                "optimized alpha-beta returned a different best move");
        require(!minimax.move.empty() && minimax.move != "0000",
                "minimax did not return a best move");
        require(!alpha_beta.move.empty() && alpha_beta.move != "0000",
                "alpha-beta did not return a best move");
        require(!optimized.move.empty() && optimized.move != "0000",
                "optimized alpha-beta did not return a best move");
        require(minimax.search_milliseconds > 0.0 &&
                    alpha_beta.search_milliseconds > 0.0 &&
                    optimized.search_milliseconds > 0.0,
                "search timing did not produce a positive duration");
        require(minimax.build_milliseconds > 0.0 &&
                    alpha_beta.build_milliseconds > 0.0 &&
                    optimized.build_milliseconds > 0.0,
                "tree construction timing did not produce a positive duration");

        const double alpha_beta_speedup =
            alpha_beta.search_milliseconds / optimized.search_milliseconds;
        const double build_speedup =
            alpha_beta.build_milliseconds / optimized.build_milliseconds;
        std::cout << "depth=" << depth
                  << " move=" << minimax.move
                  << " baseline_build_ms=" << alpha_beta.build_milliseconds
                  << " optimized_build_ms=" << optimized.build_milliseconds
                  << " baseline_alpha_beta_ms="
                  << alpha_beta.search_milliseconds
                  << " optimized_alpha_beta_ms="
                  << optimized.search_milliseconds
                  << " build_speedup=" << build_speedup << "x"
                  << " search_speedup=" << alpha_beta_speedup << "x\n";
    }
}

} // namespace

int main() {
    try {
        compareSearchesAtSeveralDepths();
        std::cout << "PASS search_benchmark\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL search_benchmark: " << error.what() << '\n';
        return 1;
    }
}
