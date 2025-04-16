#include "NodeMove.h"

NodeMove::NodeMove(Board board, NodeMove *parent) : board(board), parent(parent) {
    if(parent == nullptr){
        currentDepth = 0;
    }
    else{
        currentDepth = parent->currentDepth + 1;
    }
    if(currentDepth < MAX_DEPTH) {
        Movelist legalMoves;
        movegen::legalmoves(legalMoves, board);
        for (const Move& move : legalMoves) {
            NodeMove *child = new NodeMove(board, this);
            child->board.makeMove(move); // Apply the move to the child node
            addChild(child);
        }
    }


}

NodeMove::~NodeMove() {
    for (int i = 0; i < MAX_BRANCH; ++i) {
        delete childs[i];
    }
}

void NodeMove::addChild(NodeMove *child) {
    if (childIndex >= MAX_BRANCH) {
        std::cerr << "Error: Maximum number of child nodes reached." << std::endl;
        return;
    }
    this->childs[childIndex] = child;
    childIndex++;
}
void NodeMove::printBoard() {
    std::cout << "  +-----------------+" << std::endl;
    for (int rank = 7; rank >= 0; --rank) { 
        std::cout << rank + 1 << " | "; 
        for (int row = 0; row < 8; ++row) { 
            chess::Square square(static_cast<chess::File::underlying>(row), static_cast<chess::Rank::underlying>(rank));
            chess::Piece piece = board.at(square);
            std::cout << static_cast<std::string>(piece) << " "; 
        }
        std::cout << "|" << std::endl;
    }
    std::cout << "  +-----------------+" << std::endl;
    std::cout << "    a b c d e f g h" << std::endl;
}



