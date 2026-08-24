#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "chess.hpp"
#ifdef INFINITY
#undef INFINITY
#endif
using namespace chess;

// [E14] Replace macros with constexpr in a namespace to avoid
// polluting every translation unit that includes this header.
namespace eval {
    constexpr int PAWN_VALUE   = 100;   // [E11] centipawn scale
    constexpr int KNIGHT_VALUE = 300;
    constexpr int BISHOP_VALUE = 315;
    constexpr int ROOK_VALUE   = 500;
    constexpr int QUEEN_VALUE  = 900;

    // [B1] Mate and draw scoring constants (separate from the window).
    constexpr float MATE_SCORE  = 30000.0f;
    constexpr float INFINITY    = 50000.0f;
}

// Backward-compatible macros (deprecated, prefer eval:: namespace).
#define PAWN_VALUE   100
#define KNIGHT_VALUE 300
#define BISHOP_VALUE 315
#define ROOK_VALUE   500
#define QUEEN_VALUE  900

class Evaluator {
protected:
    // [E14] Remove unused max/min members.
    // [E14] Unify importance tables into one (symmetric for both colors).
    float importance[8][8] = {
        {0.025, 0.05, 0.075, 0.1, 0.1, 0.075, 0.05, 0.025},
        {0.05, 0.1, 0.125, 0.15, 0.15, 0.125, 0.1, 0.05},
        {0.075, 0.125, 0.175, 0.2, 0.2, 0.175, 0.125, 0.075},
        {0.1, 0.15, 0.2, 0.25, 0.25, 0.2, 0.15, 0.1},
        {0.1, 0.15, 0.2, 0.25, 0.25, 0.2, 0.15, 0.1},
        {0.075, 0.125, 0.175, 0.2, 0.2, 0.175, 0.125, 0.075},
        {0.05, 0.1, 0.125, 0.15, 0.15, 0.125, 0.1, 0.05},
        {0.025, 0.05, 0.075, 0.1, 0.1, 0.075, 0.05, 0.025}};

public:
    Evaluator() = default;
    virtual ~Evaluator() = default;

    // [E8] All terms return a score in perspective of the side to move
    // (negamax convention).  The search negates the sign when going up.
    virtual float evaluate(const Board *board, const Color color) = 0;
    virtual float positionOfThePiecesAndMaterial(const Board *board) = 0;
    virtual float pawn_structure(const Board *board, const Color color) = 0;
    virtual float safe_king(const Board *board, const Color color) = 0;
    virtual float control(const Board *board, const Color color) = 0;
};

#endif

