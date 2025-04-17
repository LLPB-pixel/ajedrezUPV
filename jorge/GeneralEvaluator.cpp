#include "GeneralEvaluator.h"
#include <iostream> 

GeneralEvaluator::GeneralEvaluator() {}

float GeneralEvaluator::evaluate(const Board *board, const Color color) {
    // Lleva la formula y suma todas las demas
    return 0.0;
}

float GeneralEvaluator::position(const Board *board, const Color color) {
    return 0.0;
}

float GeneralEvaluator::material(const Board *board, const Color color) {
    float score = 0.0;
    score += board->pieces(PieceType::PAWN, color).count() * PAWN_VALUE;
    score += board->pieces(PieceType::KNIGHT, color).count() * KNIGHT_VALUE;
    score += board->pieces(PieceType::BISHOP, color).count() * BISHOP_VALUE;
    score += board->pieces(PieceType::ROOK, color).count() * ROOK_VALUE;
    score += board->pieces(PieceType::QUEEN, color).count() * QUEEN_VALUE;
    return score;
}   

float GeneralEvaluator::pawn_structure(const Board *board, const Color color) {
    return 0.0;
}

float GeneralEvaluator::safe_king(const Board *board, const Color color) {
    return 0.0;
}

float GeneralEvaluator::mobility(const Board *board, const Color color) {
    return 0.0;
}

GeneralEvaluator::~GeneralEvaluator() {

}
