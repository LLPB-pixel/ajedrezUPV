#ifndef NODEMOVE_H
#define NODEMOVE_H

#include "chess.hpp"
using namespace chess;

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

#define MAX_DEPTH 10 //tocar esto para cambiar la profundidad maxima de busqueda
#define MAX_BRANCH 100


class NodeMove{
    private:
        int currentDepth;
        Board board;
        NodeMove *parent = nullptr;
        NodeMove *childs[MAX_BRANCH] = {nullptr};
        int childIndex = 0;

    public:
        //Constructor genral
        NodeMove(Board board, NodeMove *parent);
        // Destructor
        ~NodeMove();
        // Añadir hijo
        void addChild(NodeMove *child);
        void printBoard();
};

#endif // NODEMOVE_H