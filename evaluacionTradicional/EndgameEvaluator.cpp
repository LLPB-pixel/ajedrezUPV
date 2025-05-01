#include "EndgameEvaluator.h"
#include <future>
EndgameEvaluator::EndgameEvaluator() {}

float EndgameEvaluator::evaluate(const Board *board, const Color color) {
    if (board->isGameOver().second == chess::GameResult::WIN) {
        return color == chess::Color::WHITE ? -10000.0f : 10000.0f;
    } else if (board->isGameOver().second == chess::GameResult::DRAW) {
        return 0.0f;
    }

    float score = 0.0f;

    // Paralelizar las evaluaciones
    auto materialFuture = std::async(&EndgameEvaluator::positionOfThePiecesAndMaterial, this, board);
    auto kingSafetyFutureWe = std::async(&EndgameEvaluator::safe_king, this, board, color);
    auto kingSafetyFutureOpp = std::async(&EndgameEvaluator::safe_king, this, board, ~color);
    auto controlFuture = std::async(&EndgameEvaluator::control, this, board, color);
    auto controlFutureOpp = std::async(&EndgameEvaluator::control, this, board, ~color);
    //auto pawnStructureFutureWe = std::async(&EndgameEvaluator::pawn_structure, this, board, color);
    //auto pawnStructureFutureOpp = std::async(&EndgameEvaluator::pawn_structure, this, board, ~color);

    // Obtener los resultados
    float materialScore = materialFuture.get();
    float kingSafetyScore = kingSafetyFutureWe.get() - kingSafetyFutureOpp.get();
    float controlScore = controlFuture.get() - controlFutureOpp.get();
    //float pawnStructureScore = pawnStructureFutureWe.get() - pawnStructureFutureOpp.get();


    score += 1.0f * materialScore;
    score += 0.01f * kingSafetyScore;
    score += 0.01f * controlScore;
    //score += 1.2f * pawnStructureScore;

    return score;
}

float EndgameEvaluator::positionOfThePiecesAndMaterial(const Board *board) {
    float whiteMaterial = 0;
    float blackMaterial = 0;

    whiteMaterial += evaluatePieceType(board, chess::PieceType::PAWN, chess::Color::WHITE, 1.0, pawn_endgame_heatmap);
    blackMaterial += evaluatePieceType(board, chess::PieceType::PAWN, chess::Color::BLACK, 1.0, pawn_endgame_heatmap);

    whiteMaterial += evaluatePieceType(board, chess::PieceType::KNIGHT, chess::Color::WHITE, 3.0, white_importance);
    whiteMaterial += evaluatePieceType(board, chess::PieceType::BISHOP, chess::Color::WHITE, 3.15, white_importance);
    whiteMaterial += evaluatePieceType(board, chess::PieceType::ROOK, chess::Color::WHITE, 5.0, white_importance);
    whiteMaterial += evaluatePieceType(board, chess::PieceType::QUEEN, chess::Color::WHITE, 9.0, white_importance);
    whiteMaterial += evaluatePieceType(board, chess::PieceType::KING, chess::Color::WHITE, 0.0, king_endgame_heatmap);

    blackMaterial += evaluatePieceType(board, chess::PieceType::KNIGHT, chess::Color::BLACK, 3.0, black_importance);
    blackMaterial += evaluatePieceType(board, chess::PieceType::BISHOP, chess::Color::BLACK, 3.15, black_importance);
    blackMaterial += evaluatePieceType(board, chess::PieceType::ROOK, chess::Color::BLACK, 5.0, black_importance);
    blackMaterial += evaluatePieceType(board, chess::PieceType::QUEEN, chess::Color::BLACK, 9.0, black_importance);
    blackMaterial += evaluatePieceType(board, chess::PieceType::KING, chess::Color::BLACK, 0.0, king_endgame_heatmap);

    return whiteMaterial - blackMaterial;
}

EndgameEvaluator::~EndgameEvaluator() {}
