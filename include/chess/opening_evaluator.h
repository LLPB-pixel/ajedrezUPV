#ifndef OPENINGEVALUATOR_H
#define OPENINGEVALUATOR_H

#include "chess/general_evaluator.h"

// [PERF] OpeningEvaluator is now a thin phase selector.  All heatmap tables
// and the shared evaluation flow live in GeneralEvaluator::evaluateCommon,
// which computes the attack maps once per call instead of ten times.
class OpeningEvaluator : public GeneralEvaluator {
public:
    OpeningEvaluator();
    float evaluate(const Board *board, const Color color) override;
    ~OpeningEvaluator();
};

#endif
