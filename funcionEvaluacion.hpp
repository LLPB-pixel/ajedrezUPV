#include "chess.hpp"

constexpr int w1 = 1, w2 = 1, w3 = 1, w4 = 1; // Asignar valores a las constantes

int staticEvaluation(const Board& board) {
    int whiteMaterial = 0;
    int blackMaterial = 0;

    for (const auto& piece : board.getPieces()) {
        if (piece.isWhite()) {
            whiteMaterial += piece.getValue();
        } else {
            blackMaterial += piece.getValue();
        }
    }

    return whiteMaterial - blackMaterial;
}

chess::Bitboard getSeenSquares(const chess::Board & board, chess::Color color) {
    chess::Bitboard attackedSquares = 0ULL;
    chess::Bitboard pieces = board.us(color);

    while (pieces) {
        chess::Square sq = pieces.pop();
        chess::Piece piece = board.at(sq);

        switch (static_cast<chess::PieceType::underlying>(piece.type().internal())) {
            case chess::PieceType::underlying::PAWN:
                attackedSquares |= chess::attacks::pawn(color, sq);
                break;
            case chess::PieceType::underlying::KNIGHT:
                attackedSquares |= chess::attacks::knight(sq);
                break;
            case chess::PieceType::underlying::BISHOP:
                attackedSquares |= chess::attacks::bishop(sq, board.occ());
                break;
            case chess::PieceType::underlying::ROOK:
                attackedSquares |= chess::attacks::rook(sq, board.occ());
                break;
            case chess::PieceType::underlying::QUEEN:
                attackedSquares |= chess::attacks::queen(sq, board.occ());
                break;
            case chess::PieceType::underlying::KING:
                attackedSquares |= chess::attacks::king(sq);
                break;
            default:
                break;
        }
    }

    return attackedSquares; // Corregido: debería devolver attackedSquares
}

chess::Bitboard getControlledSquares(const chess::Board & board, chess::Color color) {
    // Subrutina para obtener las casillas disputadas
    chess::Bitboard thisSeenSquares = getSeenSquares(board, color);
    chess::Bitboard opponentSeenSquares = getSeenSquares(board, ~color);
    chess::Bitboard disputedSquares = thisSeenSquares & opponentSeenSquares;

    while (disputedSquares) {
        chess::Square sq = disputedSquares.pop(); 
        int ownAttackers = 0;
        int opponentAttackers = 0;

        chess::Bitboard attackers = board.attackersTo(sq, color);
        while (attackers) {
            chess::Square attackerSq = attackers.pop();
            chess::Piece attackerPiece = board.at(attackerSq);
            if (attackerPiece.type() == chess::PieceType::PAWN) {
                ownAttackers += 100; // Los peones tienen un peso especial
            } else {
                ownAttackers++;
            }
        }

        attackers = board.attackersTo(sq, ~color);
        while (attackers) {
            chess::Square attackerSq = attackers.pop();
            chess::Piece attackerPiece = board.at(attackerSq);
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

float overallControl(const chess::Board& board) {
    constexpr float white_importance[8][8] = {
        {0.1, 0.2, 0.3, 0.4, 0.4, 0.3, 0.2, 0.1},
        {0.2, 0.4, 0.5, 0.6, 0.6, 0.5, 0.4, 0.2},
        {0.3, 0.5, 0.7, 0.8, 0.8, 0.7, 0.5, 0.3},
        {0.4, 0.6, 0.8, 1.0, 1.0, 0.8, 0.6, 0.4},
        {0.4, 0.6, 0.8, 1.0, 1.0, 0.8, 0.6, 0.4},
        {0.3, 0.5, 0.7, 0.8, 0.8, 0.7, 0.5, 0.3},
        {0.2, 0.4, 0.5, 0.6, 0.6, 0.5, 0.4, 0.2},
        {0.1, 0.2, 0.3, 0.4, 0.4, 0.3, 0.2, 0.1}
    };

    constexpr float black_importance[8][8] = {
        {0.1, 0.2, 0.3, 0.4, 0.4, 0.3, 0.2, 0.1},
        {0.2, 0.4, 0.5, 0.6, 0.6, 0.5, 0.4, 0.2},
        {0.3, 0.5, 0.7, 0.8, 0.8, 0.7, 0.5, 0.3},
        {0.4, 0.6, 0.8, 1.0, 1.0, 0.8, 0.6, 0.4},
        {0.4, 0.6, 0.8, 1.0, 1.0, 0.8, 0.6, 0.4},
        {0.3, 0.5, 0.7, 0.8, 0.8, 0.7, 0.5, 0.3},
        {0.2, 0.4, 0.5, 0.6, 0.6, 0.5, 0.4, 0.2},
        {0.1, 0.2, 0.3, 0.4, 0.4, 0.3, 0.2, 0.1}
    };

    chess::Bitboard whiteControlledSquares = getControlledSquares(board, chess::Color::WHITE);
    chess::Bitboard blackControlledSquares = getControlledSquares(board, chess::Color::BLACK);

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


int dynamicEvaluation(const Board& board) {
    return 0;
}
