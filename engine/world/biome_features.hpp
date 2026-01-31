#pragma once
#include "biome.hpp"
#include "perlin_noise.hpp"
#include "../loaders/worldgen_loader.hpp"
#include "chunk.hpp"
#include <cmath>
#include <random>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// FEATURE TYPES
// ============================================================================

enum class FeatureType {
    NONE = 0,
    RIVER,
    LAKE,
    ROAD,
    BRIDGE,
    COUNT
};

struct FeatureSample {
    FeatureType type = FeatureType::NONE;
    float intensity = 0.0f;
    bool is_crossing = false;
    
    bool has_feature() const { return type != FeatureType::NONE && intensity > 0.0f; }
};

// ============================================================================
// FEATURE GENERATOR
// Uses elevation + moisture to create realistic rivers that flow downhill
// ============================================================================

class BiomeFeatureGenerator {
private:
    PerlinNoise source_noise;    // Where rivers originate
    PerlinNoise path_noise;      // River path shape
    PerlinNoise meander_noise;   // River bends
    PerlinNoise lake_noise;      // Lake placement
    PerlinNoise road_noise;      // Road network
    unsigned int seed;
    
    RiverConfig rivers;
    LakeConfig lakes;
    RoadConfig roads;
    ElevationThresholds elev;
    
public:
    BiomeFeatureGenerator(unsigned int seed = 0) 
        : source_noise(seed + 111),
          path_noise(seed + 222),
          meander_noise(seed + 333),
          lake_noise(seed + 444),
          road_noise(seed + 555),
          seed(seed) {}
    
    void set_seed(unsigned int new_seed) {
        seed = new_seed;
        source_noise = PerlinNoise(seed + 111);
        path_noise = PerlinNoise(seed + 222);
        meander_noise = PerlinNoise(seed + 333);
        lake_noise = PerlinNoise(seed + 444);
        road_noise = PerlinNoise(seed + 555);
    }
    
    void set_config(const WorldGenConfig& cfg) {
        rivers = cfg.rivers;
        lakes = cfg.lakes;
        roads = cfg.roads;
        elev = cfg.elev_thresh;
    }
    
    // === RIVER SAMPLING ===
    // Rivers spawn in highlands and flow toward the ocean
    // Width increases as elevation decreases
    FeatureSample sample_river(int wx, int wy, float elevation, float moisture) const {
        FeatureSample result;
        if (!rivers.enabled) return result;
        
        // Rivers only between source elevation and coast
        if (elevation < elev.coast || elevation > rivers.source_max_elev) {
            return result;
        }
        
        // Need minimum moisture for river sources
        if (elevation > rivers.source_min_elev && moisture < rivers.source_min_moisture) {
            return result;
        }
        
        // === STAGE 1: River source locations ===
        // Low-frequency noise determines WHERE rivers exist
        double source_val = source_noise.octave_noise(
            wx * rivers.source_density,
            wy * rivers.source_density,
            2, 0.5
        );
        source_val = std::abs(source_val);
        
        // Only continue near river "spawn lines"
        if (source_val > 0.07) return result;
        
        // === STAGE 2: River path shape ===
        // Meanders create organic bending
        double mx = meander_noise.noise(
            wx * rivers.meander_scale,
            wy * rivers.meander_scale
        ) * rivers.meander_strength;
        double my = meander_noise.noise(
            wx * rivers.meander_scale + 50,
            wy * rivers.meander_scale + 50
        ) * rivers.meander_strength;
        
        // Path noise with meander offset
        double path_val = path_noise.octave_noise(
            (wx + mx) * rivers.path_scale,
            (wy + my) * rivers.path_scale,
            2, 0.6
        );
        path_val = std::abs(path_val);
        
        // === STAGE 3: Width based on elevation ===
        // Rivers are narrow at source (high elevation) and wide near ocean (low elevation)
        float elev_range = rivers.source_max_elev - elev.coast;
        float elev_norm = (elevation - elev.coast) / elev_range;  // 0 at coast, 1 at source
        
        // Width grows as we go downstream (lower elevation)
        float width_factor = std::pow(1.0f - elev_norm, rivers.width_growth);
        float current_width = rivers.base_width + 
            (rivers.max_width - rivers.base_width) * width_factor;
        
        // Check if we're within the river
        if (path_val < current_width) {
            result.type = FeatureType::RIVER;
            result.intensity = 1.0f - (static_cast<float>(path_val) / current_width);
        }
        
        return result;
    }
    
    // === LAKE SAMPLING ===
    // Lakes form in low areas with high moisture
    FeatureSample sample_lake(int wx, int wy, float elevation, float moisture) const {
        FeatureSample result;
        if (!lakes.enabled) return result;
        
        // Lakes in specific elevation range
        if (elevation < lakes.min_elev || elevation > lakes.max_elev) {
            return result;
        }
        
        // Need moisture
        if (moisture < lakes.min_moisture) return result;
        
        double lake_val = lake_noise.octave_noise(
            wx * lakes.density,
            wy * lakes.density,
            2, 0.5
        );
        lake_val = (lake_val + 1.0) / 2.0;  // 0-1
        
        if (lake_val > lakes.threshold) {
            result.type = FeatureType::LAKE;
            result.intensity = (static_cast<float>(lake_val) - lakes.threshold) / 
                              (1.0f - lakes.threshold);
        }
        
        return result;
    }
    
    // === ROAD SAMPLING ===
    // Roads connect areas, avoid water and steep terrain
    FeatureSample sample_road(int wx, int wy, float elevation) const {
        FeatureSample result;
        if (!roads.enabled) return result;
        
        // Roads on land, not too steep
        if (elevation < roads.min_elev || elevation > roads.max_elev) {
            return result;
        }
        
        // Grid-like roads with variation
        double road_x = road_noise.octave_noise(wx * roads.scale, wy * 0.0008, 2, 0.5);
        double road_y = road_noise.octave_noise(wx * 0.0008, wy * roads.scale, 2, 0.5);
        
        road_x = std::abs(road_x);
        road_y = std::abs(road_y);
        
        if (road_x < roads.threshold || road_y < roads.threshold) {
            result.type = FeatureType::ROAD;
            float ix = road_x < roads.threshold ? 1.0f - (road_x / roads.threshold) : 0.0f;
            float iy = road_y < roads.threshold ? 1.0f - (road_y / roads.threshold) : 0.0f;
            result.intensity = std::max(ix, iy);
        }
        
        return result;
    }
    
    // === COMBINED SAMPLING ===
    // Priority: River > Lake > Road
    FeatureSample sample_features(int wx, int wy, float elevation, float moisture) const {
        FeatureSample best;
        
        // Rivers first
        FeatureSample river = sample_river(wx, wy, elevation, moisture);
        if (river.has_feature()) {
            best = river;
        }
        
        // Lakes
        FeatureSample lake = sample_lake(wx, wy, elevation, moisture);
        if (lake.has_feature() && lake.intensity > best.intensity) {
            best = lake;
        }
        
        // Roads (can cross water for bridges)
        FeatureSample road = sample_road(wx, wy, elevation);
        if (road.has_feature()) {
            if (best.type == FeatureType::RIVER || best.type == FeatureType::LAKE) {
                if (road.intensity > 0.6f) {
                    best.type = FeatureType::BRIDGE;
                    best.is_crossing = true;
                }
            } else if (road.intensity > best.intensity) {
                best = road;
            }
        }
        
        return best;
    }
    
    // Legacy compatibility overload
    FeatureSample sample_features(int wx, int wy, float elev, float moist, int /*biome*/) const {
        return sample_features(wx, wy, elev, moist);
    }
    
    // Apply feature to tile
    void apply_feature_to_tile(Tile& tile, const FeatureSample& feat, 
                               std::mt19937& rng, int /*biome*/ = 0) const {
        if (!feat.has_feature()) return;
        
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        switch (feat.type) {
            case FeatureType::RIVER:
                tile.walkable = false;
                tile.symbol = '~';
                tile.color = feat.intensity > 0.6f ? "blue" : "cyan";
                break;
                
            case FeatureType::LAKE:
                tile.walkable = false;
                if (feat.intensity > 0.4f) {
                    tile.symbol = '~';
                    tile.color = "blue";
                } else {
                    // Lake edge
                    tile.walkable = true;
                    tile.symbol = '"';
                    tile.color = "dark_green";
                }
                break;
                
            case FeatureType::ROAD:
                tile.walkable = true;
                tile.symbol = dist(rng) < 0.8f ? ':' : '.';
                tile.color = "brown";
                break;
                
            case FeatureType::BRIDGE:
                tile.walkable = true;
                tile.symbol = '=';
                tile.color = "brown";
                break;
                
            default:
                break;
        }
    }
    
    // Getters for compatibility
    const RiverConfig& get_river_config() const { return rivers; }
    const LakeConfig& get_lake_config() const { return lakes; }
    const RoadConfig& get_road_config() const { return roads; }
};

// ============================================================================
// BIOME BLENDER
// Smooth transitions between biomes
// ============================================================================

class BiomeBlender {
private:
    PerlinNoise blend_noise;
    
public:
    BiomeBlender(unsigned int seed = 0) : blend_noise(seed + 999) {}
    
    void set_seed(unsigned int seed) {
        blend_noise = PerlinNoise(seed + 999);
    }
    
    float calculate_blend(int wx, int wy) const {
        double noise = blend_noise.noise(wx * 0.08, wy * 0.08);
        return static_cast<float>((noise + 1.0) / 2.0);
    }
    
    TileVariant blend_tiles(const TileVariant& a, const TileVariant& b,
                           float factor, std::mt19937& rng) const {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(rng) < factor ? b : a;
    }
    
    bool should_blend(BiomeType a, BiomeType b) const {
        if (a == b) return false;
        
        auto is_water = [](BiomeType t) {
            return t == BiomeType::OCEAN || t == BiomeType::DEEP_OCEAN || 
                   t == BiomeType::RIVER || t == BiomeType::LAKE;
        };
        
        // Don't blend water with land directly
        return is_water(a) == is_water(b);
    }
};

// ============================================================================
// SUB-FEATURE GENERATOR
// Simplified - minimal extra details to keep visuals clean
// ============================================================================

class BiomeSubFeatureGenerator {
private:
    PerlinNoise detail_noise;
    unsigned int seed;
    
public:
    BiomeSubFeatureGenerator(unsigned int seed = 0) 
        : detail_noise(seed + 777), seed(seed) {}
    
    void set_seed(unsigned int new_seed) {
        seed = new_seed;
        detail_noise = PerlinNoise(seed + 777);
    }
    
    struct SubFeatureResult {
        bool has_feature = false;
        char symbol = '.';
        std::string color = "white";
        bool walkable = true;
    };
    
    // Simplified: returns no sub-features to keep biomes visually clean
    SubFeatureResult sample_sub_feature(int /*wx*/, int /*wy*/, BiomeType /*biome*/, 
                                        float /*micro*/, std::mt19937& /*rng*/) const {
        SubFeatureResult result;
        // No sub-features - keep biomes clean and readable
        return result;
    }
    
private:
    bool is_in_ring(int wx, int wy, int spacing) const {
        int cx = (wx / spacing) * spacing + spacing / 2;
        int cy = (wy / spacing) * spacing + spacing / 2;
        
        unsigned int hash = (cx * 73856093u) ^ (cy * 19349663u) ^ seed;
        if ((hash % 60) != 0) return false;
        
        float dx = static_cast<float>(wx - cx);
        float dy = static_cast<float>(wy - cy);
        float dist = std::sqrt(dx * dx + dy * dy);
        
        return dist >= 3.5f && dist <= 4.5f;
    }
    
    bool is_in_cluster(int wx, int wy, int spacing) const {
        int cx = (wx / spacing) * spacing + spacing / 2;
        int cy = (wy / spacing) * spacing + spacing / 2;
        
        unsigned int hash = (cx * 12345u) ^ (cy * 67890u) ^ seed;
        if ((hash % 40) != 0) return false;
        
        int dx = wx - cx;
        int dy = wy - cy;
        return (dx * dx + dy * dy) <= 9;
    }
};
