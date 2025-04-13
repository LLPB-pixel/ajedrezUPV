#pragma once
#include "chess.hpp"
#include "classEvaluator.hpp"

using namespace chess;
class GeneralEvaluator : public Evaluator {
// Asignar valores para cada pieza para obtener el control del tablero
public: 

    float evaluate(const Board *board, Color color) override {
        auto [reason, result] = board->isGameOver();
        if (result == chess::GameResult::NONE){
            float materialValue = staticEvaluation(board);
            float controlValue = overallControl(board);
            float totalValue = materialValue + controlValue;
            return totalValue;
        }
        else if (result == chess::GameResult::WIN){
            return (board->sideToMove() == color) ? -100000.0f : 100000.0f;
        } 
        else {
            return 0.0f;
        }
    }

private:
int staticEvaluation(const Board *board) {
    float whiteMaterial = 0;
    float blackMaterial = 0;

    constexpr float white_importance[8][8] = {
        {0.025, 0.05, 0.075, 0.1, 0.1, 0.075, 0.05, 0.025},
        {0.05, 0.1, 0.125, 0.15, 0.15, 0.125, 0.1, 0.05},
        {0.075, 0.125, 0.175, 0.2, 0.2, 0.175, 0.125, 0.075},
        {0.1, 0.15, 0.2, 0.25, 0.25, 0.2, 0.15, 0.1},
        {0.1, 0.15, 0.2, 0.25, 0.25, 0.2, 0.15, 0.1},
        {0.075, 0.125, 0.175, 0.2, 0.2, 0.175, 0.125, 0.075},
        {0.05, 0.1, 0.125, 0.15, 0.15, 0.125, 0.1, 0.05},
        {0.025, 0.05, 0.075, 0.1, 0.1, 0.075, 0.05, 0.025}
    };

    constexpr float black_importance[8][8] = {
        {0.025, 0.05, 0.075, 0.1, 0.1, 0.075, 0.05, 0.025},
        {0.05, 0.1, 0.125, 0.15, 0.15, 0.125, 0.1, 0.05},
        {0.075, 0.125, 0.175, 0.2, 0.2, 0.175, 0.125, 0.075},
        {0.1, 0.15, 0.2, 0.25, 0.25, 0.2, 0.15, 0.1},
        {0.1, 0.15, 0.2, 0.25, 0.25, 0.2, 0.15, 0.1},
        {0.075, 0.125, 0.175, 0.2, 0.2, 0.175, 0.125, 0.075},
        {0.05, 0.1, 0.125, 0.15, 0.15, 0.125, 0.1, 0.05},
        {0.025, 0.05, 0.075, 0.1, 0.1, 0.075, 0.05, 0.025}
    };

    // Iterar sobre piezas blancas
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
    //Terminada
    float overallControl(const Board *board) {
        constexpr float white_importance[8][8] = {
            {0.025, 0.05, 0.075, 0.1, 0.1, 0.075, 0.05, 0.025},
            {0.05, 0.1, 0.125, 0.15, 0.15, 0.125, 0.1, 0.05},
            {0.075, 0.125, 0.175, 0.2, 0.2, 0.175, 0.125, 0.075},
            {0.1, 0.15, 0.2, 0.25, 0.25, 0.2, 0.15, 0.1},
            {0.1, 0.15, 0.2, 0.25, 0.25, 0.2, 0.15, 0.1},
            {0.075, 0.125, 0.175, 0.2, 0.2, 0.175, 0.125, 0.075},
            {0.05, 0.1, 0.125, 0.15, 0.15, 0.125, 0.1, 0.05},
            {0.025, 0.05, 0.075, 0.1, 0.1, 0.075, 0.05, 0.025}
        };
        constexpr float black_importance[8][8] = {
            {0.025, 0.05, 0.075, 0.1, 0.1, 0.075, 0.05, 0.025},
            {0.05, 0.1, 0.125, 0.15, 0.15, 0.125, 0.1, 0.05},
            {0.075, 0.125, 0.175, 0.2, 0.2, 0.175, 0.125, 0.075},
            {0.1, 0.15, 0.2, 0.25, 0.25, 0.2, 0.15, 0.1},
            {0.1, 0.15, 0.2, 0.25, 0.25, 0.2, 0.15, 0.1},
            {0.075, 0.125, 0.175, 0.2, 0.2, 0.175, 0.125, 0.075},
            {0.05, 0.1, 0.125, 0.15, 0.15, 0.125, 0.1, 0.05},
            {0.025, 0.05, 0.075, 0.1, 0.1, 0.075, 0.05, 0.025}
        };

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
        //Bonus por estyar cerca del rey negro el blanco
        while (nextToBlackKing) {
            whiteControlledSquares.pop();
            whiteControl += 0.25; //Si queréis lo cambiamos
        }

        while (blackControlledSquares) {
            chess::Square sq = blackControlledSquares.pop();
            int rank = sq.rank();
            int file = sq.file();
            blackControl += black_importance[rank][file];
        }
        while (nextToWhiteKing) {
            whiteControlledSquares.pop();
            whiteControl += 0.25; //Si quereis lo cambiamos
        }
        return whiteControl - blackControl;
    }
};