#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include "chess.hpp"
#include "chess/evaluator.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

namespace chess {

// [B4] Transposition table entry flags.
enum class TTFlag : uint8_t {
    EXACT = 0,
    LOWER = 1,  // Beta cutoff (fail-high).
    UPPER = 2,  // Failed low.
};

struct TTEntry {
    uint64_t key       = 0;
    Move     move      = Move();
    float    score     = 0.0f;
    int16_t  depth     = 0;
    TTFlag   flag      = TTFlag::EXACT;
    uint8_t  age       = 0;  // Generation counter for replacement.
};

// Fixed-size transposition table with power-of-two buckets.
// Each bucket holds multiple entries for set-associative lookup.
// [PERF] Internal storage is packed to 16 B per entry (64 B buckets =
// exactly one cache line, ~1.5× capacity for the same MB).
// The public TTEntry API is unchanged — probe() unpacks to the full view.
class TranspositionTable {
public:
    static constexpr int BUCKET_SIZE = 4;

    explicit TranspositionTable(size_t megabytes = 16);
    ~TranspositionTable() = default;

    TranspositionTable(const TranspositionTable&) = delete;
    TranspositionTable& operator=(const TranspositionTable&) = delete;

    // Resize the table (in megabytes).
    void resize(size_t megabytes);

    // Probe the table. Returns true if a matching entry was found.
    bool probe(uint64_t key, TTEntry& entry) const;

    // Store an entry in the table.
    void store(uint64_t key, Move move, float score, int depth,
               TTFlag flag, int ply);

    // Adjust mate scores for storage/retrieval (ply-relative).
    static float scoreForTT(float score, int ply);
    static float scoreFromTT(float score, int ply);

    // Increment the age counter (call once per search).
    void newSearch() { ++currentAge_; }

    // Clear all entries.
    void clear();

    // Statistics.
    uint64_t hits() const { return hits_; }
    uint64_t misses() const { return misses_; }
    uint64_t stores() const { return stores_; }

private:
    // [PERF] Packed internal entry — 16 B (4 × 16 = 64 B per bucket =
    // exactly one cache line). Score kept as float for exact storage.
    struct PackedEntry {
        uint32_t key32 = 0;   // high 32 bits of the hash (0 = empty slot)
        Move     move  = Move();
        float    score = 0.0f;
        int16_t  depth = 0;
        uint8_t  flag  = 0;
        uint8_t  age   = 0;
    };
    static_assert(sizeof(PackedEntry) == 16, "PackedEntry must be 16 bytes");
    struct Bucket {
        PackedEntry e[BUCKET_SIZE];
    };

    static inline uint32_t high32(uint64_t key) {
        uint32_t h = static_cast<uint32_t>(key >> 32);
        return h == 0 ? 1u : h;  // 0 is reserved for "empty"
    }


    size_t numBuckets_ = 0;
    size_t mask_ = 0;  // numBuckets_ - 1  [PERF] bitmask instead of modulo
    std::vector<Bucket> buckets_;
    uint8_t currentAge_ = 0;

    mutable uint64_t hits_ = 0;    // [PERF] plain counters, not atomics
    mutable uint64_t misses_ = 0;  // single-threaded search: no contention
    mutable uint64_t stores_ = 0;

    size_t index(uint64_t key) const { return static_cast<size_t>(key & mask_); }
};

} // namespace chess

#endif // TRANSPOSITION_TABLE_H
