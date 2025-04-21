#ifndef ENDGAMEEVALUATOR_H
#define ENDGAMEEVALUATOR_H

#include "GeneralEvaluator.h"

class EndgameEvaluator : public GeneralEvaluator {
public:
    EndgameEvaluator();
    float evaluate(const Board *board, const Color color) override;
    float positionOfThePiecesAndMaterial(const Board *board) override;
    ~EndgameEvaluator();

private:
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
