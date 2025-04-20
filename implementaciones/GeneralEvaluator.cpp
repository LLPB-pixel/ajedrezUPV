#include "GeneralEvaluator.h"
#include <iostream> 

using namespace chess;
GeneralEvaluator::GeneralEvaluator() {}

float GeneralEvaluator::evaluate(const Board *board, const Color color) {
    if(board->isGameOver().second == chess::GameResult::WIN) {
        return color == chess::Color::WHITE ? -10000.0f : 10000.0f;
    }
    else if(board->isGameOver().second == chess::GameResult::DRAW) {
        return 0.0f;
    }
    float score = 0.0f;

    // Diferencia de material y posición
    float materialScore = positionOfThePiecesAndMaterial(board);
    // Seguridad del rey
    float kingSafetyScore = safe_king(board, color) - safe_king(board, ~color);
    // Estructura de peones

    float pawnStructureScore = 0.0f;
    // Control de casillas importantes

    float controlScore = control(board, color) - control(board, ~color);
    // Suma ponderada (puedes ajustar los pesos según resultados de pruebas)
    score += 1.0f * materialScore;
    score += 0.05f * kingSafetyScore;
    score += 0.05f * pawnStructureScore;
    score += 1.0f * controlScore;

    return score;
}

float GeneralEvaluator::positionOfThePiecesAndMaterial(const Board *board) {
    float whiteMaterial = 0;
    float blackMaterial = 0;

    whiteMaterial += evaluatePieceType(board, chess::PieceType::PAWN, chess::Color::WHITE, 1.0, white_importance);
    whiteMaterial += evaluatePieceType(board, chess::PieceType::KNIGHT, chess::Color::WHITE, 3.0, white_importance);
    whiteMaterial += evaluatePieceType(board, chess::PieceType::BISHOP, chess::Color::WHITE, 3.15, white_importance);
    whiteMaterial += evaluatePieceType(board, chess::PieceType::ROOK, chess::Color::WHITE, 5.0, white_importance);
    whiteMaterial += evaluatePieceType(board, chess::PieceType::QUEEN, chess::Color::WHITE, 9.0, white_importance);

    blackMaterial += evaluatePieceType(board, chess::PieceType::PAWN, chess::Color::BLACK, 1.0, black_importance);
    blackMaterial += evaluatePieceType(board, chess::PieceType::KNIGHT, chess::Color::BLACK, 3.0, black_importance);
    blackMaterial += evaluatePieceType(board, chess::PieceType::BISHOP, chess::Color::BLACK, 3.15, black_importance);
    blackMaterial += evaluatePieceType(board, chess::PieceType::ROOK, chess::Color::BLACK, 5.0, black_importance);
    blackMaterial += evaluatePieceType(board, chess::PieceType::QUEEN, chess::Color::BLACK, 9.0, black_importance);

    return whiteMaterial - blackMaterial;
}

float GeneralEvaluator::evaluatePieceType(
    const Board* board,
    chess::PieceType type,
    chess::Color color,
    float baseValue,
    const float importance[8][8]
) {
    float material = 0.0;
    chess::Bitboard pieces = board->pieces(type, color);
    while (pieces) {
        chess::Square sq = pieces.pop();
        int rank = sq.rank();
        int file = sq.file();
        if (type == chess::PieceType::PAWN) {
            material += 0.95 + 0.05 * (color == chess::Color::WHITE ? rank : (7 - rank)); // no tocar fdo: LLORENÇ
        } else {
            material += baseValue + importance[rank][file];
        }
    }
    return material;
}

 


float GeneralEvaluator::safe_king(const chess::Board *board, chess::Color color) {
    chess::Color oppositeColor = (color == chess::Color::WHITE) ? chess::Color::BLACK : chess::Color::WHITE;

    chess::Bitboard theOtherSidesControll = getControlledSquares(board, oppositeColor);
    chess::Bitboard theOtherSidesSeen = getSeenSquares(board, oppositeColor);
    chess::Bitboard enemySeenIgnoringOwnPieces = getEnemySeenSquaresIgnoringOwnPieces(board, oppositeColor);

    chess::Square kingSquare = board->kingSq(color);
    chess::Bitboard nextToKing = chess::attacks::king(kingSquare);

    chess::Bitboard controlledAroundKing = nextToKing & theOtherSidesControll;
    float controlBonus = static_cast<float>(controlledAroundKing.count()) * 0.5f;

    chess::Bitboard seenAroundKing = nextToKing & theOtherSidesSeen;
    float seenBonus = static_cast<float>(seenAroundKing.count()) * 0.3f;

    chess::Bitboard seenIgnoringOwnAroundKing = nextToKing & enemySeenIgnoringOwnPieces;
    float ignoringOwnBonus = static_cast<float>(seenIgnoringOwnAroundKing.count()) * 0.2f;

    
    chess::Bitboard kingSquareBitboard = chess::Bitboard::fromSquare(kingSquare);
    bool kingSquareSeenIgnoringOwn = (enemySeenIgnoringOwnPieces & kingSquareBitboard).getBits() != 0;
    float kingSquarePenalty = kingSquareSeenIgnoringOwn ? -1.0f : 0.0f;

    float kingSafety = static_cast<float>(nextToKing.count() - theOtherSidesSeen.count());
    kingSafety += controlBonus + seenBonus + ignoringOwnBonus + kingSquarePenalty;

    return kingSafety;
}

float GeneralEvaluator::control(const Board *board, const Color color) {
    chess::Square kingSquareWhite = board->kingSq(chess::Color::WHITE);
    chess::Square kingSquareBlack = board->kingSq(chess::Color::BLACK);

    chess::Bitboard nextToWhiteKing = chess::attacks::king(kingSquareWhite);
    chess::Bitboard nextToBlackKing = chess::attacks::king(kingSquareBlack);

    chess::Bitboard whiteControlledSquares = getControlledSquares(board, chess::Color::WHITE);
    chess::Bitboard blackControlledSquares = getControlledSquares(board, chess::Color::BLACK);

    nextToWhiteKing &= whiteControlledSquares;
    nextToBlackKing &= blackControlledSquares;
    float whiteControl = 0.0;
    float blackControl = 0.0;


    while (whiteControlledSquares) {
        chess::Square sq = whiteControlledSquares.pop();
        int rank = sq.rank();
        int file = sq.file();
        whiteControl += white_importance[rank][file];
    }
    while (blackControlledSquares) {
        chess::Square sq = blackControlledSquares.pop();
        int rank = sq.rank();
        int file = sq.file();
        blackControl += black_importance[rank][file];
    }
        
    return whiteControl - blackControl;
}

float GeneralEvaluator::pawn_structure(const chess::Board *board, chess::Color color) {
    chess::Bitboard pawns = board->pieces(chess::PieceType::PAWN, color);
    float score = 0.0f;

    while (pawns) {
        chess::Square pawnSquare = pawns.pop();
        chess::File file = pawnSquare.file();

        auto adjacentFiles = chess::Bitboard(chess::File(file - 1)) | chess::Bitboard(chess::File(file + 1));
        if ((board->pieces(chess::PieceType::PAWN, color) & adjacentFiles).empty()) {
            score -= 0.5f; // Penalización por peones aislados
        }

        if ((board->pieces(chess::PieceType::PAWN, color) & chess::Bitboard(file)).count() > 1) {
            score -= 0.5f; // Penalización por peones doblados
        }

        chess::Bitboard opposingPawns = board->pieces(chess::PieceType::PAWN, ~color);
        chess::Bitboard passedMask = chess::Bitboard(file) | chess::Bitboard(chess::File(file - 1)) | chess::Bitboard(chess::File(file + 1));
        passedMask = (color == chess::Color::WHITE) ? passedMask << 8 : passedMask >> 8;
        if ((opposingPawns & passedMask).empty()) {
            score += 1.0f; // Bonificación por peones pasados
        }
    }

    return std::clamp(score, 0.0f, 2.0f);
}

GeneralEvaluator::~GeneralEvaluator() {

}

chess::Bitboard GeneralEvaluator::getSeenSquares(const chess::Board *board, chess::Color color) {
    chess::Bitboard attackedSquares = 0ULL;
    chess::Bitboard pieces = board->us(color);

    while (pieces) {
        chess::Square sq = pieces.pop();
        chess::Piece piece = board->at(sq);

        switch (piece.type().internal()) { 
            case chess::PieceType::PAWN:
                attackedSquares |= chess::attacks::pawn(color, sq);
                break;
            case chess::PieceType::KNIGHT:
                attackedSquares |= chess::attacks::knight(sq);
                break;
            case chess::PieceType::BISHOP:
                attackedSquares |= chess::attacks::bishop(sq, board->occ());
                break;
            case chess::PieceType::ROOK:
                attackedSquares |= chess::attacks::rook(sq, board->occ());
                break;
            case chess::PieceType::QUEEN:
                attackedSquares |= chess::attacks::queen(sq, board->occ());
                break;
            case chess::PieceType::KING:
                attackedSquares |= chess::attacks::king(sq);
                break;
            default:
                break;
        }
    }

    return attackedSquares;
}

chess::Bitboard GeneralEvaluator::getControlledSquares(const chess::Board *board, chess::Color color) {
    // Subrutina para obtener las casillas disputadas
    // arreglar ordenes de magnitud 

    chess::Bitboard thisSeenSquares = getSeenSquares(board, color);
    chess::Bitboard opponentSeenSquares = getSeenSquares(board, ~color);
    chess::Bitboard disputedSquares = thisSeenSquares & opponentSeenSquares;

    while (disputedSquares) {
        chess::Square sq = disputedSquares.pop();
        int ownAttackers = 0;
        int opponentAttackers = 0;

        //Revisar logica
        chess::Bitboard attackers = 0;
        attackers |= chess::attacks::pawn(~color, sq) & board->pieces(chess::PieceType::PAWN, color);
        attackers |= chess::attacks::knight(sq) & board->pieces(chess::PieceType::KNIGHT, color);
        attackers |= chess::attacks::bishop(sq, board->occ()) & (board->pieces(chess::PieceType::BISHOP, color) | board->pieces(chess::PieceType::QUEEN, color));
        attackers |= chess::attacks::rook(sq, board->occ()) & (board->pieces(chess::PieceType::ROOK, color) | board->pieces(chess::PieceType::QUEEN, color));
        attackers |= chess::attacks::king(sq) & board->pieces(chess::PieceType::KING, color);
        while (attackers) {
            chess::Square attackerSq = attackers.pop();
            chess::Piece attackerPiece = board->at(attackerSq);
            if (attackerPiece.type() == chess::PieceType::PAWN) {
                ownAttackers += 100; // Los peones tienen un peso especial
            } else {
                ownAttackers++;
            }
        }

        //Reviasr logica
        attackers |= chess::attacks::pawn(color, sq) & board->pieces(chess::PieceType::PAWN, ~color);
        attackers |= chess::attacks::knight(sq) & board->pieces(chess::PieceType::KNIGHT, ~color);
        attackers |= chess::attacks::bishop(sq, board->occ()) & (board->pieces(chess::PieceType::BISHOP, ~color) | board->pieces(chess::PieceType::QUEEN, ~color));
        attackers |= chess::attacks::rook(sq, board->occ()) & (board->pieces(chess::PieceType::ROOK, ~color) | board->pieces(chess::PieceType::QUEEN, ~color));
        attackers |= chess::attacks::king(sq) & board->pieces(chess::PieceType::KING, ~color);


        while (attackers) {
            chess::Square attackerSq = attackers.pop();
            chess::Piece attackerPiece = board->at(attackerSq);
            if (attackerPiece.type() == chess::PieceType::PAWN) {
                opponentAttackers += 100; // Los peones tienen un peso especial
            } else {
                opponentAttackers++;
            }
        }

        if ((ownAttackers <= opponentAttackers && ownAttackers > 0) || opponentAttackers >= 100) {
            thisSeenSquares.clear(sq.index());
        }
    }

    return thisSeenSquares;
}

chess::Bitboard GeneralEvaluator::getEnemySeenSquaresIgnoringOwnPieces(const Board *board, Color enemyColor) {
    // Crear un tablero temporal sin las piezas propias
    chess::Bitboard enemyPieces = board->us(enemyColor);
    chess::Bitboard emptyBoard = board->occ() ^ board->us(~enemyColor); // Eliminar las piezas propias
    
    chess::Bitboard seenSquares = 0ULL;
    
    while (enemyPieces) {
        chess::Square sq = enemyPieces.pop();
        chess::Piece piece = board->at(sq);
    
        switch (piece.type().internal()) {
            case chess::PieceType::PAWN:
                seenSquares |= chess::attacks::pawn(enemyColor, sq);
                break;
            case chess::PieceType::KNIGHT:
                seenSquares |= chess::attacks::knight(sq);
                break;
            case chess::PieceType::BISHOP:
                seenSquares |= chess::attacks::bishop(sq, emptyBoard);
                break;
            case chess::PieceType::ROOK:
                seenSquares |= chess::attacks::rook(sq, emptyBoard);
                break;
            case chess::PieceType::QUEEN:
                seenSquares |= chess::attacks::queen(sq, emptyBoard);
                break;
            case chess::PieceType::KING:
                seenSquares |= chess::attacks::king(sq);
                break;
            default:
                break;
        }
    }

    return seenSquares;
}


