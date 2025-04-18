#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "chess.hpp"
using namespace chess;

#define PAWN_VALUE 1
#define KNIGHT_VALUE 3   
#define BISHOP_VALUE 3
#define ROOK_VALUE 5
#define QUEEN_VALUE 9

class Evaluator{
    protected:
        float max, min;
        float white_importance[8][8] = {
            {0.025, 0.05, 0.075, 0.1, 0.1, 0.075, 0.05, 0.025},
            {0.05, 0.1, 0.125, 0.15, 0.15, 0.125, 0.1, 0.05},
            {0.075, 0.125, 0.175, 0.2, 0.2, 0.175, 0.125, 0.075},
            {0.1, 0.15, 0.2, 0.25, 0.25, 0.2, 0.15, 0.1},
            {0.1, 0.15, 0.2, 0.25, 0.25, 0.2, 0.15, 0.1},
            {0.075, 0.125, 0.175, 0.2, 0.2, 0.175, 0.125, 0.075},
            {0.05, 0.1, 0.125, 0.15, 0.15, 0.125, 0.1, 0.05},
            {0.025, 0.05, 0.075, 0.1, 0.1, 0.075, 0.05, 0.025}};

        float black_importance[8][8] = {
            {0.025, 0.05, 0.075, 0.1, 0.1, 0.075, 0.05, 0.025},
            {0.05, 0.1, 0.125, 0.15, 0.15, 0.125, 0.1, 0.05},
            {0.075, 0.125, 0.175, 0.2, 0.2, 0.175, 0.125, 0.075},
            {0.1, 0.15, 0.2, 0.25, 0.25, 0.2, 0.15, 0.1},
            {0.1, 0.15, 0.2, 0.25, 0.25, 0.2, 0.15, 0.1},
            {0.075, 0.125, 0.175, 0.2, 0.2, 0.175, 0.125, 0.075},
            {0.05, 0.1, 0.125, 0.15, 0.15, 0.125, 0.1, 0.05},
            {0.025, 0.05, 0.075, 0.1, 0.1, 0.075, 0.05, 0.025}};

      public:
        virtual float evaluate(const Board *board, const Color color) = 0;
        virtual float positionOfThePiecesAndMaterial(const Board *board);
        virtual float pawn_structure(const Board *board, const Color color);
        virtual float safe_king(const Board *board, const Color color);
        virtual float control(const Board *board, const Color color);
        virtual ~Evaluator();
};



#endif



#endif
