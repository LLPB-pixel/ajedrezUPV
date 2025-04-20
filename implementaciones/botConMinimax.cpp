#include "NodeMove.h"
#include "GeneralEvaluator.h"
#include "chess.hpp"
#include <iostream>
#include <string>

void printBoard(const chess::Board& board) {
    std::cout << "  +-----------------+" << std::endl;
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << rank + 1 << " | ";
        for (int file = 0; file < 8; ++file) {
            chess::Square square = chess::Square(rank * 8 + file);
            chess::Piece piece = board.at(square);

            if (piece == chess::Piece()) { // Cambiado para manejar piezas vacías
                std::cout << ". ";
            } else {
                std::cout << static_cast<std::string>(piece) << " ";
            }
        }
        std::cout << "|" << std::endl;
    }
    std::cout << "  +-----------------+" << std::endl;
    std::cout << "    a b c d e f g h" << std::endl;
}

int main() {
    // Carga la posición inicial del tablero
    chess::Board board(chess::constants::STARTPOS);
    GeneralEvaluator evaluator;

    std::cout << "¡Bienvenido al juego de ajedrez!" << std::endl;
    printBoard(board);

    while (board.isGameOver().second == chess::GameResult::NONE) {
        // Turno del usuario
        if (board.sideToMove() == chess::Color::WHITE) {
            std::string userMove;
            chess::Move move;
            bool illegalMove = true;
            while (illegalMove) {
                std::cout << "Introduce tu movimiento (ejemplo: e2e4): ";
                std::cin >> userMove;

                try {
                    move = chess::uci::uciToMove(board, userMove);
                    illegalMove = false;
                } catch (...) {
                    std::cout << "Movimiento ilegal. Intenta de nuevo." << std::endl;
                }
            }

            board.makeMove(move);
            printBoard(board);
        } else {
            // Turno del motor
            std::cout << "Calculando el movimiento del motor..." << std::endl;

            NodeMove rootNode(board, nullptr);
            rootNode.minimax(&evaluator, board.sideToMove());

            // Encuentra el mejor movimiento
            float bestScore = rootNode.getEval();
            chess::Move bestMove;
            for (int i = 0; i < rootNode.getChildIndex(); ++i) {
                NodeMove* child = rootNode.getChild(i);
                if (child != nullptr && child->getEval() == bestScore) {
                    bestMove = child->getLastMove();
                    break;
                }
            }

            std::cout << "El motor juega: " << bestMove << std::endl;
            board.makeMove(bestMove);
            printBoard(board);
        }
    }

    // Fin del juego
    auto [reason, result] = board.isGameOver();
    std::cout << "El juego ha terminado. ";
    if (result == chess::GameResult::WIN) {
        std::cout << (board.sideToMove() == chess::Color::WHITE ? "Negras" : "Blancas") << " ganan por jaque mate." << std::endl;
    } else if (result == chess::GameResult::DRAW) {
        std::cout << "Empate." << std::endl;
    }

    return 0;
}