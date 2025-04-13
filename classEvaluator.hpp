#pragma once
#include "chess.hpp"
using namespace chess;
class Evaluator {
public:
    virtual float evaluate(const Board *board, Color color) = 0;
    virtual ~Evaluator() = default;
};