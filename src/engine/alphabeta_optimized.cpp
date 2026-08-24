#include "chess/node_move_optimized.h"
#include "chess/general_evaluator.h"
#include "chess/search.h"
#include "chess.hpp"

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <csignal>

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
    if (move == chess::Move()) return false; // [Fix] NO_MOVE is never legal.
    chess::Movelist legal_moves;
    chess::movegen::legalmoves(legal_moves, board);
    return std::find(legal_moves.begin(), legal_moves.end(), move) !=
           legal_moves.end();
}

// Convert user input to lowercase for case-insensitive matching.
std::string toLower(const std::string& s) {
    std::string result = s;
    for (char& c : result) {
        if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
    }
    return result;
}

} // namespace

int main(int argc, char* argv[]) {
    // Parse optional FEN and depth arguments.
    std::string fen = chess::constants::STARTPOS;
    int engine_depth = 3;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--fen" && i + 1 < argc) {
            fen = argv[++i];
        } else if (arg == "--depth" && i + 1 < argc) {
            engine_depth = std::max(1, std::min(std::stoi(argv[++i]), 10));
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [--fen <FEN>] [--depth <N>]\n";
            return 0;
        }
    }

    chess::Board board(fen);
    GeneralEvaluator evaluator;
    auto search = std::make_unique<chess::Search>(&evaluator, 16); // 16MB TT

    // Move history for undo support.
    std::vector<chess::Move> move_history;
    std::vector<std::string> fen_history;
    fen_history.push_back(board.getFen());

    std::cout << "Bienvenido al juego de ajedrez!\n";
    std::cout << "Comandos: <move> (UCI), quit, undo, new, fen, d (display), go depth <N>\n";
    printBoard(board);

    while (board.isGameOver().second == chess::GameResult::NONE) {
        if (board.sideToMove() == chess::Color::BLACK) {
            // ── User's turn ──
            std::string raw_input;

            // [FIX] Handle EOF gracefully instead of infinite-loop.
            if (!(std::cout << "Tu movimiento: " << std::flush)) break;
            if (!std::getline(std::cin, raw_input)) break; // EOF → exit loop

            std::string input = toLower(raw_input);

            // Trim leading/trailing whitespace.
            size_t start = input.find_first_not_of(" \t");
            if (start == std::string::npos) continue; // Empty/whitespace input → re-prompt.
            size_t end = input.find_last_not_of(" \t");
            input = input.substr(start, end - start + 1);

            // ── Commands ──
            if (input == "quit" || input == "q" || input == "exit") {
                std::cout << "¡Hasta luego!\n";
                return 0;
            }
            if (input == "d" || input == "board" || input == "display") {
                printBoard(board);
                continue;
            }
            if (input == "fen") {
                std::cout << board.getFen() << "\n";
                continue;
            }
            if (input == "new" || input == "restart") {
                board = chess::Board(chess::constants::STARTPOS);
                move_history.clear();
                fen_history.clear();
                fen_history.push_back(board.getFen());
                search = std::make_unique<chess::Search>(&evaluator, 16);
                std::cout << "Nueva partida.\n";
                printBoard(board);
                continue;
            }
            if (input == "undo") {
                if (move_history.size() >= 2) {
                    // Undo both user's and engine's last moves.
                    for (int i = 0; i < 2 && !move_history.empty(); ++i) {
                        chess::Move last = move_history.back();
                        board.unmakeMove(last);
                        move_history.pop_back();
                        fen_history.pop_back();
                    }
                    std::cout << "Deshecho.\n";
                    printBoard(board);
                } else {
                    std::cout << "No hay movimientos para deshacer.\n";
                }
                continue;
            }
            if (input.rfind("go depth ", 0) == 0) {
                // go depth N → engine plays with specified depth.
                // Parse depth after "go depth ".
                int depth = 3;
                try {
                    depth = std::stoi(input.substr(9));
                } catch (...) {
                    std::cout << "Profundidad inválida. Uso: go depth <N>\n";
                    continue;
                }
                depth = std::max(1, std::min(depth, 10));

                chess::SearchLimits limits;
                limits.maxDepth = depth;
                chess::SearchResult result = search->search(board, limits);

                if (result.move == chess::Move()) {
                    std::cout << "El motor no encuentra movimiento.\n";
                    break;
                }
                board.makeMove(result.move);
                move_history.push_back(result.move);
                fen_history.push_back(board.getFen());
                std::cout << "Motor (prof " << depth << "): "
                          << chess::uci::moveToUci(result.move)
                          << " (eval: " << result.score << ")\n";
                printBoard(board);
                continue;
            }

            // ── Parse as UCI move ──
            if (input.size() < 4 || input.size() > 5) {
                std::cout << "Formato inválido. Usa notación UCI (ej: e2e4).\n";
                continue;
            }

            chess::Move move;
            try {
                move = chess::uci::uciToMove(board, input);
            } catch (...) {
                std::cout << "Movimiento inválido. Intenta de nuevo.\n";
                continue;
            }

            if (move == chess::Move()) {
                std::cout << "Movimiento inválido. Intenta de nuevo.\n";
                continue;
            }
            if (!isThisMoveLegal(board, move)) {
                std::cout << "Movimiento ilegal. Intenta de nuevo.\n";
                continue;
            }

            board.makeMove(move);
            move_history.push_back(move);
            fen_history.push_back(board.getFen());
            printBoard(board);

        } else {
            // ── Engine's turn ──
            std::cout << "Calculando...\n";

            chess::SearchLimits limits;
            limits.maxDepth = engine_depth;
            chess::SearchResult result = search->search(board, limits);

            // [FIX] Verify the move is valid before playing.
            if (result.move == chess::Move()) {
                std::cout << "El motor no encuentra movimiento.\n";
                break;
            }

            board.makeMove(result.move);
            move_history.push_back(result.move);
            fen_history.push_back(board.getFen());

            std::cout << "Motor: " << chess::uci::moveToUci(result.move)
                      << " (eval: " << result.score << ")\n";
            printBoard(board);
        }
    }

    // ── Game over ──
    auto [reason, result] = board.isGameOver();
    switch (result) {
        case chess::GameResult::WIN:
            std::cout << "¡Jaque mate! Ganador: "
                      << (board.sideToMove() == chess::Color::BLACK ? "Blancas" : "Negras")
                      << ".\n";
            break;
        case chess::GameResult::DRAW:
            std::cout << "Tablas: ";
            switch (reason) {
                case chess::GameResultReason::STALEMATE: std::cout << "Ahogado."; break;
                case chess::GameResultReason::FIFTY_MOVE_RULE: std::cout << "Regla de 50 movimientos."; break;
                case chess::GameResultReason::THREEFOLD_REPETITION: std::cout << "Triple repetición."; break;
                case chess::GameResultReason::INSUFFICIENT_MATERIAL: std::cout << "Material insuficiente."; break;
                default: std::cout << "Resultado indefinido."; break;
            }
            std::cout << "\n";
            break;
        default:
            std::cout << "El juego ha terminado.\n";
            break;
    }

    return 0;
}
