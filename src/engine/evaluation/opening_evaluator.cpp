#include "chess/opening_evaluator.h"

OpeningEvaluator::OpeningEvaluator() {}
OpeningEvaluator::~OpeningEvaluator() {}

float OpeningEvaluator::evaluate(const Board *board, const Color color) {
    return evaluateCommon(board, color, /*endgame=*/false);
}
