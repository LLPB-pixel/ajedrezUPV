#include "chess.hpp"
#include "classGeneralEvaluator.hpp"
#include "classEvaluator.hpp"
#include "bot.hpp"

#include <iostream>
#include <string>
#include <algorithm>
using namespace chess;

bool isThisMoveLegal(const chess::Board& board, const chess::Move& move) {
    chess::Movelist legalMoves;
    chess::movegen::legalmoves(legalMoves, board);
    return std::find(legalMoves.begin(), legalMoves.end(), move) != legalMoves.end();
}
void printBoard(const chess::Board& board) {
    std::cout << "  +-----------------+" << std::endl;
    for (int rank = 7; rank >= 0; --rank) { 
        std::cout << rank + 1 << " | "; 
        for (int file = 0; file < 8; ++file) { 
            chess::Square square(static_cast<chess::File::underlying>(file), static_cast<chess::Rank::underlying>(rank));
            chess::Piece piece = board.at(square);
            std::cout << static_cast<std::string>(piece) << " "; 
        }
        std::cout << "|" << std::endl;
    }
    std::cout << "  +-----------------+" << std::endl;
    std::cout << "    a b c d e f g h" << std::endl;
}

int main() {
    Board board;
    GeneralEvaluator eval;
    Bot bot(board, &eval, /*maxdepth=*/5);

    std::string userInput;
    chess::GameResult result = chess::GameResult::NONE;
    while (result == chess::GameResult::NONE) {
        printBoard(board);
        std::cout << "Turno de: " << (board.sideToMove() == Color::WHITE ? "Blancas" : "Negras") << std::endl;

        if (board.sideToMove() == Color::WHITE) {
            // Juega el humano (blancas)
            std::cout << "Your move: ";
            std::cin >> userInput;

            Move userMove = uci::uciToMove(board, userInput);
            if (isThisMoveLegal(board, userMove)) {
                bot.makemove(userMove);
                board.makeMove(userMove);
            } else {
                std::cout << "Movimiento ilegal.\n";
            }
        } else {
            //HASTA AQUÍ TODO OK
            Move best = bot.findBestMove(5); // profundidad deseada
            std::cout << "Bot juega: " << best << "\n";
            bot.makemove(best);
        }
        std::tie(std::ignore, result) = board.isGameOver();
    }

    std::cout << board << std::endl;
    std::cout << "Fin de la partida.\n";
    return 0;
}


