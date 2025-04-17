#ifndef GENERALEVALUATOR_H
#define GENERALEVALUATOR_H

#include "Evaluator.h"

class GeneralEvaluator : public Evaluator {
public:
  GeneralEvaluator();

  float evaluate(const Board *board, const Color color) override;
  float position(const Board *board, const Color color) override;
  float material(const Board *board, const Color color) override;
  float pawn_structure(const Board *board, const Color color) override;
  float safe_king(const Board *board, const Color color) override;
  float mobility(const Board *board, const Color color) override;

  ~GeneralEvaluator() override;
};

#endif
