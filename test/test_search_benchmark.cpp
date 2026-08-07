#include "chess/node_move.h"

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
    double milliseconds;
};

SearchResult measureMinimax(const chess::Board& position, int depth,
                            int iterations) {
    chess::Board board = position;
    BenchmarkEvaluator evaluator;
    CoutSilencer silence_output;
    chess::NodeMove root(&board);

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

    return {
        score,
        chess::uci::moveToUci(best_move),
        std::chrono::duration<double, std::milli>(end - start).count()
    };
}

SearchResult measureAlphaBeta(const chess::Board& position, int depth,
                              int iterations) {
    chess::Board board = position;
    BenchmarkEvaluator evaluator;
    CoutSilencer silence_output;
    chess::NodeMove root(&board);

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

    return {
        score,
        chess::uci::moveToUci(best_move),
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

        require(std::fabs(minimax.score - alpha_beta.score) < 0.00001f,
                "minimax and alpha-beta disagree on the search score");
        require(minimax.move == alpha_beta.move,
                "minimax and alpha-beta return different best moves");
        require(!minimax.move.empty() && minimax.move != "0000",
                "minimax did not return a best move");
        require(!alpha_beta.move.empty() && alpha_beta.move != "0000",
                "alpha-beta did not return a best move");
        require(minimax.milliseconds > 0.0 && alpha_beta.milliseconds > 0.0,
                "search timing did not produce a positive duration");

        const double speedup = minimax.milliseconds / alpha_beta.milliseconds;
        std::cout << "depth=" << depth
                  << " move=" << minimax.move
                  << " minimax_ms=" << minimax.milliseconds
                  << " alpha_beta_ms=" << alpha_beta.milliseconds
                  << " speedup=" << speedup << "x\n";
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
