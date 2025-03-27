//Bot
 
#include <chess.hpp> 
using namespace chess;
 
class Bot{
    private: 
        Board board;
        Movelist legalMoves;
    public: 
        Bot(Board board){
            this->board = board;
            movegen::legalmoves(legalMoves, board);
        }
        //~Bot(){
        //pediente
        float evaluacion(){
            
        }
}
