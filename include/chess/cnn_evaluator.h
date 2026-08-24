#pragma once
// cnn_evaluator.h — Evaluator that uses the light CNN (cnn_network.hpp)
// Drops into Search as a GeneralEvaluator replacement.
// Output of the CNN is in tanh(cp/400) ∈ [-1,1]; we scale to centipawns
// via cp = 400 * atanh(score) and return as float (same scale as heuristic).

#include "chess/evaluator.h"
#include "neural/cnn_network.hpp"

class CNNEvaluator : public Evaluator {
public:
    CNNEvaluator() { cnn_.init(); }

    // Load trained weights (binary v2 or JSON). Returns false on failure.
    bool loadBinary(const std::string& path) { return cnn_.loadBinary(path); }
    bool loadJson(const std::string& path)   { return cnn_.loadJson(path); }

    float evaluate(const Board *board, const Color color) override {
        // CNN is trained side-to-move perspective (board flipped for Black).
        // For general color perspective we flip the sign if needed:
        // evaluate(board, WHITE) should be positive when white is better,
        // regardless of side to move. Our CNN returns from side-to-move
        // perspective, so we adjust.
        float stmScore = cnn_.evaluate(*board); // in tanh space
        // Convert to cp-like scale for search (optional): keep in tanh space
        // Search expects scores in centipawn-ish range, but tanh space works
        // as long as mate scores dominate (±30000). We scale tanh*400 to cp.
        float cpScore = stmScore * 400.0f; // approx inverse of tanh for small values
        if (board->sideToMove() == color) return cpScore;
        else return -cpScore;
    }

    // Unused for CNN — required by interface, return 0 or delegate
    float positionOfThePiecesAndMaterial(const Board *board) override {
        (void)board; return 0.f;
    }
    float pawn_structure(const Board *board, const Color color) override {
        (void)board; (void)color; return 0.f;
    }
    float safe_king(const Board *board, const Color color) override {
        (void)board; (void)color; return 0.f;
    }
    float control(const Board *board, const Color color) override {
        (void)board; (void)color; return 0.f;
    }

private:
    chess::cnn::CNN cnn_;
};
