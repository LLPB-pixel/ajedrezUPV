#include "chess/transposition_table.h"
#include <algorithm>
#include <cmath>

namespace chess {

TranspositionTable::TranspositionTable(size_t megabytes) {
    resize(megabytes);
}

void TranspositionTable::resize(size_t megabytes) {
    constexpr size_t ENTRY_SIZE = sizeof(PackedEntry);
    size_t total_entries = (megabytes * 1024ULL * 1024ULL) / ENTRY_SIZE;
    numBuckets_ = std::max<size_t>(1, total_entries / BUCKET_SIZE);
    // Round down to power of two for fast bitmask indexing.
    numBuckets_ = 1ULL << (63 - __builtin_clzll(numBuckets_));
    mask_ = numBuckets_ - 1;
    buckets_.assign(numBuckets_, Bucket{});
    clear();
}

bool TranspositionTable::probe(uint64_t key, TTEntry& entry) const {
    const uint32_t k32 = high32(key);
    const size_t idx = index(key);
    const auto& bucket = buckets_[idx];

    for (int i = 0; i < BUCKET_SIZE; ++i) {
        const PackedEntry& e = bucket.e[i];
        if (e.key32 == k32) {
            entry.key   = key;  // restore full key for API compatibility
            entry.move  = e.move;
            entry.score = static_cast<float>(e.score); // adjusted value; caller does scoreFromTT(entry.score, ply)
            entry.depth = e.depth;
            entry.flag  = static_cast<TTFlag>(e.flag);
            entry.age   = e.age;
            ++hits_;
            return true;
        }
    }
    ++misses_;
    return false;
}

void TranspositionTable::store(uint64_t key, Move move, float score,
                               int depth, TTFlag flag, int ply) {
    const uint32_t k32 = high32(key);
    const size_t idx = index(key);
    auto& bucket = buckets_[idx];

    const float packedScore = scoreForTT(score, ply);
    const int16_t packedDepth = static_cast<int16_t>(std::clamp(depth, -32768, 32767));

    // Find the best slot: prefer exact key match, then empty slot, then
    // stale (old age), then shallowest of the current generation.
    int target = -1;
    int shallowestDepth = INT32_MAX;
    int shallowestIdx = 0;
    for (int i = 0; i < BUCKET_SIZE; ++i) {
        if (bucket.e[i].key32 == k32) {
            target = i;  // Same position — always overwrite.
            break;
        }
        if (bucket.e[i].key32 == 0) {
            target = i;  // Empty slot — use it.
            break;
        }
        if (bucket.e[i].age != currentAge_) {
            target = i;  // Stale entry from a previous search — prefer it.
            break;
        }
        if (bucket.e[i].depth < shallowestDepth) {
            shallowestDepth = bucket.e[i].depth;
            shallowestIdx = i;
        }
    }
    if (target == -1) target = shallowestIdx;

    PackedEntry& e = bucket.e[target];

    // Replace if same key, empty, stale, or at least as deep.
    if (e.key32 == k32 || e.key32 == 0 || e.age != currentAge_ ||
        packedDepth >= e.depth) {
        e.key32 = k32;
        e.move  = move;
        e.score = static_cast<float>(packedScore);
        e.depth = packedDepth;
        e.flag  = static_cast<uint8_t>(flag);
        e.age   = currentAge_;
        ++stores_;
    }
}

float TranspositionTable::scoreForTT(float score, int ply) {
    // Mate scores are ply-relative: adjust so they can be stored globally.
    if (score > eval::MATE_SCORE - 1000) return score + static_cast<float>(ply);
    if (score < -eval::MATE_SCORE + 1000) return score - static_cast<float>(ply);
    return score;
}

float TranspositionTable::scoreFromTT(float score, int ply) {
    // Reverse the ply-relative adjustment.
    if (score > eval::MATE_SCORE - 1000) return score - static_cast<float>(ply);
    if (score < -eval::MATE_SCORE + 1000) return score + static_cast<float>(ply);
    return score;
}

void TranspositionTable::clear() {
    for (auto& bucket : buckets_) {
        for (auto& entry : bucket.e) {
            entry = PackedEntry{};
        }
    }
    hits_ = 0;
    misses_ = 0;
    stores_ = 0;
}

} // namespace chess
