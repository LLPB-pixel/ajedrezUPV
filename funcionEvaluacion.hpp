#include "chess.hpp"


int w1, w2, w3, w4


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



chess::Bitboard getAttackedSquares(const chess::Board & board, chess::Color color){
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






int spaceControl(const Board& board) {
    Bitboard espacio;


    //por conretar
    return 0;
}



int dynamicEvaluation(const Board& board) {
    return 0;
}