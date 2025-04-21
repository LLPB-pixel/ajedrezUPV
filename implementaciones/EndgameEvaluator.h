#ifndef ENDGAMEEVALUATOR_H
#define ENDGAMEEVALUATOR_H

#include "GeneralEvaluator.h"

class EndgameEvaluator : public GeneralEvaluator {
public:
    EndgameEvaluator();
    float evaluate(const Board *board, const Color color) override;
    float positionOfThePiecesAndMaterial(const Board *board);
    float pawn_structure(const Board *board, const Color color);
    float safe_king(const Board *board, const Color color);
    float control(const Board *board, const Color color);
    ~EndgameEvaluator();

private:
    chess::Bitboard getSeenSquares(const chess::Board *board, chess::Color color);
    chess::Bitboard getControlledSquares(const chess::Board *board, chess::Color color);
    chess::Bitboard getEnemySeenSquaresIgnoringOwnPieces(const Board *board, Color enemyColor);
    float evaluatePieceType(const Board* board, chess::PieceType type, chess::Color color, float baseValue, const float importance[8][8]);

    // Mapas de calor específicos para finales
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
};

#endif