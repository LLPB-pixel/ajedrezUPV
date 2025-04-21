#include "OpeningEvaluator.h"
#include <iostream>
#include <future>

OpeningEvaluator::OpeningEvaluator() {}
OpeningEvaluator::~OpeningEvaluator() {}

float OpeningEvaluator::evaluate(const Board *board, const Color color) {
    if(board->isGameOver().second == chess::GameResult::WIN) {
        return color == chess::Color::WHITE ? -10000.0f : 10000.0f;
    }
    else if(board->isGameOver().second == chess::GameResult::DRAW) {
        return 0.0f;
    }

    float score = 0.0f;

    auto materialFuture = std::async(&OpeningEvaluator::positionOfThePiecesAndMaterial, this, board);
    auto kingSafetyFutureWe = std::async(&OpeningEvaluator::safe_king, this, board, color);
    auto kingSafetyFutureOpp = std::async(&OpeningEvaluator::safe_king, this, board, ~color);
    auto controlFuture = std::async(&OpeningEvaluator::control, this, board, color);
    auto controlFutureOpp = std::async(&OpeningEvaluator::control, this, board, ~color);
    auto pawnStructureFuture = std::async(&OpeningEvaluator::pawn_structure, this, board, color);
    auto pawnStructureFutureOpp = std::async(&OpeningEvaluator::pawn_structure, this, board, ~color);

    float materialScore = materialFuture.get();
    float kingSafetyScore = kingSafetyFutureWe.get() - kingSafetyFutureOpp.get();
    float controlScore = controlFuture.get() - controlFutureOpp.get();
    float pawnStructureScore = pawnStructureFuture.get() - pawnStructureFutureOpp.get();

    score += 1.0f * materialScore;
    score += 0.2f * kingSafetyScore;
    score += 1.2f * controlScore;
    score += 0.3f * pawnStructureScore;

    return score;
}

float OpeningEvaluator::positionOfThePiecesAndMaterial(const Board *board) {
    float whiteMaterial = 0;
    float blackMaterial = 0;

    whiteMaterial += evaluatePieceType(board, chess::PieceType::PAWN, chess::Color::WHITE, 1.0, white_pawn_heatmap);
    whiteMaterial += evaluatePieceType(board, chess::PieceType::KNIGHT, chess::Color::WHITE, 3.0, minor_pieces_heatmap);
    whiteMaterial += evaluatePieceType(board, chess::PieceType::BISHOP, chess::Color::WHITE, 3.15, minor_pieces_heatmap);
    whiteMaterial += evaluatePieceType(board, chess::PieceType::ROOK, chess::Color::WHITE, 5.0, white_importance);
    whiteMaterial += evaluatePieceType(board, chess::PieceType::QUEEN, chess::Color::WHITE, 9.0, white_importance);

    blackMaterial += evaluatePieceType(board, chess::PieceType::PAWN, chess::Color::BLACK, 1.0, black_pawn_heatmap);
    blackMaterial += evaluatePieceType(board, chess::PieceType::KNIGHT, chess::Color::BLACK, 3.0, minor_pieces_heatmap);
    blackMaterial += evaluatePieceType(board, chess::PieceType::BISHOP, chess::Color::BLACK, 3.15, minor_pieces_heatmap);
    blackMaterial += evaluatePieceType(board, chess::PieceType::ROOK, chess::Color::BLACK, 5.0, black_importance);
    blackMaterial += evaluatePieceType(board, chess::PieceType::QUEEN, chess::Color::BLACK, 9.0, black_importance);

    return whiteMaterial - blackMaterial;
}


