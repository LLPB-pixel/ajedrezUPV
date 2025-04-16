#include "NodeMove.h"



int main(){
    Board board(chess::constants::STARTPOS);
    NodeMove *root = new NodeMove(board, nullptr);
    root->printBoard();
    delete root;

    return 0;
}
