#ifndef OPENINGEVALUATOR_H
#define OPENINGEVALUATOR_H

#include "GeneralEvaluator.h"

class OpeningEvaluator : public GeneralEvaluator {
public:
    OpeningEvaluator();
    float evaluate(const Board *board, const Color color) override;
    float positionOfThePiecesAndMaterial(const Board *board);
    float pawn_structure(const Board *board, const Color color);
    float safe_king(const Board *board, const Color color);
    float control(const Board *board, const Color color);
    ~OpeningEvaluator();

private:
    chess::Bitboard getSeenSquares(const chess::Board *board, chess::Color color);
    chess::Bitboard getControlledSquares(const chess::Board *board, chess::Color color);
    chess::Bitboard getEnemySeenSquaresIgnoringOwnPieces(const Board *board, Color enemyColor);
    float evaluatePieceType(const Board* board, chess::PieceType type, chess::Color color, float baseValue, const float importance[8][8]);

    // Mapas de calor específicos para apertura
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

    static constexpr float king_safety_heatmap[8][8] = {
        {1,  1,  1,  0.0,  0.0, 0.0,  1,  1},
        {0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0},
        {0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0},
        {0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0},
        {0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0},
        {0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0},
        {0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0},
        {1,  1,  1,  0.0, 0.0,  0.0,  1,  1}};
};

#endif