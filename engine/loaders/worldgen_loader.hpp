#pragma once
#include "../util/json.hpp"
#include "../world/biome.hpp"
#include <string>
#include <unordered_map>
#include <filesystem>
#include <cmath>

// Noise layer configuration
struct NoiseConfig {
    double scale = 0.01;
    int octaves = 4;
    double persistence = 0.5;
    double lacunarity = 2.0;
};

// Elevation thresholds for terrain types
struct ElevationThresholds {
    float deep_ocean = 0.20f;    // Below = deep ocean
    float ocean = 0.30f;         // Below = shallow ocean
    float coast = 0.35f;         // Below = beach/coast
    float lowland = 0.50f;       // Below = lowlands (plains, deserts)
    float midland = 0.65f;       // Below = midlands (hills, forests)
    float highland = 0.80f;      // Below = highlands (mountains)
    // Above highland = peaks
};

// Moisture thresholds for climate zones
struct MoistureThresholds {
    float arid = 0.20f;          // Below = desert
    float dry = 0.35f;           // Below = dry grassland/savanna
    float moderate = 0.55f;      // Below = temperate
    float wet = 0.75f;           // Below = wet forests
    // Above wet = swamp/jungle
};

// River generation config
struct RiverConfig {
    bool enabled = true;
    
    // Where rivers start (sources in highlands)
    double source_density = 0.003;       // How often river sources spawn
    float source_min_elev = 0.55f;       // Rivers start above this elevation
    float source_max_elev = 0.85f;       // But below peaks
    float source_min_moisture = 0.25f;   // Need some moisture
    
    // River path shape
    double path_scale = 0.012;           // Path noise frequency
    double meander_scale = 0.005;        // Bend frequency
    double meander_strength = 18.0;      // Bend amplitude
    
    // River width (grows toward ocean)
    float base_width = 0.010f;           // Width at source
    float max_width = 0.065f;            // Width at coast
    float width_growth = 2.0f;           // Growth curve (higher = stays narrow longer)
};

// Lake generation config
struct LakeConfig {
    bool enabled = true;
    double density = 0.003;
    float threshold = 0.90f;
    float min_elev = 0.36f;
    float max_elev = 0.65f;
    float min_moisture = 0.45f;
};

// Road generation config
struct RoadConfig {
    bool enabled = false;  // Roads disabled by default
    double scale = 0.008;
    float threshold = 0.022f;
    float min_elev = 0.36f;
    float max_elev = 0.72f;
};

// Structure generation config
struct StructureConfig {
    int spawn_exclusion_radius = 0;  // Radius around (0,0) where no structures spawn (in world units)
    float spawn_chance_multiplier = 1.0f;  // Global structure spawn rate multiplier
};

// Master world generation config
struct WorldGenConfig {
    // Noise layers
    NoiseConfig elevation;   // Primary terrain shape
    NoiseConfig moisture;    // Rainfall/wetness
    NoiseConfig continent;   // Large landmass shapes
    NoiseConfig detail;      // Local variation
    
    // Thresholds
    ElevationThresholds elev_thresh;
    MoistureThresholds moist_thresh;
    
    // Features
    RiverConfig rivers;
    LakeConfig lakes;
    RoadConfig roads;
    StructureConfig structures;
    
    // Blending
    float continent_weight = 0.55f;  // Continental influence on elevation
    float local_weight = 0.45f;      // Local noise influence
    float detail_amplitude = 0.12f;  // How much detail noise affects terrain
};

// ============================================================================
// WORLDGEN LOADER CLASS
// ============================================================================

class WorldGenLoader {
public:
    // Load main config from JSON
    static bool load_config(const std::string& filepath, WorldGenConfig& config) {
        json::Value root;
        if (!json::try_parse_file(filepath, root)) return false;
        
        // Noise configs
        if (root.has("noise")) {
            const auto& n = root["noise"];
            if (n.has("elevation")) parse_noise(n["elevation"], config.elevation);
            if (n.has("moisture")) parse_noise(n["moisture"], config.moisture);
            if (n.has("continent")) parse_noise(n["continent"], config.continent);
            if (n.has("detail")) parse_noise(n["detail"], config.detail);
        }
        
        // Elevation thresholds
        if (root.has("elevation")) {
            const auto& e = root["elevation"];
            config.elev_thresh.deep_ocean = e["deep_ocean"].get_number(0.20);
            config.elev_thresh.ocean = e["ocean"].get_number(0.30);
            config.elev_thresh.coast = e["coast"].get_number(0.35);
            config.elev_thresh.lowland = e["lowland"].get_number(0.50);
            config.elev_thresh.midland = e["midland"].get_number(0.65);
            config.elev_thresh.highland = e["highland"].get_number(0.80);
        }
        
        // Moisture thresholds
        if (root.has("moisture")) {
            const auto& m = root["moisture"];
            config.moist_thresh.arid = m["arid"].get_number(0.20);
            config.moist_thresh.dry = m["dry"].get_number(0.35);
            config.moist_thresh.moderate = m["moderate"].get_number(0.55);
            config.moist_thresh.wet = m["wet"].get_number(0.75);
        }
        
        // River config
        if (root.has("rivers")) {
            const auto& r = root["rivers"];
            config.rivers.enabled = r["enabled"].get_bool(true);
            config.rivers.source_density = r["source_density"].get_number(0.003);
            config.rivers.source_min_elev = r["source_min_elevation"].get_number(0.55);
            config.rivers.source_max_elev = r["source_max_elevation"].get_number(0.85);
            config.rivers.source_min_moisture = r["source_min_moisture"].get_number(0.25);
            config.rivers.path_scale = r["path_scale"].get_number(0.012);
            config.rivers.meander_scale = r["meander_scale"].get_number(0.005);
            config.rivers.meander_strength = r["meander_strength"].get_number(18.0);
            config.rivers.base_width = r["base_width"].get_number(0.010);
            config.rivers.max_width = r["max_width"].get_number(0.065);
            config.rivers.width_growth = r["width_growth"].get_number(2.0);
        }
        
        // Lake config
        if (root.has("lakes")) {
            const auto& l = root["lakes"];
            config.lakes.enabled = l["enabled"].get_bool(true);
            config.lakes.density = l["density"].get_number(0.003);
            config.lakes.threshold = l["threshold"].get_number(0.90);
            config.lakes.min_elev = l["min_elevation"].get_number(0.36);
            config.lakes.max_elev = l["max_elevation"].get_number(0.65);
            config.lakes.min_moisture = l["min_moisture"].get_number(0.45);
        }
        
        // Road config
        if (root.has("roads")) {
            const auto& r = root["roads"];
            config.roads.enabled = r["enabled"].get_bool(true);
            config.roads.scale = r["scale"].get_number(0.008);
            config.roads.threshold = r["threshold"].get_number(0.022);
            config.roads.min_elev = r["min_elevation"].get_number(0.36);
            config.roads.max_elev = r["max_elevation"].get_number(0.72);
        }
        
        // Structure config
        if (root.has("structures")) {
            const auto& s = root["structures"];
            config.structures.spawn_exclusion_radius = static_cast<int>(s["spawn_exclusion_radius"].get_number(0));
            config.structures.spawn_chance_multiplier = s["spawn_chance_multiplier"].get_number(1.0);
        }
        
        // Blending weights
        if (root.has("blending")) {
            const auto& b = root["blending"];
            config.continent_weight = b["continent_weight"].get_number(0.55);
            config.local_weight = b["local_weight"].get_number(0.45);
            config.detail_amplitude = b["detail_amplitude"].get_number(0.12);
        }
        
        return true;
    }
    
    // Load biome definitions from JSON
    static bool load_biomes(const std::string& filepath, std::unordered_map<BiomeType, BiomeData>& biomes) {
        json::Value root;
        if (!json::try_parse_file(filepath, root)) return false;
        if (!root.has("biomes") || !root["biomes"].is_array()) return false;
        
        for (const auto& b : root["biomes"].as_array()) {
            BiomeData data;
            std::string id = b["id"].get_string("plains");
            data.type = string_to_biome(id);
            data.name = b["name"].get_string("Unknown");
            data.description = b["description"].get_string("");
            
            // Tiles
            if (b.has("tiles") && b["tiles"].is_array()) {
                for (const auto& t : b["tiles"].as_array()) {
                    std::string s = t["symbol"].get_string(".");
                    char sym = s.empty() ? '.' : s[0];
                    data.add_tile(sym, 
                        t["color"].get_string("white"),
                        t["walkable"].get_bool(true),
                        t["weight"].get_number(1.0),
                        t["name"].get_string("ground"),
                        t["movement_cost"].get_number(1.0));
                }
            }
            
            // Features
            if (b.has("features") && b["features"].is_array()) {
                for (const auto& f : b["features"].as_array()) {
                    std::string s = f["symbol"].get_string("*");
                    char sym = s.empty() ? '*' : s[0];
                    data.add_feature(
                        f["name"].get_string("feature"),
                        sym,
                        f["color"].get_string("white"),
                        f["walkable"].get_bool(true),
                        f["spawn_chance"].get_number(0.01),
                        f["cluster_chance"].get_number(0.0));
                }
            }
            
            // Thresholds
            if (b.has("thresholds")) {
                const auto& t = b["thresholds"];
                data.tree_threshold = t["tree"].get_number(0.7);
                data.rock_threshold = t["rock"].get_number(0.1);
                data.water_threshold = t["water"].get_number(0.05);
                data.flower_threshold = t["flower"].get_number(0.92);
            }
            
            // Modifiers
            if (b.has("modifiers")) {
                const auto& m = b["modifiers"];
                data.movement_cost = m["movement_cost"].get_number(1.0);
                data.visibility_range = m["visibility_range"].get_number(1.0);
                data.allows_structures = m["allows_structures"].get_bool(true);
            }
            
            biomes[data.type] = data;
        }
        
        return !biomes.empty();
    }
    
    // Find assets path
    static std::string find_assets_path(const std::string& base = "") {
        std::vector<std::string> paths = {
            base + "assets/worldgen/",
            base + "../assets/worldgen/",
            "assets/worldgen/",
            "../assets/worldgen/"
        };
        for (const auto& p : paths) {
            if (std::filesystem::exists(p)) return p;
        }
        return "assets/worldgen/";
    }

private:
    static void parse_noise(const json::Value& obj, NoiseConfig& cfg) {
        cfg.scale = obj["scale"].get_number(0.01);
        cfg.octaves = obj["octaves"].get_int(4);
        cfg.persistence = obj["persistence"].get_number(0.5);
        cfg.lacunarity = obj["lacunarity"].get_number(2.0);
    }
    
    static BiomeType string_to_biome(const std::string& id) {
        static const std::unordered_map<std::string, BiomeType> map = {
            {"ocean", BiomeType::OCEAN}, {"deep_ocean", BiomeType::DEEP_OCEAN},
            {"beach", BiomeType::BEACH}, {"plains", BiomeType::PLAINS},
            {"forest", BiomeType::FOREST}, {"dense_forest", BiomeType::DENSE_FOREST},
            {"hills", BiomeType::HILLS}, {"mountains", BiomeType::MOUNTAINS},
            {"snow_peaks", BiomeType::SNOW_PEAKS}, {"desert", BiomeType::DESERT},
            {"desert_dunes", BiomeType::DESERT_DUNES}, {"swamp", BiomeType::SWAMP},
            {"tundra", BiomeType::TUNDRA}, {"frozen_tundra", BiomeType::FROZEN_TUNDRA},
            {"savanna", BiomeType::SAVANNA}, {"jungle", BiomeType::JUNGLE},
            {"tropical_beach", BiomeType::TROPICAL_BEACH}, {"volcanic", BiomeType::VOLCANIC},
            {"mushroom_forest", BiomeType::MUSHROOM_FOREST}, {"dead_lands", BiomeType::DEAD_LANDS},
            {"river", BiomeType::RIVER}, {"lake", BiomeType::LAKE}, {"road", BiomeType::ROAD}
        };
        auto it = map.find(id);
        return it != map.end() ? it->second : BiomeType::PLAINS;
    }
};

// ============================================================================
// BIOME SELECTION - Pure Elevation + Moisture Logic
// ============================================================================

inline BiomeType select_biome(float elev, float moist, 
                               const ElevationThresholds& e, 
                               const MoistureThresholds& m) {
    // === WATER ===
    if (elev < e.deep_ocean) return BiomeType::DEEP_OCEAN;
    if (elev < e.ocean) return BiomeType::OCEAN;
    
    // === COAST ===
    if (elev < e.coast) {
        return (moist > m.wet) ? BiomeType::SWAMP : BiomeType::BEACH;
    }
    
    // === PEAKS ===
    if (elev >= e.highland) {
        return BiomeType::SNOW_PEAKS;
    }
    
    // === HIGHLANDS (mountains) ===
    if (elev >= e.midland) {
        if (moist < m.arid) return BiomeType::MOUNTAINS;
        if (moist > m.wet) return BiomeType::FROZEN_TUNDRA;
        return BiomeType::MOUNTAINS;
    }
    
    // === MIDLANDS (hills, forests) ===
    if (elev >= e.lowland) {
        if (moist < m.arid) return BiomeType::HILLS;
        if (moist < m.dry) return BiomeType::SAVANNA;
        if (moist < m.moderate) return BiomeType::HILLS;
        if (moist < m.wet) return BiomeType::FOREST;
        return BiomeType::DENSE_FOREST;
    }
    
    // === LOWLANDS ===
    if (moist < m.arid) return BiomeType::DESERT;
    if (moist < m.dry) return BiomeType::DESERT_DUNES;
    if (moist < m.moderate) return BiomeType::PLAINS;
    if (moist < m.wet) return BiomeType::FOREST;
    return BiomeType::SWAMP;
}
