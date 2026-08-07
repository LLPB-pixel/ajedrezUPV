#include "chess/node_move_optimized.h"
#include "chess/general_evaluator.h"
#include "chess.hpp"

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>

namespace {

void printBoard(const chess::Board& board) {
    std::cout << "  +-----------------+\n";
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << rank + 1 << " | ";
        for (int file = 0; file < 8; ++file) {
            const chess::Square square(static_cast<chess::File>(file),
                                       static_cast<chess::Rank>(rank));
            const chess::Piece piece = board.at(square);
            std::cout << (piece == chess::Piece()
                              ? ". "
                              : static_cast<std::string>(piece) + " ");
        }
        std::cout << "|\n";
    }
    std::cout << "  +-----------------+\n";
    std::cout << "    a b c d e f g h\n";
}

bool isThisMoveLegal(const chess::Board& board, const chess::Move& move) {
    chess::Movelist legal_moves;
    chess::movegen::legalmoves(legal_moves, board);
    return std::find(legal_moves.begin(), legal_moves.end(), move) !=
           legal_moves.end();
}

} // namespace

int main() {
    chess::Board board(chess::constants::STARTPOS);
    auto root_node = std::make_unique<chess::NodeMoveOptimized>(&board);
    chess::NodeMoveOptimized* current_node = root_node.get();
    GeneralEvaluator evaluator;

    std::cout << "¡Bienvenido al juego de ajedrez!\n";
    printBoard(board);

    while (board.isGameOver().second == chess::GameResult::NONE) {
        if (board.sideToMove() == chess::Color::BLACK) {
            std::string user_move;
            chess::Move move;
            bool illegal_move = true;

            while (illegal_move) {
                std::cout << "Introduce tu movimiento (ejemplo: e2e4): ";
                std::cin >> user_move;

                try {
                    move = chess::uci::uciToMove(board, user_move);
                    if (isThisMoveLegal(board, move)) {
                        illegal_move = false;
                    } else {
                        std::cout << "Movimiento ilegal. Intenta de nuevo.\n";
                    }
                } catch (...) {
                    std::cout << "Movimiento inválido. Intenta de nuevo.\n";
                }
            }

            board.makeMove(move);
            chess::NodeMoveOptimized* next =
                current_node->getChildByMove(move);
            if (next) {
                current_node = next;
                current_node->rebuildUntilDepth(&board);
            } else {
                root_node = std::make_unique<chess::NodeMoveOptimized>(&board);
                current_node = root_node.get();
            }

            printBoard(board);
        } else {
            std::cout << "Calculando el movimiento del motor...\n";

            const float best_score = current_node->alphaBeta(
                &evaluator, board.sideToMove(), &board);
            const chess::Move best_move = current_node->getBestMove(best_score);

            chess::NodeMoveOptimized* next =
                current_node->getChildByMove(best_move);
            if (next) {
                current_node = next;
                board.makeMove(best_move);
                current_node->rebuildUntilDepth(&board);
            } else {
                board.makeMove(best_move);
                root_node = std::make_unique<chess::NodeMoveOptimized>(&board);
                current_node = root_node.get();
            }

            std::cout << "El motor juega: "
                      << chess::uci::moveToUci(best_move) << "\n";
            printBoard(board);
        }
    }

    std::cout << "El juego ha terminado.\n";
    return 0;
}
