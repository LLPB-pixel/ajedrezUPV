#pragma once
#include "classEvaluator.hpp"
#include "chess.hpp"

using namespace chess;

class EvaluatorPhased : public Evaluator {
private:
    Evaluator* openingEvaluator;
    Evaluator* endgameEvaluator;

public:
    EvaluatorPhased(Evaluator* openingEval, Evaluator* endgameEval)
        : openingEvaluator(openingEval), endgameEvaluator(endgameEval) {}

    float evaluate(const Board& board, PieceColor color) override {
        int materialTotal = totalMaterial(board);

        if (queenDetector(board)) {
            return middleGameEvaluator->evaluate(board, color);
        else
            return endgameEvaluator->evaluate(board, color);
    }

private:
    bool queenDetector(const Board& board) {
        chess::Bitboard queens = board.pieces(chess::PieceType::QUEEN);
        return queens == 0 ? false : true;
    }

        
};
