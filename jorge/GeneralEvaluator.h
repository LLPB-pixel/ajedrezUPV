#ifndef GENERALEVALUATOR_H
#define GENERALEVALUATOR_H

#include "chess.hpp"
#include "Evaluator.h"
using namespace chess;



class GeneralEvaluator : public Evaluator {
public:
    GeneralEvaluator();
    float evaluate(const Board *board, const Color color) override;
    float positionOfThePiecesAndMaterial(const Board *board) override;
    float pawn_structure(const Board *board, const Color color) override;
    float safe_king(const Board *board, const Color color) override;
    float control(const Board *board, const Color color) override;
    ~GeneralEvaluator() override;

private:
    chess::Bitboard getSeenSquares(const chess::Board *board, chess::Color color);
    chess::Bitboard getControlledSquares(const chess::Board *board, chess::Color color);
    chess::Bitboard getEnemySeenSquaresIgnoringOwnPieces(const Board *board, Color enemyColor);
};

#endif
