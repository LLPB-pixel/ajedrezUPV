#include "chess.hpp"
using namespace chess;

constexpr int MAX_DEPTH = 60;

class Bot {
private:
    int currentDepth = 0;
    Board board;
    Movelist legalMoves;
    Move moveHistory[60]; 

public:
    Bot(Board board, int maxdepth){
        this->board = board;
        movegen::legalmoves(legalMoves, board);
    }
    Bot(const Bot& other) : maxdepth(other.maxdepth), currentDepth(other.currentDepth), board(other.board), legalMoves(other.legalMoves) {
        // Copiar el historial de movimientos
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

        board.undoMove(move);        
        moveHistory[depth] = Move(); 
        currentDepth--;
    }

    float evaluacion() {
        //Call a la funcion de evaluacion

        return 0.0f;
    }

    const Move* getMoveHistory() const {
        return moveHistory; 
    }
};
