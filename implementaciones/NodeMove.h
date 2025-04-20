#ifndef NODEMOVE_H
#define NODEMOVE_H

#include "chess.hpp"
#include "GeneralEvaluator.h"
using namespace chess;

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

#define MAX_DEPTH 3//tocar esto para cambiar la profundidad maxima de busqueda
#define MAX_BRANCH 100


class NodeMove{
    private:
        int currentDepth;
        Board board;
        NodeMove *parent = nullptr;
        NodeMove *childs[MAX_BRANCH] = {nullptr};
        Move lastMove;
        float eval = 0.0f;
        int childIndex = 0;


    public:
        //Constructor genral
        NodeMove(Board board, NodeMove *parent);
        // Destructor
        ~NodeMove();
        // Añadir hijo
        void addChild(NodeMove *child);
        void printBoard();
        int evaluateBoard();
        float minimax(GeneralEvaluator *evaluator, Color rootColor);
        chess::Move getBestMove(float bestScore);
        void printEvaluationsOfChildren() const;
        void printBoardsAndEvaluationsOfChildren() const;

        //getters varios
        float getEval() const;
        int getChildIndex() const;
        NodeMove* getChild(int index) const;
        chess::Move getLastMove() const;
};

#endif // NODEMOVE_H