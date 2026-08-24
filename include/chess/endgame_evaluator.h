#ifndef ENDGAMEEVALUATOR_H
#define ENDGAMEEVALUATOR_H

#include "chess/general_evaluator.h"

// [PERF] EndgameEvaluator is now a thin phase selector.  All heatmap tables
// and the shared evaluation flow live in GeneralEvaluator::evaluateCommon,
// which computes the attack maps once per call instead of ten times.
class EndgameEvaluator : public GeneralEvaluator {
public:
    EndgameEvaluator();
    float evaluate(const Board *board, const Color color) override;
    ~EndgameEvaluator();
};

#endif
