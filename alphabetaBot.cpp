#include "NodeMove.h"
#include "GeneralEvaluator.h"
#include "chess.hpp"
#include <iostream>
#include <string>
#include <mutex>

void printBoard(const chess::Board& board) {
    std::cout << "  +-----------------+\n";
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << rank + 1 << " | ";
        for (int file = 0; file < 8; ++file) {
            chess::Square square(static_cast<chess::File>(file), 
                            static_cast<chess::Rank>(rank));
            chess::Piece piece = board.at(square);
            std::cout << (piece == chess::Piece() ? ". " : static_cast<std::string>(piece) + " ");
        }
        std::cout << "|\n";
    }
    std::cout << "  +-----------------+\n";
    std::cout << "    a b c d e f g h\n";
}
bool isThisMoveLegal(const chess::Board& board, const chess::Move& move) {
    chess::Movelist legalMoves;
    chess::movegen::legalmoves(legalMoves, board);
    return std::find(legalMoves.begin(), legalMoves.end(), move) != legalMoves.end();
}

int main() {
    chess::Board board(chess::constants::STARTPOS);
    NodeMove* rootNode(&board, nullptr);
    GeneralEvaluator evaluator;

    std::cout << "¡Bienvenido al juego de ajedrez!\n";
    printBoard(board);

    while (board.isGameOver().second == chess::GameResult::NONE) {
        if (board.sideToMove() == chess::Color::BLACK) {
            // Human player turn
            std::string userMove;
            chess::Move move;
            bool illegalMove = true;
            
            while (illegalMove) {
                std::cout << "Introduce tu movimiento (ejemplo: e2e4): ";
                std::cin >> userMove;

                try {
                    move = chess::uci::uciToMove(board, userMove);
                    if (isThisMoveLegal(board, move)) {
                        illegalMove = false;
                    } else {
                        std::cout << "Movimiento ilegal. Intenta de nuevo.\n";
                    }
                } catch (...) {
                    std::cout << "Movimiento inválido. Intenta de nuevo.\n";
                }
            }

            board.makeMove(move);

            printBoard(board);
        } else {
            // Engine turn
            std::cout << "Calculando el movimiento del motor...\n";

            
            float alpha = -99999.0f;
            float beta = 99999.0f;

            // Llamada al algoritmo Alpha-Beta
            std::mutex alphaBetaMutex;
            float bestScore = rootNode->alphaBeta(&evaluator, &alpha, &beta, board.sideToMove(), &board, &alphaBetaMutex);
            chess::Move bestMove = rootNode->getBestMove(bestScore);
            NodeMove* rootNode = rootNode->getChildByMove(bestMove);
            rootNode->rebuildUntilDepth(&board);

            
            std::cout << "El motor juega: " << bestMove << "\n";
            board.makeMove(bestMove);
            printBoard(board);
            
        }
    }

    // Game over
    auto [reason, result] = board.isGameOver();
    std::cout << "El juego ha terminado. ";
    

    return 0;
}
