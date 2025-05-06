#include "NodeMove.h"
#include <omp.h>
#include <iostream>
#include <algorithm>
#include <thread>
#include <mutex>

using namespace chess;
std::mutex cout_mutex;

NodeMove::NodeMove(Board *board, NodeMove* parent) : 
    parent_(parent),
    current_depth_(parent ? parent->current_depth_ + 1 : 0) {
    
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Creando nodo en profundidad: " << current_depth_ 
                    << " - Hilo: " << std::this_thread::get_id() << "\n";
    }

    if (current_depth_ == 1) {
        Movelist moves;
        movegen::legalmoves(moves, *board);

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Profundidad 1 - Movimientos posibles: " << moves.size() 
                      << " - MAX_BRANCH: " << MAX_BRANCH << "\n";
        }

        size_t move_count = moves.size();
        size_t num_threads = std::min((size_t)std::thread::hardware_concurrency(), move_count);
        if (num_threads == 0) num_threads = 1; 
        size_t moves_per_thread = move_count / num_threads;

        std::mutex child_mutex;
        std::array<std::thread, 64> threads;

        auto build_children = [&](size_t start, size_t end) {
            Board local_board = *board;
            for (size_t i = start; i < end && i < move_count; ++i) {
                Move move = moves[i];
                local_board.makeMove(move);

                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "Hilo " << std::this_thread::get_id() 
                              << " procesando movimiento: " << move 
                              << " en profundidad 1\n";
                }

                auto child = std::make_unique<NodeMove>(&local_board, this);
                local_board.unmakeMove(move);
                child->last_move_ = move;

                std::lock_guard<std::mutex> lock(child_mutex);
                if (child_count_ < MAX_BRANCH) {
                    children_[child_count_++] = std::move(child);
                    
                    std::lock_guard<std::mutex> cout_lock(cout_mutex);
                    std::cout << "Hijo añadido en profundidad 1. Total hijos: " 
                              << child_count_ << "\n";
                } else {
                    std::lock_guard<std::mutex> cout_lock(cout_mutex);
                    std::cout << "MAX_BRANCH alcanzado en profundidad 1. Movimiento omitido: " 
                              << move << "\n";
                }
            }
        };

        size_t start = 0;
        for (size_t t = 0; t < num_threads; ++t) {
            size_t end = std::min(start + moves_per_thread, move_count);
            threads[t] = std::thread(build_children, start, end);
            start = end;
        }

        for (size_t t = 0; t < num_threads; ++t) {
            threads[t].join();
        }
    }
    else if(current_depth_ < MAX_DEPTH) {
        child_count_ = 0;
        Movelist moves;
        movegen::legalmoves(moves, *board);

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Profundidad " << current_depth_ 
                      << " - Movimientos: " << moves.size() 
                      << " - MAX_BRANCH: " << MAX_BRANCH << "\n";
        }

        for (const Move& move : moves) {
            if (child_count_ >= MAX_BRANCH) {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "MAX_BRANCH alcanzado en profundidad " 
                          << current_depth_ << "\n";
                break;
            }
            
            
            board->makeMove(move);
            children_[child_count_] = std::make_unique<NodeMove>(board, this);
            board->unmakeMove(move);
            children_[child_count_]->last_move_ = move;

            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "Añadiendo hijo en profundidad " << current_depth_
                          << " - Movimiento: " << move 
                          << " - Hijos actuales: " << child_count_ + 1 << "\n";
            }

            child_count_++;
        }

    }
}

void NodeMove::printTree(int indent) const {
    std::string spacing(indent, ' ');
    
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << spacing << "Nodo profundidad " << current_depth_ 
              << " - Movimiento: " << last_move_ 
              << " - Hijos: " << child_count_ 
              << " - Eval: " << eval_ << "\n";
    
    for (size_t i = 0; i < child_count_; ++i) {
        children_[i]->printTree(indent + 4);
    }
}
void NodeMove::rebuildUntilDepth(Board* board) {

    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Rebuild en profundidad: " << current_depth_ 
                  << " - Hilo: " << std::this_thread::get_id() << "\n";
    }

    this->current_depth_ = this->parent_->current_depth_;
    if(this->current_depth_ < MAX_DEPTH -1){
        //no hace falta generar los hijos de este nodo
        for(int i = 0; i < this->child_count_; i++){
            this->children_[i]->rebuildUntilDepth(board);
        }
        return;
    }
    else if(this->current_depth_ == MAX_DEPTH -1){
        //hace falta generar los hijos de este nodo
        Movelist moves;
        movegen::legalmoves(moves, *board);

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Rebuild en MAX_DEPTH-1 - Movimientos: " << moves.size() 
                      << "\n";
        }

        size_t move_count = moves.size();
        size_t num_threads = std::min((size_t)std::thread::hardware_concurrency(), move_count);
        if (num_threads == 0) num_threads = 1;
        size_t moves_per_thread = move_count / num_threads;
        std::mutex child_mutex;
        std::array<std::thread, 64> threads;
        child_count_ = 0;
        auto build_children = [&](size_t start, size_t end) {
            Board local_board = *board;
            for (size_t i = start; i < end && i < move_count; ++i) {
                Move move = moves[i];
                local_board.makeMove(move);
                auto child = std::make_unique<NodeMove>(&local_board, this);
                local_board.unmakeMove(move);
                child->last_move_ = move;
    
                std::lock_guard<std::mutex> lock(child_mutex);
                if (child_count_ < MAX_BRANCH) {
                    children_[child_count_++] = std::move(child);
                }
            }
        };
    
        size_t start = 0;
        for (size_t t = 0; t < num_threads; ++t) {
            size_t end = std::min(start + moves_per_thread, move_count);
            threads[t] = std::thread(build_children, start, end);
            start = end;
        }
    
        for (size_t t = 0; t < num_threads; ++t) {
            threads[t].join();
        }
    }
}



void NodeMove::addChild(Board *board, Move move) {
    if (child_count_ < MAX_BRANCH) {
        children_[child_count_] = std::make_unique<NodeMove>(board, this);
        children_[child_count_]->last_move_ = move;
        child_count_++;
    }
}

void NodeMove::printBoard(Board &board) const {
    std::cout << "  +-----------------+\n";
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << rank + 1 << " | ";
        for (int file = 0; file < 8; ++file) {
            Square square(static_cast<File>(file), static_cast<Rank>(rank));
            std::cout << static_cast<std::string>(board.at(square)) << " ";
        }
        std::cout << "|\n";
    }
    std::cout << "  +-----------------+\n";
    std::cout << "    a b c d e f g h\n";
}



float NodeMove::minimax(GeneralEvaluator* evaluator, Board *board, Color root_color) {
    auto [reason, result] = board->isGameOver();
    if(result != GameResult::NONE) {
        eval_ = evaluator->evaluate(board, root_color);
        return eval_;
    }


    if (current_depth_ == MAX_DEPTH) {
        eval_ = evaluator->evaluate(board, root_color);
        return eval_;
    }

    if (child_count_ == 0) return 0.0f;

    if (board->sideToMove() == Color::WHITE) {
        float best_value = -99999.0f;
        for (size_t i = 0; i < child_count_; ++i) {
            board->makeMove(children_[i]->last_move_);
            float value = children_[i]->minimax(evaluator, board, root_color);
            board->unmakeMove(children_[i]->last_move_);
            best_value = std::max(best_value, value);
        }
        eval_ = best_value;
        return best_value;
    } else {
        float best_value = 99999.0f;
        for (size_t i = 0; i < child_count_; ++i) {
            float value = children_[i]->minimax(evaluator, board, root_color);
            best_value = std::min(best_value, value);
        }
        eval_ = best_value;
        return best_value;
    }
}
float NodeMove::alphaBeta(GeneralEvaluator* evaluator, float *alpha, float *beta, Color root_color, Board* board, std::mutex *alphaBetaMutex) {
    auto [reason, result] = board->isGameOver();
    if (result != GameResult::NONE) {
        eval_ = evaluator->evaluate(board, root_color);
        return eval_;
    }

    if (current_depth_ == MAX_DEPTH) {
        eval_ = evaluator->evaluate(board, root_color);
        return eval_;
    }

    if (current_depth_ == 1) {
        size_t numThreads = std::thread::hardware_concurrency();
        size_t movesPerThread;
        int aux = 0;
        
        if (child_count_ < numThreads) {
            movesPerThread = 1;
            numThreads = child_count_;
        } else if (child_count_ % numThreads == 0) {
            movesPerThread = child_count_ / numThreads;
        } else {
            movesPerThread = child_count_ / numThreads + 1;
            aux = child_count_ % numThreads;
        }
    
        std::array<std::thread, 64> threads; // Cambiado a un array de tamaño fijo
    
        float globalAlpha = *alpha;
        float globalBeta = *beta;
    
        std::mutex localMutex;
        auto threadTask = [&](size_t start, size_t end, Board threadBoard) {
            for (size_t i = start; i < end && i < child_count_; ++i) {
                threadBoard.makeMove(children_[i]->last_move_);
                float value = children_[i]->alphaBeta(evaluator, &globalAlpha, &globalBeta, root_color, &threadBoard, alphaBetaMutex);
                threadBoard.unmakeMove(children_[i]->last_move_);
    
                std::lock_guard<std::mutex> lock(localMutex);
                if (board->sideToMove() == Color::WHITE) {
                    globalAlpha = std::max(globalAlpha, value);
                    if (globalAlpha >= globalBeta) {
                        break;
                    }
                } else {
                    globalBeta = std::min(globalBeta, value);
                    if (globalBeta <= globalAlpha) {
                        break;
                    }
                }
            }
        };
    
        size_t lastEnd = 0;
        size_t start = 0;
        size_t end = 0;

        for (size_t t = 0; t < numThreads && lastEnd <= child_count_; ++t) {
            if(aux > 0){
                start = lastEnd;
                end = std::min(start + movesPerThread + 1, (size_t)child_count_);
                threads[t] = std::thread(threadTask, start, end, *board); 
                lastEnd = end;
                aux--;
            
            }
            else{
                start = lastEnd;
                end = std::min(start + movesPerThread, (size_t)child_count_);
                threads[t] = std::thread(threadTask, start, end, *board); 
                lastEnd = end;
            }
            
        }
    
        for (size_t t = 0; t < numThreads; ++t) { 
            threads[t].join();
        }
    
        eval_ = (board->sideToMove() == Color::WHITE) ? globalAlpha : globalBeta;
        return eval_;
    }

    if (board->sideToMove() == Color::WHITE) {
        float bestValue = -99999.0f;
        for (size_t i = 0; i < child_count_; ++i) {
            board->makeMove(children_[i]->last_move_);
            float value = children_[i]->alphaBeta(evaluator, alpha, beta, root_color, board, alphaBetaMutex);
            board->unmakeMove(children_[i]->last_move_);
            bestValue = std::max(bestValue, value);

            std::lock_guard<std::mutex> lock(*alphaBetaMutex);
            *alpha = std::max(*alpha, bestValue);
            if (*alpha >= *beta) {
                break;
            }
        }
        eval_ = bestValue;
        return bestValue;
    } else {
        float bestValue = 99999.0f;
        for (size_t i = 0; i < child_count_; ++i) {
            board->makeMove(children_[i]->last_move_);
            float value = children_[i]->alphaBeta(evaluator, alpha, beta, root_color, board, alphaBetaMutex);
            board->unmakeMove(children_[i]->last_move_);
            bestValue = std::min(bestValue, value);

            std::lock_guard<std::mutex> lock(*alphaBetaMutex);
            *beta = std::min(*beta, bestValue);
            if (*beta <= *alpha) {
                break;
            }
        }
        eval_ = bestValue;
        return bestValue;
    }
}

chess::Move NodeMove::getBestMove(float best_score) const {
    for (size_t i = 0; i < child_count_; ++i) {
        if (children_[i]->eval_ == best_score) {
            return children_[i]->last_move_;
        }
    }
    return Move(); // Return null move if not found
}

void NodeMove::printEvaluationsOfChildren() const {
    std::cout << "Child move evaluations:\n";
    for (size_t i = 0; i < child_count_; ++i) {
        std::cout << "Move: " << children_[i]->last_move_
                  << " | Eval: " << children_[i]->eval_ << "\n";
    }
}
NodeMove* NodeMove::getChildByMove(const Move& move) {
    for (size_t i = 0; i < this->child_count_; ++i) {
        if (this->children_[i]->last_move_ == move) {
            //copilot: cast to nodemove from unique_ptr
            return this->children_[i].get();
        }
    }
    return nullptr;
}
