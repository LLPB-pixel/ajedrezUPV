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
    NodeMove rootNode(&board);  // solo una vez
    NodeMove* currentNode = &rootNode;
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

            NodeMove* next = currentNode->getChildByMove(move);
            if (next) {
                currentNode = next;
                currentNode->rebuildUntilDepth(&board);
            } else {
                std::cout << "Movimiento inesperado. Recalculando árbol...\n";
                currentNode = new NodeMove(&board);  // o manejar memoria con smart pointer
            }

            printBoard(board);
        } else {
            std::cout << "Calculando el movimiento del motor...\n";
            float alpha = -99999.0f;
            float beta = 99999.0f;
            std::mutex alphaBetaMutex;

            float bestScore = currentNode->alphaBeta(&evaluator, &alpha, &beta, board.sideToMove(), &board, &alphaBetaMutex);
            chess::Move bestMove = currentNode->getBestMove(bestScore);

            NodeMove* next = currentNode->getChildByMove(bestMove);
            if (next) {
                currentNode = next;
                currentNode->rebuildUntilDepth(&board);
            }

            std::cout << "El motor juega: " << bestMove << "\n";
            board.makeMove(bestMove);
            printBoard(board);
        }
    }

    auto [reason, result] = board.isGameOver();
    std::cout << "El juego ha terminado.\n";

    return 0;
}
