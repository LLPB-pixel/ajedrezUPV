#include "chess.hpp"
#include "classGeneralEvaluator.hpp"

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

using namespace chess;


class Bot {
private:
    int MAX_DEPTH;
    GeneralEvaluator* evaluator = nullptr;
    int currentDepth = 0;
    Board board;
    Movelist legalMoves;
    Move moveHistory[60]; 

public:
    Bot(Board board, GeneralEvaluator* evaluator, int MAX_DEPTH) : MAX_DEPTH(MAX_DEPTH) {
        this->evaluator = evaluator;
        this->board = board;
        movegen::legalmoves(legalMoves, board);
    }
    Bot(const Bot& other) : currentDepth(other.currentDepth), board(other.board), legalMoves(other.legalMoves) {
        for (int i = 0; i < MAX_DEPTH; ++i) {
            moveHistory[i] = other.moveHistory[i];
        }
    }

    ~Bot() {
        legalMoves.clear(); 
    }

    void makemove(Move move) {
        if (this->currentDepth >= MAX_DEPTH) { 
            throw std::out_of_range("Depth exceeds maximum allowed depth");
        }

        board.makeMove(move);         
        moveHistory[currentDepth] = move; 
        currentDepth++;
    }

    void undoMove(Move move, int depth) {
        if (depth >= MAX_DEPTH) { 
            throw std::out_of_range("Depth exceeds maximum allowed depth");
        }

        board.unmakeMove(move);        
        moveHistory[depth] = Move(); 
        currentDepth--;
    }


    const Move* getMoveHistory() const {
        return moveHistory; 
    }



    Move findBestMove(int maxDepth) {
        // Genera los movimientos legales 
        movegen::legalmoves(legalMoves, board);
        //declarar semaforo para la variable compartida
        std::mutex mutex;
        float bestScore = -100000.0f;
        Move bestMove;
        std::vector<std::thread> threads;
        for (const Move& move : legalMoves) {
            //PELIGROSO: SE PUEDEN PROVOCAR SEGMENTATION FAULTS
            Bot threadBot(*this);
            threads.emplace_back([threadBot, move, maxDepth, &mutex, &bestScore, &bestMove]() mutable {
                threadBot.makemove(move);
                float score = -threadBot.megascout(maxDepth - 1, -100000.0f, 100000.0f);
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    if (score > bestScore) {
                        bestScore = score;
                        bestMove = move;
                    }
                }
            });
        }
    
        // Se espera a que finalicen todos los hilos.
        for (std::thread& t : threads) {
            t.join();
        }
    
        return bestMove;
    }
     
    float megascout(int depth, float alpha, float beta) {
        if (depth == 0 || board.isGameOver().second != chess::GameResult::NONE) {
            return evaluator->evaluate(&board, board.sideToMove());
        }
       
        
        movegen::legalmoves(this->legalMoves, board);
    
        if (this->legalMoves.size() == 0) {
            return evaluator->evaluate(&board, board.sideToMove());
        }
    
        float maxScore = -100000.0f;
        for (const Move& move : legalMoves) {
            makemove(move);
            float score = -megascout(depth - 1, -beta, -alpha);
            undoMove(move, currentDepth - 1);
    
            if (score > maxScore) {
                maxScore = score;
            }
            if (maxScore > alpha) {
                alpha = maxScore;
            }
            if (alpha >= beta) {
                break; // poda beta
            }
        }
    
        return maxScore;
    }
    

};
