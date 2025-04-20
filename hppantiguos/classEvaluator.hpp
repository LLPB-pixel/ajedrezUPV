#pragma once
#include "chess.hpp"
using namespace chess;
class Evaluator {
public:
    virtual float evaluate(const Board *board) = 0;
    virtual ~Evaluator() = default;
};