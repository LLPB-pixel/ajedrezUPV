#include "chess.hpp"

int w1, w2, w3, w4;

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



chess::Bitboard getSeenSquares(const chess::Board & board, chess::Color color){
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

    return attackedSquares;
}

chess::Bitboard getDisputedSquares(const chess::Board & board, chess::Color color){
    chess::Bitboard whiteSeenSquares = getSeenSquares(board, chess::Color::WHITE);
    chess::Bitboard blackSeenSquares = getSeenSquares(board, chess::Color::BLACK);
    chess::Bitboard disputedSquares = whiteSeenSquares & blackSeenSquares;
    return disputedSquares;
}



float overallControl(const Board& board) {
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

    const float black_importance[8][8] = {
        {0.1, 0.2, 0.3, 0.4, 0.4, 0.3, 0.2, 0.1},
        {0.2, 0.4, 0.5, 0.6, 0.6, 0.5, 0.4, 0.2},
        {0.3, 0.5, 0.7, 0.8, 0.8, 0.7, 0.5, 0.3},
        {0.4, 0.6, 0.8, 1.0, 1.0, 0.8, 0.6, 0.4},
        {0.4, 0.6, 0.8, 1.0, 1.0, 0.8, 0.6, 0.4},
        {0.3, 0.5, 0.7, 0.8, 0.8, 0.7, 0.5, 0.3},
        {0.2, 0.4, 0.5, 0.6, 0.6, 0.5, 0.4, 0.2},
        {0.1, 0.2, 0.3, 0.4, 0.4, 0.3, 0.2, 0.1}
    };
    return 0.0;
}  



int dynamicEvaluation(const Board& board) {
    return 0;
}
