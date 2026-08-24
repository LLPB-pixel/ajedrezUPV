#include "chess/endgame_evaluator.h"

EndgameEvaluator::EndgameEvaluator() {}

float EndgameEvaluator::evaluate(const Board *board, const Color color) {
    return evaluateCommon(board, color, /*endgame=*/true);
}

EndgameEvaluator::~EndgameEvaluator() {}
