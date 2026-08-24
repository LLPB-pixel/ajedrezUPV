#include "chess/general_evaluator.h"
#include "chess/opening_evaluator.h"
#include "chess/endgame_evaluator.h"
#include <algorithm>
#include <iostream>

using namespace chess;

namespace {

// [PERF] Precomputed bitboard masks for pawn_structure() — replaces the old
// per-pawn rank/file loops that called board->at() for every square ahead.
struct PawnMasks {
    Bitboard adjacentFiles[8];  // files f-1 and f+1 (bounds-safe)
    Bitboard passed[2][64];     // [color][sq]: squares strictly ahead in 3 files

    PawnMasks() {
        for (int f = 0; f < 8; ++f) {
            Bitboard adj = 0ULL;
            if (f > 0) adj |= Bitboard(File(f - 1));
            if (f < 7) adj |= Bitboard(File(f + 1));
            adjacentFiles[f] = adj;
        }
        for (int sq = 0; sq < 64; ++sq) {
            Square s(sq);
            int file = s.file();
            int rank = s.rank();
            Bitboard files = Bitboard(File(file)) | adjacentFiles[file];

            // White pawns: all ranks strictly above; black: strictly below.
            Bitboard whitePassed = 0ULL;
            Bitboard blackPassed = 0ULL;
            for (int r = 0; r < 8; ++r) {
                if (r > rank) whitePassed |= files & Bitboard(Rank(r));
                if (r < rank) blackPassed |= files & Bitboard(Rank(r));
            }
            passed[0][sq] = whitePassed;
            passed[1][sq] = blackPassed;
        }
    }
};

const PawnMasks& pawnMasks() {
    static const PawnMasks masks;  // thread-safe one-time init
    return masks;
}

// Weighted attacker count: pawns weigh 100, other pieces 1 (original semantics).
// Each attack pattern is masked by its own piece-type bitboard BEFORE unioning,
// exactly as the original per-square loops did.
inline int weightedAttackers(const Board* board, Color by,
                             Bitboard pawnAtt, Bitboard knightAtt,
                             Bitboard bishopAtt, Bitboard rookAtt,
                             Bitboard kingAtt) {
    const Bitboard pawnHits  = pawnAtt & board->pieces(PieceType::PAWN, by);
    const Bitboard otherHits = (knightAtt & board->pieces(PieceType::KNIGHT, by))
                             | (bishopAtt & (board->pieces(PieceType::BISHOP, by)
                                           | board->pieces(PieceType::QUEEN, by)))
                             | (rookAtt   & (board->pieces(PieceType::ROOK, by)
                                           | board->pieces(PieceType::QUEEN, by)))
                             | (kingAtt   & board->pieces(PieceType::KING, by));
    return 100 * pawnHits.count() + otherHits.count();
}

} // namespace

GeneralEvaluator::GeneralEvaluator() {}

// =========================================================================
// [PERF] Single-pass evaluation.  The attack maps are computed exactly once
// per call (2 getSeenSquares calls instead of the previous 10) and reused
// across both colors' king-safety and control terms.
// =========================================================================
float GeneralEvaluator::evaluate(const Board *board, const Color color) {
    // Delegate to the phase-specific evaluation (opening vs endgame).
    bool hasQueens = !board->pieces(chess::PieceType::QUEEN, chess::Color::WHITE).empty() ||
                     !board->pieces(chess::PieceType::QUEEN, chess::Color::BLACK).empty();
    return evaluateCommon(board, color, /*endgame=*/!hasQueens);
}

float GeneralEvaluator::evaluateCommon(const Board *board, const Color color,
                                       bool endgame) {
    const auto [reason, result] = board->isGameOver();
    if (result == chess::GameResult::LOSE) {
        return (board->sideToMove() == color) ? -eval::MATE_SCORE
                                              :  eval::MATE_SCORE;
    }
    if (result == chess::GameResult::DRAW) {
        return 0.0f;
    }

    // [PERF] One shared AttackMaps computation for every term below.
    AttackMaps maps;
    computeAttackMaps(board, maps);

    const float materialScore   = endgame ? material_endgame(board, color)
                                          : material_opening(board, color);
    const float kingSafetyScore = safe_king_maps(board, color, maps)
                                - safe_king_maps(board, ~color, maps);
    const float controlScore    = control_maps(color, maps)
                                - control_maps(~color, maps);

    float score = 0.0f;
    score += 1.0f  * materialScore;
    score += 0.01f * kingSafetyScore;
    score += 0.01f * controlScore;

    return score;
}

float GeneralEvaluator::positionOfThePiecesAndMaterial(const Board *board) {
    float whiteMaterial = 0;
    float blackMaterial = 0;

    whiteMaterial += evaluatePieceType(board, PieceType::PAWN,   Color::WHITE, 1.0,  importance);
    whiteMaterial += evaluatePieceType(board, PieceType::KNIGHT, Color::WHITE, 3.0,  importance);
    whiteMaterial += evaluatePieceType(board, PieceType::BISHOP, Color::WHITE, 3.15, importance);
    whiteMaterial += evaluatePieceType(board, PieceType::ROOK,   Color::WHITE, 5.0,  importance);
    whiteMaterial += evaluatePieceType(board, PieceType::QUEEN,  Color::WHITE, 9.0,  importance);

    blackMaterial += evaluatePieceType(board, PieceType::PAWN,   Color::BLACK, 1.0,  importance);
    blackMaterial += evaluatePieceType(board, PieceType::KNIGHT, Color::BLACK, 3.0,  importance);
    blackMaterial += evaluatePieceType(board, PieceType::BISHOP, Color::BLACK, 3.15, importance);
    blackMaterial += evaluatePieceType(board, PieceType::ROOK,   Color::BLACK, 5.0,  importance);
    blackMaterial += evaluatePieceType(board, PieceType::QUEEN,  Color::BLACK, 9.0,  importance);

    return whiteMaterial - blackMaterial;
}

float GeneralEvaluator::material_opening(const Board *board, Color color) const {
    float whiteMaterial = 0;
    float blackMaterial = 0;

    whiteMaterial += evaluatePieceType(board, PieceType::PAWN,   Color::WHITE, 1.0,  white_pawn_heatmap);
    whiteMaterial += evaluatePieceType(board, PieceType::KNIGHT, Color::WHITE, 3.0,  minor_pieces_heatmap);
    whiteMaterial += evaluatePieceType(board, PieceType::BISHOP, Color::WHITE, 3.15, minor_pieces_heatmap);
    whiteMaterial += evaluatePieceType(board, PieceType::ROOK,   Color::WHITE, 5.0,  importance);
    whiteMaterial += evaluatePieceType(board, PieceType::QUEEN,  Color::WHITE, 9.0,  importance);

    blackMaterial += evaluatePieceType(board, PieceType::PAWN,   Color::BLACK, 1.0,  black_pawn_heatmap);
    blackMaterial += evaluatePieceType(board, PieceType::KNIGHT, Color::BLACK, 3.0,  minor_pieces_heatmap);
    blackMaterial += evaluatePieceType(board, PieceType::BISHOP, Color::BLACK, 3.15, minor_pieces_heatmap);
    blackMaterial += evaluatePieceType(board, PieceType::ROOK,   Color::BLACK, 5.0,  importance);
    blackMaterial += evaluatePieceType(board, PieceType::QUEEN,  Color::BLACK, 9.0,  importance);

    float score = whiteMaterial - blackMaterial;
    return (color == Color::WHITE) ? score : -score;
}

float GeneralEvaluator::material_endgame(const Board *board, Color color) const {
    float whiteMaterial = 0;
    float blackMaterial = 0;

    whiteMaterial += evaluatePieceType(board, PieceType::PAWN,   Color::WHITE, 1.0,  pawn_endgame_heatmap);
    whiteMaterial += evaluatePieceType(board, PieceType::KNIGHT, Color::WHITE, 3.0,  importance);
    whiteMaterial += evaluatePieceType(board, PieceType::BISHOP, Color::WHITE, 3.15, importance);
    whiteMaterial += evaluatePieceType(board, PieceType::ROOK,   Color::WHITE, 5.0,  importance);
    whiteMaterial += evaluatePieceType(board, PieceType::QUEEN,  Color::WHITE, 9.0,  importance);
    whiteMaterial += evaluatePieceType(board, PieceType::KING,   Color::WHITE, 0.0,  king_endgame_heatmap);

    blackMaterial += evaluatePieceType(board, PieceType::PAWN,   Color::BLACK, 1.0,  pawn_endgame_heatmap);
    blackMaterial += evaluatePieceType(board, PieceType::KNIGHT, Color::BLACK, 3.0,  importance);
    blackMaterial += evaluatePieceType(board, PieceType::BISHOP, Color::BLACK, 3.15, importance);
    blackMaterial += evaluatePieceType(board, PieceType::ROOK,   Color::BLACK, 5.0,  importance);
    blackMaterial += evaluatePieceType(board, PieceType::QUEEN,  Color::BLACK, 9.0,  importance);
    blackMaterial += evaluatePieceType(board, PieceType::KING,   Color::BLACK, 0.0,  king_endgame_heatmap);

    float score = whiteMaterial - blackMaterial;
    return (color == Color::WHITE) ? score : -score;
}

float GeneralEvaluator::evaluatePieceType(
    const Board* board,
    chess::PieceType type,
    chess::Color color,
    float baseValue,
    const float heatmaps[8][8]
) const {
    float material = 0.0;
    chess::Bitboard pieces = board->pieces(type, color);
    while (pieces) {
        chess::Square sq = pieces.pop();
        int rank = sq.rank();
        int file = sq.file();
        if (type == chess::PieceType::PAWN) {
            material += 0.95f + 0.05f * (color == chess::Color::WHITE ? rank : (7 - rank));
            material += heatmaps[rank][file] * 1.2f;
        } else {
            material += baseValue + heatmaps[rank][file];
        }
    }
    return material;
}

// =========================================================================
// [E7] King safety, operating on precomputed attack maps.
// =========================================================================
float GeneralEvaluator::safe_king_maps(const Board *board, const Color color,
                                       const AttackMaps& maps) const {
    const Color enemy = ~color;
    const int enemyIdx = (enemy == Color::WHITE) ? 0 : 1;

    chess::Bitboard nextToKing = chess::attacks::king(board->kingSq(color));

    // Fraction of neighboring squares under enemy control (0 = safe, 1 = unsafe).
    const chess::Bitboard controlledAroundKing = nextToKing & maps.controlled[enemyIdx];
    const float controlFraction =
        static_cast<float>(controlledAroundKing.count())
        / static_cast<float>(std::max<int>(nextToKing.count(), 1));

    const chess::Bitboard seenAroundKing = nextToKing & maps.seen[enemyIdx];
    const float seenFraction =
        static_cast<float>(seenAroundKing.count())
        / static_cast<float>(std::max<int>(nextToKing.count(), 1));

    // Check if the king square itself is attacked.
    const chess::Bitboard kingBB = chess::Bitboard::fromSquare(board->kingSq(color));
    const bool kingAttacked = (maps.seen[enemyIdx] & kingBB).getBits() != 0;

    return -controlFraction * 5.0f
           - seenFraction * 3.0f
           - (kingAttacked ? 2.0f : 0.0f);
}

// Public virtual API: compute maps on demand (kept for standalone use).
float GeneralEvaluator::safe_king(const chess::Board *board, chess::Color color) {
    AttackMaps maps;
    computeAttackMaps(board, maps);
    return safe_king_maps(board, color, maps);
}

// =========================================================================
// [E2] Control score from precomputed maps.
// =========================================================================
float GeneralEvaluator::control_maps(const Color color,
                                     const AttackMaps& maps) const {
    const int idx = (color == Color::WHITE) ? 0 : 1;
    chess::Bitboard controlledSquares = maps.controlled[idx];
    float score = 0.0f;

    while (controlledSquares) {
        chess::Square sq = controlledSquares.pop();
        score += importance[sq.rank()][sq.file()];
    }

    return score;
}

float GeneralEvaluator::control(const Board *board, const Color color) {
    AttackMaps maps;
    computeAttackMaps(board, maps);
    return control_maps(color, maps);
}

// =========================================================================
// [E3/E4/E5] Pawn structure — now fully bitboard-based with precomputed
// masks (one AND + popcount per check instead of nested square loops).
// Semantics identical to the original implementation.
// =========================================================================
float GeneralEvaluator::pawn_structure(const chess::Board *board, chess::Color color) {
    const PawnMasks& masks = pawnMasks();
    const int colorIdx = (color == Color::WHITE) ? 0 : 1;
    const Bitboard pawns = board->pieces(PieceType::PAWN, color);
    const Bitboard theirPawns = board->pieces(PieceType::PAWN, ~color);
    float score = 0.0f;

    Bitboard copy = pawns;
    while (copy) {
        const Square pawnSquare = copy.pop();
        const int file = pawnSquare.file();

        // Isolated pawn: no own pawn on either adjacent file.
        if ((pawns & masks.adjacentFiles[file]).empty()) {
            score -= 0.5f;
        }

        // Doubled pawn: more than one own pawn on this file.
        if ((pawns & Bitboard(File(file))).count() > 1) {
            score -= 0.5f;
        }

        // Passed pawn: no enemy pawn strictly ahead in the 3 relevant files.
        if ((theirPawns & masks.passed[colorIdx][pawnSquare.index()]).empty()) {
            score += 1.0f;
        }
    }

    return score;
}

GeneralEvaluator::~GeneralEvaluator() {}

chess::Bitboard GeneralEvaluator::getSeenSquares(const chess::Board *board, chess::Color color) const {
    chess::Bitboard attackedSquares = 0ULL;
    chess::Bitboard pieces = board->us(color);

    while (pieces) {
        chess::Square sq = pieces.pop();
        chess::Piece piece = board->at(sq);

        switch (piece.type().internal()) {
            case chess::PieceType::PAWN:
                attackedSquares |= chess::attacks::pawn(color, sq);
                break;
            case chess::PieceType::KNIGHT:
                attackedSquares |= chess::attacks::knight(sq);
                break;
            case chess::PieceType::BISHOP:
                attackedSquares |= chess::attacks::bishop(sq, board->occ());
                break;
            case chess::PieceType::ROOK:
                attackedSquares |= chess::attacks::rook(sq, board->occ());
                break;
            case chess::PieceType::QUEEN:
                attackedSquares |= chess::attacks::queen(sq, board->occ());
                break;
            case chess::PieceType::KING:
                attackedSquares |= chess::attacks::king(sq);
                break;
            default:
                break;
        }
    }

    return attackedSquares;
}

void GeneralEvaluator::computeAttackMaps(const Board *board, AttackMaps& maps) const {
    maps.seen[0] = getSeenSquares(board, Color::WHITE);
    maps.seen[1] = getSeenSquares(board, Color::BLACK);
    maps.controlled[0] = getControlledSquaresFromMaps(board, Color::WHITE, maps);
    maps.controlled[1] = getControlledSquaresFromMaps(board, Color::BLACK, maps);
}

// Contest filtering on precomputed seen-maps with popcount attacker counts
// (identical semantics to the original per-square attacker loops).
chess::Bitboard GeneralEvaluator::getControlledSquaresFromMaps(
    const Board *board, Color color, const AttackMaps& maps) const {
    const int idx = (color == Color::WHITE) ? 0 : 1;
    const int enemyIdx = 1 - idx;
    chess::Bitboard controlled = maps.seen[idx];
    chess::Bitboard disputed = maps.seen[idx] & maps.seen[enemyIdx];

    while (disputed) {
        const Square sq = disputed.pop();

        const Bitboard pawnAttOwn   = attacks::pawn(~color, sq);
        const Bitboard pawnAttOpp   = attacks::pawn(color, sq);
        const Bitboard knightAtt    = attacks::knight(sq);
        const Bitboard bishopAtt    = attacks::bishop(sq, board->occ());
        const Bitboard rookAtt      = attacks::rook(sq, board->occ());
        const Bitboard kingAtt      = attacks::king(sq);

        const int ownAttackers = weightedAttackers(board, color,
                                                   pawnAttOwn, knightAtt,
                                                   bishopAtt, rookAtt, kingAtt);
        const int opponentAttackers = weightedAttackers(board, ~color,
                                                        pawnAttOpp, knightAtt,
                                                        bishopAtt, rookAtt, kingAtt);

        if ((ownAttackers <= opponentAttackers && ownAttackers > 0) ||
            opponentAttackers >= 100) {
            controlled.clear(sq.index());
        }
    }

    return controlled;
}

chess::Bitboard GeneralEvaluator::getControlledSquares(const chess::Board *board, chess::Color color) const {
    AttackMaps maps;
    maps.seen[0] = getSeenSquares(board, Color::WHITE);
    maps.seen[1] = getSeenSquares(board, Color::BLACK);
    return getControlledSquaresFromMaps(board, color, maps);
}
