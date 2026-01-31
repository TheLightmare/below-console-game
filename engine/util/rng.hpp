#pragma once
#include <random>
#include <cstdint>

// =============================================================================
// Global Random Number Generator Utility
// =============================================================================
// Provides a centralized, properly-seeded random number generator.
// Use this instead of rand() for better distribution and reproducibility.
// =============================================================================

class GameRNG {
public:
    // Get the global RNG instance
    static GameRNG& instance() {
        static GameRNG rng;
        return rng;
    }
    
    // Seed the RNG with a specific seed (for reproducibility)
    void seed(uint32_t seed_value) {
        rng_.seed(seed_value);
    }
    
    // Get the underlying generator (for use with std::uniform_*_distribution)
    std::mt19937& generator() { return rng_; }
    
    // === Convenience methods ===
    
    // Random integer in range [min, max] (inclusive)
    int random_int(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(rng_);
    }
    
    // Random float in range [0.0, 1.0)
    float random_float() {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(rng_);
    }
    
    // Random float in range [min, max)
    float random_float(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(rng_);
    }
    
    // Dice roll: roll <count> dice with <sides> sides each, return sum
    // e.g., roll_dice(2, 6) rolls 2d6
    int roll_dice(int count, int sides) {
        int total = 0;
        std::uniform_int_distribution<int> dist(1, sides);
        for (int i = 0; i < count; ++i) {
            total += dist(rng_);
        }
        return total;
    }
    
    // Probability check: returns true with probability p (0.0 to 1.0)
    bool chance(float probability) {
        return random_float() < probability;
    }
    
    // Random index for a container of given size
    size_t random_index(size_t size) {
        if (size == 0) return 0;
        std::uniform_int_distribution<size_t> dist(0, size - 1);
        return dist(rng_);
    }
    
private:
    GameRNG() {
        // Seed with random device on construction
        std::random_device rd;
        rng_.seed(rd());
    }
    
    std::mt19937 rng_;
};

// Convenience free functions for quick access
inline int random_int(int min, int max) {
    return GameRNG::instance().random_int(min, max);
}

inline float random_float() {
    return GameRNG::instance().random_float();
}

inline float random_float(float min, float max) {
    return GameRNG::instance().random_float(min, max);
}

inline int roll_dice(int count, int sides) {
    return GameRNG::instance().roll_dice(count, sides);
}

inline bool chance(float probability) {
    return GameRNG::instance().chance(probability);
}

inline size_t random_index(size_t size) {
    return GameRNG::instance().random_index(size);
}
