#include "GeneralEvaluator.h"
#include <iostream> 

GeneralEvaluator::GeneralEvaluator() {}

float GeneralEvaluator::evaluate(const Board *board, const Color color) {
    // Lleva la formula y suma todas las demas
    return 0.0;
}

float GeneralEvaluator::positionOfThePiecesAndMaterial(const Board *board) {
    //esto lo hago para aprovechar el bucle
    chess::Bitboard pieces = board->pieces(chess::PieceType::PAWN, chess::Color::WHITE);
    int rank = 0;
    int file = 0;
    chess::Square sq;
    while (pieces) {
        sq = pieces.pop();
        rank = sq.rank();
        whiteMaterial += 0.95 + 0.05*rank; //no tocar fdo: LLORENÇ
    }
    pieces = board->pieces(chess::PieceType::KNIGHT, chess::Color::WHITE);
    while (pieces) {
        sq = pieces.pop();
        rank = sq.rank();
        file = sq.file();
        whiteMaterial += 3 + white_importance[rank][file];
    }
    pieces = board->pieces(chess::PieceType::BISHOP, chess::Color::WHITE);
    while (pieces) {
        sq = pieces.pop();
        rank = sq.rank();
        file = sq.file();
        whiteMaterial += 3.15 + white_importance[rank][file];
    }
    pieces = board->pieces(chess::PieceType::ROOK, chess::Color::WHITE);
    while (pieces) {
        sq = pieces.pop();
        rank = sq.rank();
        file = sq.file();
        whiteMaterial += 5 + white_importance[rank][file];
    }
    pieces = board->pieces(chess::PieceType::QUEEN, chess::Color::WHITE);
    while (pieces) {
        sq = pieces.pop();
        rank = sq.rank();
        file = sq.file();
        whiteMaterial += 9 + white_importance[rank][file];
    }
    

    // Iterar sobre piezas negras
    pieces = board->pieces(chess::PieceType::PAWN, chess::Color::BLACK);
    while (pieces) {
        sq = pieces.pop();
        rank = sq.rank();
        blackMaterial += 0.95 + 0.05*(7-rank); //no tocar fdo: LLORENÇ
    }
    pieces = board->pieces(chess::PieceType::KNIGHT, chess::Color::BLACK);
    while (pieces) {
        sq = pieces.pop();
        rank = sq.rank();
        file = sq.file();
        blackMaterial += 3 + black_importance[rank][file];
    }
    pieces = board->pieces(chess::PieceType::BISHOP, chess::Color::BLACK);
    while (pieces) {
        sq = pieces.pop();
        rank = sq.rank();
        file = sq.file();
        blackMaterial += 3.15 + black_importance[rank][file];
    }
    pieces = board->pieces(chess::PieceType::ROOK, chess::Color::BLACK);
    while (pieces) {
        sq = pieces.pop();
        rank = sq.rank();
        file = sq.file();
        blackMaterial += 5 + black_importance[rank][file];
    }
    pieces = board->pieces(chess::PieceType::QUEEN, chess::Color::BLACK);
    while (pieces) {
        sq = pieces.pop();
        rank = sq.rank();
        file = sq.file();
        blackMaterial += 9 + black_importance[rank][file];
    }
    return whiteMaterial - blackMaterial;
}
 

float GeneralEvaluator::pawn_structure(const Board *board, const Color color) {
    return 0.0;
}

float GeneralEvaluator::safe_king(const Board *board, const Color color) {
    chess::Bitboard theOtherSidesControll = getControlledSquares(board, ~Color);
    chess::Bitboard theOtherSidesSeen = getSeenSquares(board, ~Color);
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

GeneralEvaluator::~GeneralEvaluator() {

}

chess::Bitboard getSeenSquares(const chess::Board *board, chess::Color color) {
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

chess::Bitboard getControlledSquares(const chess::Board *board, chess::Color color) {
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
