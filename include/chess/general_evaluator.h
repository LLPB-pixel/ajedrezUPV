#ifndef GENERALEVALUATOR_H
#define GENERALEVALUATOR_H

#include "chess.hpp"
#include "chess/evaluator.h"
using namespace chess;

class GeneralEvaluator : public Evaluator {
public:
    GeneralEvaluator();
    float evaluate(const Board *board, const Color color) override;
    float positionOfThePiecesAndMaterial(const Board *board) override;
    float pawn_structure(const Board *board, const Color color) override;    float safe_king(const Board *board, const Color color) override;
    float control(const Board *board, const Color color) override;
    ~GeneralEvaluator() override;

protected:
    float evaluatePieceType(const Board* board, chess::PieceType type,
                            chess::Color color, float baseValue,
                            const float heatmaps[8][8]) const;

    // [PERF] Shared single-pass evaluation used by all phase evaluators.
    // Computes the attack maps ONCE for both colors and reuses them across
    // material / king-safety / control terms (previously each term recomputed
    // getSeenSquares() up to 10 times per evaluate() call).
    float evaluateCommon(const Board *board, const Color color, bool endgame);

    // Heatmap tables shared by the phase evaluators (moved here so
    // evaluateCommon can select them without constructing subclass objects).
    static constexpr float white_pawn_heatmap[8][8] = {
        {0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0},
        {0.1,  0.1,  0.1,  0.1,  0.1,  0.1,  0.1,  0.1},
        {0.3,  0.3,  0.4,  0.5,  0.5,  0.4,  0.3,  0.3},
        {0.2,  0.2,  0.3,  0.4,  0.4,  0.3,  0.2,  0.2},
        {0.1,  0.1,  0.2,  0.3,  0.3,  0.2,  0.1,  0.1},
        {0.05, 0.05, 0.1,  0.1,  0.1,  0.1,  0.05, 0.05},
        {0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0},
        {0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0}};

    static constexpr float black_pawn_heatmap[8][8] = {
        {0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0},
        {0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0},
        {0.05, 0.05, 0.1,  0.1,  0.1,  0.1,  0.05, 0.05},
        {0.1,  0.1,  0.2,  0.3,  0.3,  0.2,  0.1,  0.1},
        {0.2,  0.2,  0.3,  0.4,  0.4,  0.3,  0.2,  0.2},
        {0.3,  0.3,  0.4,  0.5,  0.5,  0.4,  0.3,  0.3},
        {0.1,  0.1,  0.1,  0.1,  0.1,  0.1,  0.1,  0.1},
        {0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0}};

    static constexpr float minor_pieces_heatmap[8][8] = {
        {0.0,  0.0,  0.1,  0.1,  0.1,  0.1,  0.0,  0.0},
        {0.0,  0.2,  0.3,  0.3,  0.3,  0.3,  0.2,  0.0},
        {0.1,  0.3,  0.5,  0.6,  0.6,  0.5,  0.3,  0.1},
        {0.1,  0.3,  0.6,  0.7,  0.7,  0.6,  0.3,  0.1},
        {0.1,  0.3,  0.6,  0.7,  0.7,  0.6,  0.3,  0.1},
        {0.1,  0.3,  0.5,  0.6,  0.6,  0.5,  0.3,  0.1},
        {0.0,  0.2,  0.3,  0.3,  0.3,  0.3,  0.2,  0.0},
        {0.0,  0.0,  0.1,  0.1,  0.1,  0.1,  0.0,  0.0}};

    static constexpr float king_endgame_heatmap[8][8] = {
        {0.0,  0.1,  0.2,  0.3,  0.3,  0.2,  0.1,  0.0},
        {0.1,  0.2,  0.3,  0.4,  0.4,  0.3,  0.2,  0.1},
        {0.2,  0.3,  0.4,  0.5,  0.5,  0.4,  0.3,  0.2},
        {0.3,  0.4,  0.5,  0.6,  0.6,  0.5,  0.4,  0.3},
        {0.3,  0.4,  0.5,  0.6,  0.6,  0.5,  0.4,  0.3},
        {0.2,  0.3,  0.4,  0.5,  0.5,  0.4,  0.3,  0.2},
        {0.1,  0.2,  0.3,  0.4,  0.4,  0.3,  0.2,  0.1},
        {0.0,  0.1,  0.2,  0.3,  0.3,  0.2,  0.1,  0.0}};

    static constexpr float pawn_endgame_heatmap[8][8] = {
        {0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0},
        {0.5,  0.5,  0.5,  0.5,  0.5,  0.5,  0.5,  0.5},
        {0.4,  0.4,  0.4,  0.4,  0.4,  0.4,  0.4,  0.4},
        {0.3,  0.3,  0.3,  0.3,  0.3,  0.3,  0.3,  0.3},
        {0.2,  0.2,  0.2,  0.2,  0.2,  0.2,  0.2,  0.2},
        {0.1,  0.1,  0.1,  0.1,  0.1,  0.1,  0.1,  0.1},
        {0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0},
        {0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0}};

private:
    // Attack maps computed once per evaluation and shared by every term.
    struct AttackMaps {
        chess::Bitboard seen[2];       // squares attacked by each color
        chess::Bitboard controlled[2]; // seen squares that survive contest filtering
    };

    void computeAttackMaps(const Board *board, AttackMaps& maps) const;

    // Contest filtering on precomputed seen-maps (popcount attacker counts).
    chess::Bitboard getControlledSquaresFromMaps(const Board *board,
                                                 Color color,
                                                 const AttackMaps& maps) const;

    // Term implementations operating on precomputed maps.
    float safe_king_maps(const Board *board, const Color color,
                         const AttackMaps& maps) const;
    float control_maps(const Color color, const AttackMaps& maps) const;

    // Material by phase (heatmap selection without subclass construction).
    // [FIX] Now correctly from the requested color's perspective (old code
    // always returned white - black, which broke negamax).
    float material_opening(const Board *board, Color color) const;
    float material_endgame(const Board *board, Color color) const;

    chess::Bitboard getSeenSquares(const Board *board, Color color) const;
    chess::Bitboard getControlledSquares(const Board *board, Color color) const;
};

#endif
