#pragma once
#include "chunk.hpp"
#include "perlin_noise.hpp"
#include "../loaders/structure_loader.hpp"
#include "jigsaw_generator.hpp"
#include "biome.hpp"
#include "biome_features.hpp"
#include "tile_drawing_utils.hpp"
#include "tileset.hpp"
#include "../loaders/worldgen_loader.hpp"
#include <random>
#include <cmath>
#include <climits>
#include <vector>
#include <string>

// Import tile drawing utilities into this scope for convenience
using namespace TileDrawing;

// Structure types for procedural generation
enum class StructureType {
    NONE = -1,
    // Basic structures (original)
    RUINS = 0,
    CAMP = 1,
    STONE_CIRCLE = 2,
    // Villages and settlements
    VILLAGE = 3,
    FARM = 4,
    OUTPOST = 5,
    // Dungeon entrances
    DUNGEON_ENTRANCE = 6,
    CAVE_ENTRANCE = 7,
    MINE_ENTRANCE = 8,
    // Larger structures
    TOWER = 9,
    FORTRESS = 10,
    TEMPLE = 11,
    GRAVEYARD = 12,
    // Natural formations
    OASIS = 13,
    HOT_SPRING = 14,
    FAIRY_RING = 15,
    // Bridges and roads
    BRIDGE = 16,
    CROSSROADS = 17,
    WAYSHRINE = 18,
    // Special
    WITCH_HUT = 19,
    BANDIT_HIDEOUT = 20,
    ABANDONED_CASTLE = 21,
    DRAGON_LAIR = 22,
    // Biome specific
    FISHING_VILLAGE = 23,
    MOUNTAIN_PASS = 24,
    DESERT_TEMPLE = 25,
    // Starting area
    STARTING_HUT = 26
};

class ChunkGenerator {
private:
    PerlinNoise noise;
    PerlinNoise moisture_noise;     // Moisture/rainfall layer
    PerlinNoise temperature_noise;  // Temperature layer for biome variety
    PerlinNoise detail_noise;       // Fine detail for terrain variation
    PerlinNoise continent_noise;    // Very large scale continental shapes
    unsigned int seed;
    std::mt19937 rng;
    StructureLoader structure_loader;
    JigsawGenerator jigsaw_generator;
    bool json_structures_loaded = false;
    
    // Modular biome and feature systems
    BiomeFeatureGenerator feature_generator;
    BiomeBlender biome_blender;
    BiomeSubFeatureGenerator sub_feature_generator;
    
    // Scale constants for truly massive biomes
    static constexpr double CONTINENT_SCALE = 0.002;    // Huge continental masses
    static constexpr double BIOME_SCALE = 0.006;        // Large biome regions
    static constexpr double MOISTURE_SCALE = 0.004;     // Large moisture zones
    static constexpr double TEMPERATURE_SCALE = 0.003;  // Temperature gradients
    static constexpr double DETAIL_SCALE = 0.02;        // Local terrain detail
    static constexpr double MICRO_SCALE = 0.08;         // Fine details like rocks, plants
    
    // Minimum distance between villages (in chunks) to prevent clustering
    static constexpr int MIN_VILLAGE_DISTANCE_CHUNKS = 8;
    
    // Structure size definitions (width, height) for each structure type
    std::pair<int, int> get_structure_size(StructureType type) const {
        switch (type) {
            case StructureType::VILLAGE:          return {104, 104};  // Large, multi-chunk (52 radius for bigger buildings)
            case StructureType::FISHING_VILLAGE:  return {48, 48};  // Increased for pier extensions
            case StructureType::FORTRESS:         return {48, 48};
            case StructureType::ABANDONED_CASTLE: return {56, 56};
            case StructureType::FARM:             return {32, 32};
            case StructureType::OUTPOST:          return {24, 24};
            case StructureType::TEMPLE:           return {30, 30};
            case StructureType::DESERT_TEMPLE:    return {32, 32};
            case StructureType::GRAVEYARD:        return {26, 26};
            case StructureType::DRAGON_LAIR:      return {36, 36};
            case StructureType::TOWER:            return {20, 20};
            case StructureType::RUINS:            return {18, 18};
            case StructureType::CAMP:             return {16, 16};
            case StructureType::STONE_CIRCLE:     return {14, 14};
            case StructureType::BANDIT_HIDEOUT:   return {22, 22};
            case StructureType::WITCH_HUT:        return {12, 12};
            case StructureType::DUNGEON_ENTRANCE: return {16, 16};
            case StructureType::CAVE_ENTRANCE:    return {14, 14};
            case StructureType::MINE_ENTRANCE:    return {16, 16};
            case StructureType::OASIS:            return {20, 20};
            case StructureType::HOT_SPRING:       return {16, 16};
            case StructureType::FAIRY_RING:       return {12, 12};
            case StructureType::CROSSROADS:       return {18, 18};
            case StructureType::WAYSHRINE:        return {10, 10};
            case StructureType::BRIDGE:           return {12, 20};
            case StructureType::MOUNTAIN_PASS:    return {20, 30};
            case StructureType::STARTING_HUT:     return {18, 18};
            default:                              return {16, 16};
        }
    }
    
public:
    ChunkGenerator(unsigned int seed) 
        : noise(seed), moisture_noise(seed + 12345), 
          temperature_noise(seed + 54321), detail_noise(seed + 99999),
          continent_noise(seed + 77777), seed(seed), rng(seed),
          feature_generator(seed), biome_blender(seed), sub_feature_generator(seed) {}
    
    void set_seed(unsigned int new_seed) {
        seed = new_seed;
        noise = PerlinNoise(seed);
        moisture_noise = PerlinNoise(seed + 12345);
        temperature_noise = PerlinNoise(seed + 54321);
        detail_noise = PerlinNoise(seed + 99999);
        continent_noise = PerlinNoise(seed + 77777);
        rng.seed(seed);
        feature_generator.set_seed(seed);
        biome_blender.set_seed(seed);
        sub_feature_generator.set_seed(seed);
    }
    
    // Load JSON structure definitions from a directory
    int load_structures_from_json(const std::string& directory) {
        int count = structure_loader.load_all_structures(directory);
        json_structures_loaded = (count > 0);
        return count;
    }
    
    // Worldgen configuration
    WorldGenConfig worldgen_config;
    
    // Load worldgen configuration from JSON files
    bool load_worldgen_config(const std::string& assets_path) {
        bool success = true;
        
        // Load biomes from JSON
        std::string biomes_path = assets_path + "worldgen/biomes.json";
        std::unordered_map<BiomeType, BiomeData> loaded_biomes;
        if (WorldGenLoader::load_biomes(biomes_path, loaded_biomes)) {
            get_biome_registry().load_from_json(loaded_biomes);
        } else {
            success = false;
        }
        
        // Load world parameters from JSON
        std::string params_path = assets_path + "worldgen/world_parameters.json";
        if (WorldGenLoader::load_config(params_path, worldgen_config)) {
            // Apply config to feature generator
            feature_generator.set_config(worldgen_config);
        } else {
            success = false;
        }
        
        return success;
    }
    
    // Check if we have JSON definition for a structure type
    bool has_json_structure(int type_id) const {
        return structure_loader.has_json_definition(type_id);
    }
    
    // Get the structure loader for external use
    const StructureLoader& get_structure_loader() const {
        return structure_loader;
    }
    
    // Get biome using pure elevation + moisture (temperature derived from elevation)
    BiomeType get_biome(double height, double moisture, double /*temperature*/) {
        return select_biome(
            static_cast<float>(height), 
            static_cast<float>(moisture),
            worldgen_config.elev_thresh,
            worldgen_config.moist_thresh
        );
    }
    

    
    // Sample all terrain values at a world position
    struct TerrainSample {
        double height;
        double moisture;
        double detail;
        BiomeType biome;
        FeatureType feature = FeatureType::NONE;
        float feature_intensity = 0.0f;
    };
    
    TerrainSample sample_terrain(int world_x, int world_y) {
        TerrainSample sample;
        
        // === SINGLE CONTINENT - Radial falloff from center ===
        // Continent centered at (0,0), large radius for expansive landmass
        const double continent_radius = 2500.0;
        const double ocean_width = 600.0;  // Wide transition zone
        
        double dist = std::sqrt(static_cast<double>(world_x * world_x + world_y * world_y));
        
        // Radial falloff: 1.0 at center, 0.0 at edge
        double radial = 1.0 - (dist / (continent_radius + ocean_width));
        radial = std::max(0.0, std::min(1.0, radial));
        
        // Multi-octave coastline noise for interesting, organic outline
        // Large-scale bays and peninsulas
        double coast_large = continent_noise.octave_noise(
            world_x * 0.002, 
            world_y * 0.002, 
            2, 0.6
        );
        // Medium-scale coastal features
        double coast_medium = continent_noise.octave_noise(
            world_x * 0.006 + 500, 
            world_y * 0.006 + 500, 
            3, 0.5
        );
        // Small-scale coastal details
        double coast_small = continent_noise.octave_noise(
            world_x * 0.015 + 1000, 
            world_y * 0.015 + 1000, 
            2, 0.4
        );
        
        // Combine for varied coastline with bays, peninsulas, and fjords
        double coast_noise = coast_large * 0.4 + coast_medium * 0.35 + coast_small * 0.15;
        radial += coast_noise * 0.45;  // Significant coastline variation
        
        // Local elevation detail for terrain variety
        double local = noise.octave_noise(
            world_x * worldgen_config.elevation.scale, 
            world_y * worldgen_config.elevation.scale, 
            worldgen_config.elevation.octaves, 
            worldgen_config.elevation.persistence
        );
        local = (local + 1.0) / 2.0;  // 0-1
        
        // === LAND vs OCEAN ===
        const double shore_threshold = 0.35;
        
        if (radial < shore_threshold) {
            // OCEAN
            double depth = (shore_threshold - radial) / shore_threshold;
            sample.height = 0.25 - depth * 0.20;  // 0.05 (deep) to 0.25 (shallow)
        } else {
            // LAND - radial controls base elevation, local adds variety
            double land_progress = (radial - shore_threshold) / (1.0 - shore_threshold);
            double base_height = 0.35 + land_progress * 0.35;  // Coast to highlands
            sample.height = base_height * 0.6 + local * 0.4;  // Mix with terrain noise
        }
        
        // Add fine detail
        sample.detail = detail_noise.octave_noise(
            world_x * worldgen_config.detail.scale, 
            world_y * worldgen_config.detail.scale, 
            worldgen_config.detail.octaves, 
            worldgen_config.detail.persistence
        );
        sample.detail = (sample.detail + 1.0) / 2.0;
        sample.height += (sample.detail - 0.5) * worldgen_config.detail_amplitude;
        sample.height = std::max(0.0, std::min(1.0, sample.height));
        
        // === MOISTURE MAP ===
        sample.moisture = moisture_noise.octave_noise(
            world_x * worldgen_config.moisture.scale + 1000, 
            world_y * worldgen_config.moisture.scale + 1000, 
            worldgen_config.moisture.octaves, 
            worldgen_config.moisture.persistence
        );
        sample.moisture = (sample.moisture + 1.0) / 2.0;
        
        // === BIOME FROM ELEVATION + MOISTURE ===
        sample.biome = select_biome(
            static_cast<float>(sample.height), 
            static_cast<float>(sample.moisture),
            worldgen_config.elev_thresh,
            worldgen_config.moist_thresh
        );

        
        // === FEATURES (rivers, lakes, roads) ===
        FeatureSample feature = feature_generator.sample_features(
            world_x, world_y,
            static_cast<float>(sample.height),
            static_cast<float>(sample.moisture)
        );
        sample.feature = feature.type;
        sample.feature_intensity = feature.intensity;
        
        return sample;
    }

    // Generate terrain for a chunk using multiple noise layers and modular biome system
    void generate_terrain_chunk(Chunk* chunk) {
        if (chunk->is_generated()) return;
        
        ChunkCoord coord = chunk->get_coord();
        int world_x_start = coord.x * CHUNK_SIZE;
        int world_y_start = coord.y * CHUNK_SIZE;
        
        // Create a deterministic RNG for this chunk
        std::mt19937 chunk_rng(seed ^ (coord.x * 73856093) ^ (coord.y * 19349663));
        
        for (int local_y = 0; local_y < CHUNK_SIZE; ++local_y) {
            for (int local_x = 0; local_x < CHUNK_SIZE; ++local_x) {
                int world_x = world_x_start + local_x;
                int world_y = world_y_start + local_y;
                
                TerrainSample terrain = sample_terrain(world_x, world_y);
                
                // Micro-detail for placing objects
                double micro = detail_noise.octave_noise(
                    world_x * MICRO_SCALE, 
                    world_y * MICRO_SCALE, 
                    2, 0.5
                );
                micro = (micro + 1.0) / 2.0;
                
                // Set tile based on biome using the modular biome registry
                set_biome_tile_modular(chunk, local_x, local_y, terrain, micro, world_x, world_y, chunk_rng);
            }
        }
        
        chunk->set_generated(true);
    }
    
private:
    // Modular biome tile setter using BiomeRegistry and feature systems
    void set_biome_tile_modular(Chunk* chunk, int x, int y, const TerrainSample& terrain, 
                                 double micro, int world_x, int world_y, std::mt19937& chunk_rng) {
        // Get biome data from registry
        const BiomeData& biome_data = get_biome_registry().get(terrain.biome);
        
        // Sample terrain features (rivers, lakes, roads)
        FeatureSample feature = feature_generator.sample_features(
            world_x, world_y, 
            static_cast<float>(terrain.height),
            static_cast<float>(terrain.moisture)
        );
        
        // If there's a feature, apply it
        if (feature.has_feature()) {
            Tile tile(true, '.', "white");
            feature_generator.apply_feature_to_tile(tile, feature, chunk_rng);
            chunk->set_tile(x, y, tile);
            return;
        }
        
        // Check for sub-features (fairy rings, boulder clusters, etc.)
        auto sub_feature = sub_feature_generator.sample_sub_feature(
            world_x, world_y, terrain.biome, static_cast<float>(micro), chunk_rng
        );
        
        if (sub_feature.has_feature) {
            chunk->set_tile(x, y, Tile(sub_feature.walkable, sub_feature.symbol, sub_feature.color));
            return;
        }
        
        // Normal biome tile selection using the registry
        const TileVariant& tile_variant = biome_data.select_tile(chunk_rng);
        
        // Apply micro-detail variations based on biome thresholds
        char symbol = tile_variant.symbol;
        std::string color = tile_variant.color;
        bool walkable = tile_variant.walkable;
        float movement_cost = tile_variant.movement_cost;
        
        // Apply threshold-based features
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        // Trees (if above tree threshold and not already a tree)
        if (micro > biome_data.tree_threshold && tile_variant.walkable) {
            switch (terrain.biome) {
                case BiomeType::FOREST:
                case BiomeType::DENSE_FOREST:
                    symbol = 'T';
                    color = dist(chunk_rng) < 0.5f ? "green" : "dark_green";
                    walkable = true;
                    movement_cost = 0.5f;  // Trees slow movement
                    break;
                case BiomeType::JUNGLE:
                    symbol = 'T';
                    color = "green";
                    walkable = true;
                    movement_cost = 0.5f;  // Trees slow movement
                    break;
                case BiomeType::SAVANNA:
                    if (dist(chunk_rng) < 0.3f) {
                        symbol = 'T';
                        color = "brown";
                        walkable = true;
                        movement_cost = 0.5f;  // Trees slow movement
                    }
                    break;
                default:
                    break;
            }
        }
        
        // Rocks (if below rock threshold)
        if (micro < biome_data.rock_threshold && tile_variant.walkable) {
            symbol = 'o';
            color = "gray";
            walkable = dist(chunk_rng) < 0.3f ? false : true;
        }
        
        // Water pools (if below water threshold in wet biomes)
        if (micro < biome_data.water_threshold && terrain.moisture > 0.5 && tile_variant.walkable) {
            symbol = '~';
            color = terrain.biome == BiomeType::SWAMP ? "dark_green" : "blue";
            walkable = false;
        }
        
        // Flowers/decorations (if above flower threshold)
        if (micro > biome_data.flower_threshold && tile_variant.walkable) {
            switch (terrain.biome) {
                case BiomeType::PLAINS:
                    symbol = '*';
                    color = dist(chunk_rng) < 0.4f ? "yellow" : 
                            (dist(chunk_rng) < 0.7f ? "magenta" : "cyan");
                    break;
                case BiomeType::FOREST:
                    if (dist(chunk_rng) < 0.3f) {
                        symbol = '*';
                        color = dist(chunk_rng) < 0.5f ? "red" : "white";
                    }
                    break;
                case BiomeType::JUNGLE:
                    symbol = '*';
                    color = dist(chunk_rng) < 0.5f ? "magenta" : "red";
                    break;
                case BiomeType::TUNDRA:
                    symbol = '*';
                    color = "cyan";
                    break;
                default:
                    break;
            }
        }
        
        chunk->set_tile(x, y, Tile(walkable, symbol, color, movement_cost));
    }
    
public:
    
    // Determine structure type based on chunk location and biome
    StructureType determine_structure_type(Chunk* chunk, std::mt19937& chunk_rng) {
        ChunkCoord coord = chunk->get_coord();
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        // Get biome at chunk center
        int world_cx = coord.x * CHUNK_SIZE + CHUNK_SIZE / 2;
        int world_cy = coord.y * CHUNK_SIZE + CHUNK_SIZE / 2;
        
        TerrainSample terrain = sample_terrain(world_cx, world_cy);
        BiomeType biome = terrain.biome;
        
        // Near origin (starting area) has guaranteed village
        if (std::abs(coord.x) <= 1 && std::abs(coord.y) <= 1 && coord.x == 1 && coord.y == 0) {
            return StructureType::VILLAGE;
        }
        
        float roll = dist(chunk_rng);
        
        // Structure spawn chances vary by biome
        switch (biome) {
            // Water biomes - rare structures
            case BiomeType::OCEAN:
            case BiomeType::DEEP_OCEAN:
                if (roll < 0.03f) return StructureType::FISHING_VILLAGE;
                break;
                
            // Beach biomes
            case BiomeType::BEACH:
            case BiomeType::TROPICAL_BEACH:
                if (roll < 0.05f) return StructureType::FISHING_VILLAGE;
                if (roll < 0.07f) return StructureType::RUINS;
                break;
                
            // Plains - most structures
            case BiomeType::PLAINS:
                if (roll < 0.02f) return StructureType::VILLAGE;
                if (roll < 0.06f) return StructureType::FARM;
                if (roll < 0.15f) return StructureType::CROSSROADS;
                if (roll < 0.17f) return StructureType::CAMP;
                if (roll < 0.19f) return StructureType::WAYSHRINE;
                if (roll < 0.20f) return StructureType::OUTPOST;
                if (roll < 0.21f) return StructureType::DUNGEON_ENTRANCE;
                break;
                
            // Forest biomes
            case BiomeType::FOREST:
            case BiomeType::DENSE_FOREST:
                if (roll < 0.01f) return StructureType::VILLAGE;
                if (roll < 0.03f) return StructureType::WITCH_HUT;
                if (roll < 0.08f) return StructureType::BANDIT_HIDEOUT;
                if (roll < 0.10f) return StructureType::FAIRY_RING;
                if (roll < 0.12f) return StructureType::RUINS;
                if (roll < 0.14f) return StructureType::CAMP;
                if (roll < 0.15f) return StructureType::CAVE_ENTRANCE;
                break;
                
            case BiomeType::MUSHROOM_FOREST:
                if (roll < 0.05f) return StructureType::FAIRY_RING;
                if (roll < 0.10f) return StructureType::WITCH_HUT;
                if (roll < 0.12f) return StructureType::RUINS;
                break;
                
            // Hills
            case BiomeType::HILLS:
                if (roll < 0.01f) return StructureType::VILLAGE;
                if (roll < 0.03f) return StructureType::TOWER;
                if (roll < 0.08f) return StructureType::MINE_ENTRANCE;
                if (roll < 0.10f) return StructureType::STONE_CIRCLE;
                if (roll < 0.12f) return StructureType::GRAVEYARD;
                if (roll < 0.14f) return StructureType::RUINS;
                break;
                
            // Mountain biomes
            case BiomeType::MOUNTAINS:
                if (roll < 0.02f) return StructureType::FORTRESS;
                if (roll < 0.04f) return StructureType::DRAGON_LAIR;
                if (roll < 0.06f) return StructureType::MOUNTAIN_PASS;
                if (roll < 0.08f) return StructureType::CAVE_ENTRANCE;
                if (roll < 0.10f) return StructureType::ABANDONED_CASTLE;
                break;
                
            case BiomeType::SNOW_PEAKS:
                if (roll < 0.03f) return StructureType::CAVE_ENTRANCE;
                if (roll < 0.05f) return StructureType::RUINS;
                break;
                
            case BiomeType::VOLCANIC:
                if (roll < 0.03f) return StructureType::DRAGON_LAIR;
                if (roll < 0.06f) return StructureType::RUINS;
                if (roll < 0.08f) return StructureType::CAVE_ENTRANCE;
                break;
                
            // Desert biomes
            case BiomeType::DESERT:
            case BiomeType::DESERT_DUNES:
                if (roll < 0.015f) return StructureType::VILLAGE;
                if (roll < 0.035f) return StructureType::DESERT_TEMPLE;
                if (roll < 0.10f) return StructureType::OASIS;
                if (roll < 0.12f) return StructureType::RUINS;
                if (roll < 0.14f) return StructureType::CAMP;
                break;
                
            // Swamp
            case BiomeType::SWAMP:
                if (roll < 0.005f) return StructureType::VILLAGE;
                if (roll < 0.025f) return StructureType::WITCH_HUT;
                if (roll < 0.07f) return StructureType::HOT_SPRING;
                if (roll < 0.09f) return StructureType::RUINS;
                if (roll < 0.11f) return StructureType::GRAVEYARD;
                break;
                
            // Tundra biomes
            case BiomeType::TUNDRA:
            case BiomeType::FROZEN_TUNDRA:
                if (roll < 0.01f) return StructureType::VILLAGE;
                if (roll < 0.03f) return StructureType::OUTPOST;
                if (roll < 0.08f) return StructureType::CAVE_ENTRANCE;
                if (roll < 0.10f) return StructureType::RUINS;
                if (roll < 0.12f) return StructureType::CAMP;
                break;
                
            // Savanna
            case BiomeType::SAVANNA:
                if (roll < 0.02f) return StructureType::VILLAGE;
                if (roll < 0.05f) return StructureType::CAMP;
                if (roll < 0.13f) return StructureType::WAYSHRINE;
                if (roll < 0.15f) return StructureType::OASIS;
                break;
                
            // Jungle
            case BiomeType::JUNGLE:
                if (roll < 0.005f) return StructureType::VILLAGE;
                if (roll < 0.03f) return StructureType::TEMPLE;
                if (roll < 0.09f) return StructureType::RUINS;
                if (roll < 0.11f) return StructureType::CAVE_ENTRANCE;
                if (roll < 0.13f) return StructureType::FAIRY_RING;
                break;
            
            default:
                break;
        }
        
        return StructureType::NONE;
    }
    
    // Flag to track if starting structure has been planned
    bool starting_structure_planned = false;
    
    // Ensure the starting structure is always placed at world origin
    void ensure_starting_structure(ChunkedWorld* world) {
        if (starting_structure_planned) return;
        starting_structure_planned = true;
        
        // Place starting hut centered in chunk (0,0) so player spawns near it
        // Chunk (0,0) spans from world coords (0,0) to (CHUNK_SIZE-1, CHUNK_SIZE-1)
        // Center it at the middle of the chunk
        int world_x = CHUNK_SIZE / 2;
        int world_y = CHUNK_SIZE / 2;
        
        auto [width, height] = get_structure_size(StructureType::STARTING_HUT);
        unsigned int struct_seed = seed ^ 0xDEADBEEF;  // Fixed seed for consistency
        
        // Look up JSON structure definition
        std::string structure_def_id = "starting_hut";
        int type_id = static_cast<int>(StructureType::STARTING_HUT);
        
        // Register the starting structure
        world->plan_structure(world_x, world_y, width, height, type_id, struct_seed, structure_def_id);
        
        // Mark the origin chunk as planned
        ChunkCoord origin_coord(0, 0);
        world->mark_structures_planned(origin_coord);
    }
    
    // Plan structures for a chunk and its neighbors (call before generating chunks)
    // This ensures structures that span chunk boundaries are properly planned
    void plan_structures_for_region(ChunkedWorld* world, ChunkCoord center_coord) {
        // Force spawn the starting hut at origin (0,0) if not already planned
        ensure_starting_structure(world);
        
        // Check a 5x5 area around this chunk for structure planning
        // This ensures large structures (up to 64x64) that span multiple chunks are caught
        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                ChunkCoord coord(center_coord.x + dx, center_coord.y + dy);
                
                // Skip if already planned
                if (world->has_structures_planned(coord)) continue;
                
                // Mark as planned
                world->mark_structures_planned(coord);
                
                // Deterministic RNG for this chunk's structure decision
                std::mt19937 chunk_rng(seed ^ (coord.x * 73856093) ^ (coord.y * 19349663));
                
                // Create a temporary chunk to check biome (won't be stored)
                Chunk temp_chunk(coord);
                StructureType type = determine_structure_type(&temp_chunk, chunk_rng);
                
                if (type == StructureType::NONE) continue;
                
                // Check spawn exclusion radius around origin (0,0)
                int exclusion_radius = worldgen_config.structures.spawn_exclusion_radius;
                if (exclusion_radius > 0) {
                    int chunk_center_x = coord.x * CHUNK_SIZE + CHUNK_SIZE / 2;
                    int chunk_center_y = coord.y * CHUNK_SIZE + CHUNK_SIZE / 2;
                    int dist_sq = chunk_center_x * chunk_center_x + chunk_center_y * chunk_center_y;
                    if (dist_sq < exclusion_radius * exclusion_radius) {
                        continue;  // Too close to origin
                    }
                }
                
                // Anti-clustering check for villages - ensure minimum distance between villages
                bool is_village = (type == StructureType::VILLAGE || type == StructureType::FISHING_VILLAGE);
                if (is_village) {
                    bool too_close_village = world->has_nearby_structure_type(
                        coord.x, coord.y, static_cast<int>(StructureType::VILLAGE), MIN_VILLAGE_DISTANCE_CHUNKS);
                    bool too_close_fishing = world->has_nearby_structure_type(
                        coord.x, coord.y, static_cast<int>(StructureType::FISHING_VILLAGE), MIN_VILLAGE_DISTANCE_CHUNKS);
                    
                    if (too_close_village || too_close_fishing) {
                        // Skip this village - too close to another
                        continue;
                    }
                }
                
                // Get structure size
                auto [width, height] = get_structure_size(type);
                
                // Calculate world position for structure center
                // For large structures, place closer to chunk center to minimize edge issues
                // Small margin based on structure size - larger structures get centered more
                int half_chunk = CHUNK_SIZE / 2;
                int margin_x = std::min(half_chunk - 2, width / 4);
                int margin_y = std::min(half_chunk - 2, height / 4);
                
                // Position range is narrower for larger structures
                int min_pos = half_chunk - margin_x;
                int max_pos_x = half_chunk + margin_x;
                int max_pos_y = half_chunk + margin_y;
                
                std::uniform_int_distribution<int> pos_dist_x(min_pos, max_pos_x);
                std::uniform_int_distribution<int> pos_dist_y(min_pos, max_pos_y);
                
                int local_x = pos_dist_x(chunk_rng);
                int local_y = pos_dist_y(chunk_rng);
                
                int world_x = coord.x * CHUNK_SIZE + local_x;
                int world_y = coord.y * CHUNK_SIZE + local_y;
                
                // Create structure seed for consistent generation
                unsigned int struct_seed = seed ^ (world_x * 12345) ^ (world_y * 67890);
                
                // Look up JSON structure definition to get the string ID
                std::string structure_def_id = "";
                int type_id = static_cast<int>(type);
                if (structure_loader.has_json_definition(type_id)) {
                    const StructureDefinition* def = structure_loader.get_structure_by_type(type_id);
                    if (def && def->valid) {
                        structure_def_id = def->id;
                    }
                }
                
                // Register the planned structure with JSON ID
                world->plan_structure(world_x, world_y, width, height, type_id, struct_seed, structure_def_id);
            }
        }
    }
    
    // Generate structures in a chunk (multi-chunk aware)
    void generate_structures_in_chunk(Chunk* chunk, ChunkedWorld* world) {
        ChunkCoord coord = chunk->get_coord();
        
        // First, plan structures for this region if not done
        plan_structures_for_region(world, coord);
        
        // Get all structures that overlap this chunk
        auto structures = world->get_structures_for_chunk(coord.x, coord.y);
        
        if (structures.empty()) {
            chunk->set_structure_type(-1);
            return;
        }
        
        // Find the primary structure (the one whose center is closest to this chunk's center)
        int chunk_center_x = coord.x * CHUNK_SIZE + CHUNK_SIZE / 2;
        int chunk_center_y = coord.y * CHUNK_SIZE + CHUNK_SIZE / 2;
        
        const PlannedStructure* primary = nullptr;
        int min_dist = INT_MAX;
        
        for (const PlannedStructure* s : structures) {
            int dx = s->world_x - chunk_center_x;
            int dy = s->world_y - chunk_center_y;
            int dist = dx * dx + dy * dy;
            if (dist < min_dist) {
                min_dist = dist;
                primary = s;
            }
        }
        
        if (primary) {
            // Store primary structure info in chunk for world map display
            chunk->set_structure_type(primary->type);
            chunk->set_structure_definition_id(primary->structure_id);
            // Convert world center to local coordinates
            int local_cx = primary->world_x - coord.x * CHUNK_SIZE;
            int local_cy = primary->world_y - coord.y * CHUNK_SIZE;
            chunk->set_structure_center(local_cx, local_cy);
            
            // Check if this chunk actually contains the structure center
            bool center_in_chunk = (local_cx >= 0 && local_cx < CHUNK_SIZE && 
                                    local_cy >= 0 && local_cy < CHUNK_SIZE);
            chunk->set_primary_structure(center_in_chunk);
        }
        
        // Render all structures that overlap this chunk
        for (const PlannedStructure* s : structures) {
            render_structure_to_chunk(chunk, *s, world);
        }
    }
    
    // Old signature for backwards compatibility
    // Check if a world position is walkable
    bool is_walkable(ChunkedWorld* world, int world_x, int world_y) {
        ChunkCoord coord(world_x / CHUNK_SIZE, world_y / CHUNK_SIZE);
        Chunk* chunk = world->get_chunk(coord);
        
        if (!chunk->is_generated()) {
            generate_terrain_chunk(chunk);
        }
        
        int local_x = world_x % CHUNK_SIZE;
        int local_y = world_y % CHUNK_SIZE;
        if (local_x < 0) local_x += CHUNK_SIZE;
        if (local_y < 0) local_y += CHUNK_SIZE;
        
        return chunk->get_tile(local_x, local_y).walkable;
    }
    
    // Find nearest walkable position
    bool find_walkable_position(ChunkedWorld* world, int center_x, int center_y, int max_radius, int& out_x, int& out_y) {
        for (int radius = 0; radius < max_radius; ++radius) {
            for (int angle = 0; angle < 360; angle += 15) {
                int test_x = center_x + static_cast<int>(radius * std::cos(angle * 3.14159 / 180.0));
                int test_y = center_y + static_cast<int>(radius * std::sin(angle * 3.14159 / 180.0));
                
                if (is_walkable(world, test_x, test_y)) {
                    out_x = test_x;
                    out_y = test_y;
                    return true;
                }
            }
        }
        return false;
    }
    
    // Get structure type helper
    int get_structure_type(Chunk* chunk) const {
        return chunk->get_structure_type();
    }
    
    void get_structure_center(Chunk* chunk, int& cx, int& cy) const {
        chunk->get_structure_center(cx, cy);
    }
    
    // Render a planned structure onto a chunk (handles multi-chunk structures)
    void render_structure_to_chunk(Chunk* chunk, const PlannedStructure& structure, ChunkedWorld* /*world*/) {
        ChunkCoord coord = chunk->get_coord();
        
        // Calculate chunk bounds in world coordinates
        int chunk_min_x = coord.x * CHUNK_SIZE;
        int chunk_min_y = coord.y * CHUNK_SIZE;
        int chunk_max_x = chunk_min_x + CHUNK_SIZE - 1;
        int chunk_max_y = chunk_min_y + CHUNK_SIZE - 1;
        
        // Structure center in world coordinates
        int cx = structure.world_x;
        int cy = structure.world_y;
        
        // Create RNG with structure's seed for consistent generation
        std::mt19937 struct_rng(structure.structure_seed);
        
        StructureType type = static_cast<StructureType>(structure.type);
        
        // Calculate what portion of the structure overlaps this chunk
        int struct_min_x = structure.min_x();
        int struct_max_x = structure.max_x();
        int struct_min_y = structure.min_y();
        int struct_max_y = structure.max_y();
        
        // Clamp structure bounds to chunk bounds
        int render_min_x = std::max(struct_min_x, chunk_min_x);
        int render_max_x = std::min(struct_max_x, chunk_max_x);
        int render_min_y = std::max(struct_min_y, chunk_min_y);
        int render_max_y = std::min(struct_max_y, chunk_max_y);
        
        // Generate the structure using world coordinates
        switch (type) {
            case StructureType::VILLAGE:
                generate_village_world(chunk, cx, cy, struct_rng, render_min_x, render_max_x, render_min_y, render_max_y);
                break;
            // For all other structures, use JSON definitions via the generic handler
            default:
                generate_structure_world_generic(chunk, cx, cy, type, struct_rng, render_min_x, render_max_x, render_min_y, render_max_y);
                break;
        }
    }

private:
    // ==================== MULTI-CHUNK STRUCTURE GENERATORS (World Coordinates) ====================
    
    // Generic handler for structures - supports multi-chunk rendering via world coordinates
    void generate_structure_world_generic(Chunk* chunk, int cx, int cy, StructureType type, std::mt19937& rng,
                                          int render_min_x, int render_max_x, int render_min_y, int render_max_y) {
        ChunkCoord coord = chunk->get_coord();
        
        int type_id = static_cast<int>(type);
        
        // All structures should have JSON definitions - use world-coordinate rendering for multi-chunk support
        if (json_structures_loaded && structure_loader.has_json_definition(type_id)) {
            const StructureDefinition* def = structure_loader.get_structure_by_type(type_id);
            if (def && def->valid) {
                structure_loader.generate_structure_world(chunk, *def, cx, cy, rng,
                                                          render_min_x, render_max_x,
                                                          render_min_y, render_max_y, coord);
            }
        }
        // No fallback - all structures must be defined in JSON
    }
    
    // Multi-chunk village generation using WFC/Jigsaw system
    void generate_village_world(Chunk* chunk, int cx, int cy, std::mt19937& local_rng,
                                int render_min_x, int render_max_x, int render_min_y, int render_max_y) {
        // Village extends 52 tiles in each direction (104x104 total) to fit larger buildings
        int village_radius = 52;
        
        // Use jigsaw generator for WFC-based village generation (generates roads and plans buildings)
        ChunkCoord chunk_coord = chunk->get_coord();
        jigsaw_generator.generate_village_world(chunk, cx, cy, village_radius, local_rng,
                                                 render_min_x, render_max_x,
                                                 render_min_y, render_max_y, chunk_coord);
        
        // Now render actual JSON buildings at the planned positions
        const auto& planned_buildings = jigsaw_generator.get_buildings();
        for (const auto& planned : planned_buildings) {
            if (!planned.placed) continue;
            
            // Get the JSON structure definition
            const StructureDefinition* struct_def = structure_loader.get_structure(planned.structure_id);
            if (struct_def && struct_def->valid) {
                // Render the actual JSON structure at this position
                structure_loader.generate_structure_world(chunk, *struct_def, 
                                        planned.center_x, planned.center_y, local_rng,
                                        render_min_x, render_max_x,
                                        render_min_y, render_max_y, chunk_coord, 1);
                
                // Store house buildings for NPC spawning
                bool is_house = std::find(struct_def->tags.begin(), struct_def->tags.end(), "house") != struct_def->tags.end();
                if (is_house) {
                    PlacedBuildingInfo building_info(planned.structure_id, 
                                                     planned.center_x, planned.center_y,
                                                     planned.width, planned.height);
                    
                    // Parse bed info and villagers_per_house from the structure definition's JSON
                    // This info is stored in the JSON but we can default based on house type
                    if (planned.structure_id == "house_small") {
                        building_info.villagers_per_house = 1;
                        building_info.bed_positions.push_back({3, 2});
                    } else if (planned.structure_id == "house_large") {
                        building_info.villagers_per_house = 2;
                        building_info.bed_positions.push_back({3, 1});
                        building_info.bed_positions.push_back({10, 1});
                    }
                    
                    chunk->add_placed_building(building_info);
                }
            }
        }
    }
};
