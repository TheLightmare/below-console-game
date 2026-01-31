#pragma once
#include "chunk.hpp"
#include "tile_drawing_utils.hpp"
#include "village_config.hpp"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <random>
#include <optional>
#include <algorithm>
#include <climits>
#include <cmath>

// Import tile drawing utilities
using namespace TileDrawing;

// Forward declarations for structure loading
struct StructureDefinition;
struct TilePlacement;
struct EntitySpawn;

// Direction for road generation
enum class RoadDirection {
    NORTH = 0,
    EAST = 1,
    SOUTH = 2,
    WEST = 3
};

inline std::pair<int, int> road_direction_offset(RoadDirection dir) {
    switch (dir) {
        case RoadDirection::NORTH: return {0, -1};
        case RoadDirection::EAST:  return {1, 0};
        case RoadDirection::SOUTH: return {0, 1};
        case RoadDirection::WEST:  return {-1, 0};
    }
    return {0, 0};
}

inline RoadDirection opposite_road_dir(RoadDirection dir) {
    return static_cast<RoadDirection>((static_cast<int>(dir) + 2) % 4);
}

// Village district types for themed building placement
enum class VillageDistrict {
    CENTER,      // Town square, well, fountain
    MARKET,      // Shops, stalls, tavern
    RESIDENTIAL, // Houses, gardens
    INDUSTRIAL,  // Blacksmith, stable, bakery
    RELIGIOUS,   // Church, graveyard
    ENTRANCE     // Gates, guardhouse
};

// A road segment in the village network
struct RoadSegment {
    int x1, y1, x2, y2;  // World coordinates
    int width = 2;
    bool is_main = false;  // Main roads are wider
    bool is_path = false;  // Dirt paths are narrower
    
    bool contains_point(int x, int y, int padding = 0) const {
        int min_x = std::min(x1, x2) - padding;
        int max_x = std::max(x1, x2) + width + padding;
        int min_y = std::min(y1, y2) - padding;
        int max_y = std::max(y1, y2) + width + padding;
        return x >= min_x && x <= max_x && y >= min_y && y <= max_y;
    }
    
    int length() const {
        return std::abs(x2 - x1) + std::abs(y2 - y1);
    }
    
    bool is_horizontal() const {
        return std::abs(x2 - x1) > std::abs(y2 - y1);
    }
};

// A placed building in the village
struct VillagePlacement {
    std::string structure_id;
    int center_x, center_y;  // World coordinates
    int width, height;       // Size from JSON definition
    int door_x, door_y;      // Door position for path connection
    bool placed = false;
    std::vector<std::string> tags;
    VillageDistrict district = VillageDistrict::RESIDENTIAL;
    
    int left() const { return center_x - width / 2; }
    int right() const { return center_x + (width - width / 2); }
    int top() const { return center_y - height / 2; }
    int bottom() const { return center_y + (height - height / 2); }
    
    bool intersects(const VillagePlacement& other, int padding = 3) const {
        return left() - padding < other.right() + padding &&
               right() + padding > other.left() - padding &&
               top() - padding < other.bottom() + padding &&
               bottom() + padding > other.top() - padding;
    }
    
    bool intersects_road(const RoadSegment& road, int padding = 2) const {
        int road_left = std::min(road.x1, road.x2) - padding;
        int road_right = std::max(road.x1, road.x2) + road.width + padding;
        int road_top = std::min(road.y1, road.y2) - padding;
        int road_bottom = std::max(road.y1, road.y2) + road.width + padding;
        
        return left() < road_right && right() > road_left &&
               top() < road_bottom && bottom() > road_top;
    }
};

// A placed piece for backward compatibility
struct PlacedPiece {
    std::string structure_id;
    int grid_x = 0;
    int grid_y = 0;
    int world_x = 0;
    int world_y = 0;
    int width = 0;
    int height = 0;
    std::set<std::string> tags;
};

// Building info for placement calculations
struct VillageBuildingInfo {
    std::string id;
    int width;
    int height;
    VillageDistrict preferred_district;
    float spawn_weight;  // Relative chance to spawn (higher = more common)
    bool unique;         // Only one per village?
    
    // Constructor for compatibility with old static initialization
    VillageBuildingInfo(const char* id_, int w, int h, VillageDistrict dist, float weight, bool uniq)
        : id(id_), width(w), height(h), preferred_district(dist), spawn_weight(weight), unique(uniq) {}
    
    // Default constructor
    VillageBuildingInfo() : width(0), height(0), preferred_district(VillageDistrict::RESIDENTIAL), 
                            spawn_weight(1.0f), unique(false) {}
};

// The main village generator using organic road networks and district-based building placement
class JigsawGenerator {
private:
    std::vector<RoadSegment> roads;
    std::vector<VillagePlacement> buildings;
    std::vector<std::pair<int, int>> path_nodes;  // Junction points for organic paths
    
    // Data-driven building catalog (populated from loaded structures)
    std::vector<VillageBuildingInfo> building_catalog;
    bool catalog_initialized = false;
    
    // Active village configuration (set per-generation)
    bool has_village_config = false;
    
    // Cached config values for current generation
    struct ActiveConfig {
        // Road settings
        int main_roads_min = 3;
        int main_roads_max = 4;
        int road_segments_min = 3;
        int road_segments_max = 4;
        float ring_road_chance = 0.7f;
        std::vector<float> ring_distances = {0.4f, 0.7f};
        float path_connection_chance = 0.25f;
        
        // Road style
        char main_road_symbol = '#';
        std::string main_road_color = "gray";
        int main_road_width = 3;
        char side_road_symbol = ':';
        std::string side_road_color = "gray";
        int side_road_width = 2;
        char path_symbol = '.';
        std::string path_color = "brown";
        int path_width = 1;
        
        // Ground
        std::vector<std::pair<char, std::string>> ground_tiles;  // symbol, color
        std::vector<float> ground_chances;
        
        // Building placement
        int center_clear_radius = 10;
        int building_spacing = 4;
        int edge_margin = 3;
        int max_placement_attempts = 100;
        
        // District layout
        float center_zone_radius_pct = 0.2f;
        float entrance_angle_start = 70.0f;
        float entrance_angle_end = 110.0f;
        float entrance_inner_pct = 0.7f;
        
        // Building quotas (district -> base_count, per_radius)
        std::map<VillageDistrict, std::pair<int, int>> building_quotas;
        
        // Perimeter
        bool has_perimeter = true;
        char perimeter_fence_symbol = '#';
        std::string perimeter_fence_color = "brown";
        float perimeter_tree_chance = 0.15f;
        char perimeter_tree_symbol = 'T';
        std::string perimeter_tree_color = "green";
        
        // Decorations
        float flower_chance = 0.02f;
        char flower_symbol = '*';
        std::string flower_color = "yellow";
        float dirt_patch_chance = 0.08f;
        
        // Building filters
        std::vector<std::string> allowed_buildings;
        std::vector<std::string> required_buildings;
        
        void reset_to_defaults() {
            main_roads_min = 3;
            main_roads_max = 4;
            road_segments_min = 3;
            road_segments_max = 4;
            ring_road_chance = 0.7f;
            ring_distances = {0.4f, 0.7f};
            path_connection_chance = 0.25f;
            
            main_road_symbol = '#';
            main_road_color = "gray";
            main_road_width = 3;
            side_road_symbol = ':';
            side_road_color = "gray";
            side_road_width = 2;
            path_symbol = '.';
            path_color = "brown";
            path_width = 1;
            
            ground_tiles.clear();
            ground_chances.clear();
            
            center_clear_radius = 10;
            building_spacing = 4;
            edge_margin = 3;
            max_placement_attempts = 100;
            
            center_zone_radius_pct = 0.2f;
            entrance_angle_start = 70.0f;
            entrance_angle_end = 110.0f;
            entrance_inner_pct = 0.7f;
            
            building_quotas.clear();
            building_quotas[VillageDistrict::RESIDENTIAL] = {5, 15};
            building_quotas[VillageDistrict::MARKET] = {3, 25};
            building_quotas[VillageDistrict::INDUSTRIAL] = {2, 30};
            
            has_perimeter = true;
            perimeter_fence_symbol = '#';
            perimeter_fence_color = "brown";
            perimeter_tree_chance = 0.15f;
            perimeter_tree_symbol = 'T';
            perimeter_tree_color = "green";
            
            flower_chance = 0.02f;
            flower_symbol = '*';
            flower_color = "yellow";
            dirt_patch_chance = 0.08f;
            
            allowed_buildings.clear();
            required_buildings.clear();
        }
    } active_config;
    
    // Fallback static catalog for when no structures are loaded
    static const std::vector<VillageBuildingInfo>& get_default_building_catalog() {
        static const std::vector<VillageBuildingInfo> catalog = {
            // Residential buildings (common)
            {"house_small", 20, 13, VillageDistrict::RESIDENTIAL, 3.0f, false},
            {"house_large", 15, 11, VillageDistrict::RESIDENTIAL, 1.5f, false},
            
            // Commercial buildings (market district)
            {"shop", 26, 16, VillageDistrict::MARKET, 1.0f, false},
            {"inn", 46, 20, VillageDistrict::MARKET, 0.8f, true},
            {"tavern", 32, 14, VillageDistrict::MARKET, 0.6f, true},
            {"bakery", 24, 11, VillageDistrict::MARKET, 0.7f, true},
            {"market_stall", 5, 4, VillageDistrict::MARKET, 2.0f, false},
            
            // Industrial buildings
            {"blacksmith", 32, 17, VillageDistrict::INDUSTRIAL, 0.9f, true},
            {"stable", 26, 11, VillageDistrict::INDUSTRIAL, 0.7f, true},
            
            // Religious/civic buildings
            {"church", 14, 14, VillageDistrict::RELIGIOUS, 0.5f, true},
            
            // Authority buildings
            {"guardhouse", 16, 8, VillageDistrict::ENTRANCE, 0.8f, false},
            
            // Decorative structures
            {"fountain", 9, 7, VillageDistrict::CENTER, 1.0f, true},
            {"well", 7, 6, VillageDistrict::CENTER, 1.0f, true},
            
            // Farm elements (edge of village)
            {"farm_plot", 9, 8, VillageDistrict::INDUSTRIAL, 1.5f, false},
        };
        return catalog;
    }
    
    // Get the active building catalog (data-driven or fallback)
    const std::vector<VillageBuildingInfo>& get_building_catalog() const {
        if (catalog_initialized && !building_catalog.empty()) {
            return building_catalog;
        }
        return get_default_building_catalog();
    }
    
    // Structure to define district zones
    struct DistrictZone {
        VillageDistrict type;
        float angle_start;  // Radial sector start (in radians)
        float angle_end;    // Radial sector end
        float inner_radius; // How close to center this district starts
        float outer_radius; // How far from center this district extends
    };
    
public:
    JigsawGenerator() = default;
    
    // Set building catalog from loaded structure definitions
    // This enables data-driven village generation
    template<typename StructureMap>
    void set_building_catalog_from_structures(const StructureMap& structures) {
        building_catalog.clear();
        
        for (const auto& [id, def] : structures) {
            // Only include structures with jigsaw enabled
            if (!def.jigsaw.enabled) continue;
            
            VillageBuildingInfo info;
            info.id = def.id;
            info.width = def.get_width();
            info.height = def.get_height();
            info.spawn_weight = def.jigsaw.spawn_weight;
            info.unique = def.jigsaw.unique;
            
            // Convert JigsawDistrict to VillageDistrict
            switch (static_cast<int>(def.jigsaw.district)) {
                case 0: info.preferred_district = VillageDistrict::CENTER; break;
                case 1: info.preferred_district = VillageDistrict::MARKET; break;
                case 2: info.preferred_district = VillageDistrict::RESIDENTIAL; break;
                case 3: info.preferred_district = VillageDistrict::INDUSTRIAL; break;
                case 4: info.preferred_district = VillageDistrict::RELIGIOUS; break;
                case 5: info.preferred_district = VillageDistrict::ENTRANCE; break;
                default: info.preferred_district = VillageDistrict::RESIDENTIAL; break;
            }
            
            building_catalog.push_back(info);
        }
        
        catalog_initialized = true;
    }
    
    // Clear the data-driven catalog (reverts to default)
    void clear_building_catalog() {
        building_catalog.clear();
        catalog_initialized = false;
    }
    
    // Check if using data-driven catalog
    bool is_catalog_data_driven() const {
        return catalog_initialized && !building_catalog.empty();
    }
    
    // Get number of buildings in catalog
    size_t get_catalog_size() const {
        return get_building_catalog().size();
    }
    
    // Set village configuration from a JigsawVillageConfig
    // This must be called before generate_village_world for data-driven generation
    void set_village_config(const JigsawVillageConfig& config) {
        has_village_config = true;
        
        // Road settings
        active_config.main_roads_min = config.main_roads_min;
        active_config.main_roads_max = config.main_roads_max;
        active_config.road_segments_min = config.road_segments_min;
        active_config.road_segments_max = config.road_segments_max;
        active_config.ring_road_chance = config.ring_road_chance;
        active_config.ring_distances = config.ring_distances;
        active_config.path_connection_chance = config.path_connection_chance;
        
        // Road style
        active_config.main_road_symbol = config.roads.main_road_symbol;
        active_config.main_road_color = config.roads.main_road_color;
        active_config.main_road_width = config.roads.main_road_width;
        active_config.side_road_symbol = config.roads.side_road_symbol;
        active_config.side_road_color = config.roads.side_road_color;
        active_config.side_road_width = config.roads.side_road_width;
        active_config.path_symbol = config.roads.path_symbol;
        active_config.path_color = config.roads.path_color;
        active_config.path_width = config.roads.path_width;
        
        // Ground tiles
        active_config.ground_tiles.clear();
        active_config.ground_chances.clear();
        for (const auto& gt : config.ground_tiles) {
            active_config.ground_tiles.push_back({gt.symbol, gt.color});
            active_config.ground_chances.push_back(gt.chance);
        }
        
        // Building placement
        active_config.center_clear_radius = config.center_clear_radius;
        active_config.building_spacing = config.building_spacing;
        active_config.edge_margin = config.edge_margin;
        active_config.max_placement_attempts = config.max_placement_attempts;
        
        // District layout
        active_config.center_zone_radius_pct = config.center_zone_radius_pct;
        active_config.entrance_angle_start = config.entrance_angle_start;
        active_config.entrance_angle_end = config.entrance_angle_end;
        active_config.entrance_inner_pct = config.entrance_inner_pct;
        
        // Building quotas
        active_config.building_quotas.clear();
        for (const auto& bq : config.building_quotas) {
            VillageDistrict dist;
            switch (static_cast<int>(bq.district)) {
                case 0: dist = VillageDistrict::CENTER; break;
                case 1: dist = VillageDistrict::MARKET; break;
                case 2: dist = VillageDistrict::RESIDENTIAL; break;
                case 3: dist = VillageDistrict::INDUSTRIAL; break;
                case 4: dist = VillageDistrict::RELIGIOUS; break;
                case 5: dist = VillageDistrict::ENTRANCE; break;
                default: dist = VillageDistrict::RESIDENTIAL; break;
            }
            active_config.building_quotas[dist] = {bq.base_count, bq.per_radius};
        }
        
        // Perimeter
        active_config.has_perimeter = config.has_perimeter;
        active_config.perimeter_fence_symbol = config.perimeter_fence_symbol;
        active_config.perimeter_fence_color = config.perimeter_fence_color;
        active_config.perimeter_tree_chance = config.perimeter_tree_chance;
        active_config.perimeter_tree_symbol = config.perimeter_tree_symbol;
        active_config.perimeter_tree_color = config.perimeter_tree_color;
        
        // Decorations
        active_config.flower_chance = config.flower_chance;
        active_config.flower_symbol = config.flower_symbol;
        active_config.flower_color = config.flower_color;
        active_config.dirt_patch_chance = config.dirt_patch_chance;
        
        // Building filters
        active_config.allowed_buildings = config.allowed_buildings;
        active_config.required_buildings = config.required_buildings;
    }
    
    // Reset to default configuration
    void reset_village_config() {
        has_village_config = false;
        active_config.reset_to_defaults();
    }
    
    // Check if custom config is active
    bool has_custom_config() const {
        return has_village_config;
    }
    
    void create_default_pieces() {
        // New system doesn't use pre-created pieces
    }
    
    std::vector<PlacedPiece> generate_village(Chunk* chunk, int center_x, int center_y, 
                                               int village_radius, std::mt19937& rng) {
        ChunkCoord dummy_coord{0, 0};
        return generate_village_world(chunk, center_x, center_y, village_radius, rng,
                                      0, CHUNK_SIZE - 1, 0, CHUNK_SIZE - 1, dummy_coord);
    }
    
    std::vector<PlacedPiece> generate_village_world(Chunk* chunk, int center_x, int center_y, 
                                                     int village_radius, std::mt19937& rng,
                                                     int render_min_x, int render_max_x,
                                                     int render_min_y, int render_max_y,
                                                     ChunkCoord chunk_coord) {
        roads.clear();
        buildings.clear();
        path_nodes.clear();
        
        // Step 1: Generate organic road network with main square
        generate_organic_road_network(center_x, center_y, village_radius, rng);
        
        // Step 2: Generate district zones around the village
        auto districts = generate_district_zones(center_x, center_y, village_radius, rng);
        
        // Step 3: Place buildings according to districts
        generate_district_buildings(center_x, center_y, village_radius, districts, rng);
        
        // Step 4: Render the village ground layer
        render_village_ground(chunk, center_x, center_y, village_radius,
                             render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord, rng);
        
        // Step 5: Render roads and paths
        render_roads_to_chunk(chunk, render_min_x, render_max_x, 
                             render_min_y, render_max_y, chunk_coord);
        
        // Step 6: Render building placeholders
        render_building_placeholders(chunk, render_min_x, render_max_x,
                                    render_min_y, render_max_y, chunk_coord);
        
        // Step 7: Add decorations
        render_decorations(chunk, center_x, center_y, village_radius, render_min_x, render_max_x,
                          render_min_y, render_max_y, chunk_coord, rng);
        
        // Step 8: Add village perimeter
        render_village_perimeter(chunk, center_x, center_y, village_radius,
                                render_min_x, render_max_x, render_min_y, render_max_y, 
                                chunk_coord, rng);
        
        // Convert to PlacedPiece format for compatibility
        std::vector<PlacedPiece> result;
        for (const auto& bld : buildings) {
            if (bld.placed) {
                PlacedPiece p;
                p.structure_id = bld.structure_id;
                p.world_x = bld.left();
                p.world_y = bld.top();
                p.width = bld.width;
                p.height = bld.height;
                p.tags.insert(bld.tags.begin(), bld.tags.end());
                p.tags.insert("building");
                result.push_back(p);
            }
        }
        
        return result;
    }
    
    const std::vector<VillagePlacement>& get_buildings() const { return buildings; }
    const std::vector<RoadSegment>& get_roads() const { return roads; }
    
private:
    // Generate district zones for the village (pie-slice layout with randomization)
    std::vector<DistrictZone> generate_district_zones(int center_x, int center_y, 
                                                       int radius, std::mt19937& rng) {
        (void)center_x; (void)center_y;
        
        std::vector<DistrictZone> zones;
        std::uniform_real_distribution<float> angle_jitter(-0.2f, 0.2f);
        
        float r = static_cast<float>(radius);
        
        // Center zone (town square)
        zones.push_back({VillageDistrict::CENTER, 0.0f, 6.28318f, 0.0f, r * 0.2f});
        
        // Divide the outer ring into sectors
        std::vector<VillageDistrict> sector_types = {
            VillageDistrict::MARKET,
            VillageDistrict::RESIDENTIAL,
            VillageDistrict::INDUSTRIAL,
            VillageDistrict::RESIDENTIAL,
            VillageDistrict::RELIGIOUS,
            VillageDistrict::RESIDENTIAL
        };
        
        std::shuffle(sector_types.begin(), sector_types.end(), rng);
        
        // Always put entrance at bottom (south)
        zones.push_back({VillageDistrict::ENTRANCE, 1.2f, 1.9f, r * 0.7f, r * 1.0f});
        
        // Create the other sectors
        float sector_size = 6.28318f / static_cast<float>(sector_types.size());
        for (size_t i = 0; i < sector_types.size(); ++i) {
            float start_angle = static_cast<float>(i) * sector_size + angle_jitter(rng);
            float end_angle = static_cast<float>(i + 1) * sector_size + angle_jitter(rng);
            
            zones.push_back({
                sector_types[i],
                start_angle,
                end_angle,
                r * 0.25f,
                r * 0.85f
            });
        }
        
        return zones;
    }
    
    // Get the district type for a given position
    VillageDistrict get_district_at(int x, int y, int center_x, int center_y,
                                    const std::vector<DistrictZone>& zones) {
        float dx = static_cast<float>(x - center_x);
        float dy = static_cast<float>(y - center_y);
        float dist = std::sqrt(dx * dx + dy * dy);
        float angle = std::atan2(dy, dx);
        if (angle < 0) angle += 6.28318f;
        
        for (const auto& zone : zones) {
            if (dist >= zone.inner_radius && dist <= zone.outer_radius) {
                float start = zone.angle_start;
                float end = zone.angle_end;
                
                if (start <= end) {
                    if (angle >= start && angle <= end) return zone.type;
                } else {
                    if (angle >= start || angle <= end) return zone.type;
                }
            }
        }
        
        return VillageDistrict::RESIDENTIAL;
    }
    
    // Generate an organic road network with curves and natural flow
    void generate_organic_road_network(int center_x, int center_y, int radius, std::mt19937& rng) {
        std::uniform_real_distribution<float> chance(0.0f, 1.0f);
        std::uniform_int_distribution<int> jitter(-2, 2);
        
        // Use config values or defaults
        const int MAIN_ROAD_WIDTH = has_village_config ? active_config.main_road_width : 3;
        const int SIDE_ROAD_WIDTH = has_village_config ? active_config.side_road_width : 2;
        const int PATH_WIDTH = has_village_config ? active_config.path_width : 1;
        
        int main_roads_min = has_village_config ? active_config.main_roads_min : 3;
        int main_roads_max = has_village_config ? active_config.main_roads_max : 4;
        int segments_min = has_village_config ? active_config.road_segments_min : 3;
        int segments_max = has_village_config ? active_config.road_segments_max : 4;
        float ring_chance = has_village_config ? active_config.ring_road_chance : 0.7f;
        float path_chance = has_village_config ? active_config.path_connection_chance : 0.25f;
        const std::vector<float>& ring_dists = has_village_config && !active_config.ring_distances.empty() 
            ? active_config.ring_distances : std::vector<float>{0.4f, 0.7f};
        
        path_nodes.push_back({center_x, center_y});
        
        // Main roads radiate outward from center (slightly curved)
        int num_main_roads = main_roads_min + static_cast<int>(rng() % (main_roads_max - main_roads_min + 1));
        float angle_offset = chance(rng) * 0.5f;
        
        for (int i = 0; i < num_main_roads; ++i) {
            float angle = (static_cast<float>(i) / num_main_roads) * 6.28318f + angle_offset;
            
            int prev_x = center_x;
            int prev_y = center_y;
            int segments = segments_min + static_cast<int>(rng() % (segments_max - segments_min + 1));
            float segment_len = static_cast<float>(radius - 8) / segments;
            
            for (int seg = 1; seg <= segments; ++seg) {
                float seg_angle = angle + (chance(rng) - 0.5f) * 0.3f;
                int next_x = center_x + static_cast<int>(seg * segment_len * std::cos(seg_angle));
                int next_y = center_y + static_cast<int>(seg * segment_len * std::sin(seg_angle));
                
                next_x += jitter(rng);
                next_y += jitter(rng);
                
                roads.push_back({prev_x, prev_y, next_x, next_y, MAIN_ROAD_WIDTH, true, false});
                path_nodes.push_back({next_x, next_y});
                
                prev_x = next_x;
                prev_y = next_y;
            }
        }
        
        // Add connecting ring roads at different distances from center
        for (float ring_dist : ring_dists) {
            if (chance(rng) < ring_chance) {
                int ring_radius = static_cast<int>(radius * ring_dist);
                int prev_rx = -1, prev_ry = -1;
                int first_rx = -1, first_ry = -1;
                
                int ring_segments = 6 + (rng() % 4);
                for (int seg = 0; seg <= ring_segments; ++seg) {
                    float seg_angle = (static_cast<float>(seg) / ring_segments) * 6.28318f;
                    int rx = center_x + static_cast<int>(ring_radius * std::cos(seg_angle)) + jitter(rng);
                    int ry = center_y + static_cast<int>(ring_radius * std::sin(seg_angle)) + jitter(rng);
                    
                    if (prev_rx >= 0) {
                        int seg_len = std::abs(rx - prev_rx) + std::abs(ry - prev_ry);
                        if (seg_len < radius / 2) {
                            roads.push_back({prev_rx, prev_ry, rx, ry, SIDE_ROAD_WIDTH, false, false});
                        }
                    } else {
                        first_rx = rx;
                        first_ry = ry;
                    }
                    
                    path_nodes.push_back({rx, ry});
                    prev_rx = rx;
                    prev_ry = ry;
                }
                
                if (first_rx >= 0 && prev_rx >= 0) {
                    roads.push_back({prev_rx, prev_ry, first_rx, first_ry, SIDE_ROAD_WIDTH, false, false});
                }
            }
        }
        
        // Add some random connecting paths between nearby nodes
        size_t node_count = path_nodes.size();
        for (size_t i = 0; i < node_count; ++i) {
            for (size_t j = i + 1; j < node_count; ++j) {
                int dx = path_nodes[j].first - path_nodes[i].first;
                int dy = path_nodes[j].second - path_nodes[i].second;
                int dist = std::abs(dx) + std::abs(dy);
                
                if (dist > 10 && dist < 30 && chance(rng) < path_chance) {
                    bool already_connected = false;
                    for (const auto& road : roads) {
                        if ((road.x1 == path_nodes[i].first && road.y1 == path_nodes[i].second &&
                             road.x2 == path_nodes[j].first && road.y2 == path_nodes[j].second) ||
                            (road.x2 == path_nodes[i].first && road.y2 == path_nodes[i].second &&
                             road.x1 == path_nodes[j].first && road.y1 == path_nodes[j].second)) {
                            already_connected = true;
                            break;
                        }
                    }
                    
                    if (!already_connected) {
                        roads.push_back({
                            path_nodes[i].first, path_nodes[i].second,
                            path_nodes[j].first, path_nodes[j].second,
                            PATH_WIDTH, false, true
                        });
                    }
                }
            }
        }
    }
    
    // Generate buildings in appropriate districts
    void generate_district_buildings(int center_x, int center_y, int radius,
                                     const std::vector<DistrictZone>& zones, std::mt19937& rng) {
        std::uniform_real_distribution<float> chance(0.0f, 1.0f);
        const auto& catalog = get_building_catalog();
        
        std::set<std::string> placed_unique;
        
        // Get building quotas from config or use defaults
        auto get_quota = [this, radius](VillageDistrict dist, int default_base, int default_per) -> int {
            if (has_village_config) {
                auto it = active_config.building_quotas.find(dist);
                if (it != active_config.building_quotas.end()) {
                    return it->second.first + (radius / it->second.second);
                }
            }
            return default_base + (radius / default_per);
        };
        
        int target_residential = get_quota(VillageDistrict::RESIDENTIAL, 5, 15);
        int target_commercial = get_quota(VillageDistrict::MARKET, 3, 25);
        int target_industrial = get_quota(VillageDistrict::INDUSTRIAL, 2, 30);
        
        const int CENTER_CLEAR_RADIUS = has_village_config ? active_config.center_clear_radius : 10;
        const int BUILDING_SPACING = has_village_config ? active_config.building_spacing : 4;
        const int MAX_ATTEMPTS = has_village_config ? active_config.max_placement_attempts : 100;
        
        // First pass: Place key unique buildings
        std::vector<const VillageBuildingInfo*> unique_buildings;
        for (const auto& info : catalog) {
            // Check if building is allowed
            if (has_village_config && !active_config.allowed_buildings.empty()) {
                bool allowed = false;
                for (const auto& allowed_id : active_config.allowed_buildings) {
                    if (info.id == allowed_id) { allowed = true; break; }
                }
                if (!allowed) continue;
            }
            
            if (info.unique) {
                unique_buildings.push_back(&info);
            }
        }
        
        std::shuffle(unique_buildings.begin(), unique_buildings.end(), rng);
        
        for (const auto* info : unique_buildings) {
            if (chance(rng) > info->spawn_weight * 0.8f) continue;
            
            bool found = try_place_building(*info, center_x, center_y, radius,
                                            zones, CENTER_CLEAR_RADIUS, BUILDING_SPACING, rng);
            if (found) {
                placed_unique.insert(info->id);
            }
        }
        
        // Second pass: Fill with common buildings
        int attempts = 0;
        
        while (attempts < MAX_ATTEMPTS) {
            ++attempts;
            
            int residential = 0, commercial = 0, industrial = 0;
            for (const auto& bld : buildings) {
                switch (bld.district) {
                    case VillageDistrict::RESIDENTIAL: ++residential; break;
                    case VillageDistrict::MARKET: ++commercial; break;
                    case VillageDistrict::INDUSTRIAL: ++industrial; break;
                    default: break;
                }
            }
            
            if (residential >= target_residential && 
                commercial >= target_commercial && 
                industrial >= target_industrial) {
                break;
            }
            
            // Build filtered catalog based on allowed_buildings
            float total_weight = 0.0f;
            for (const auto& info : catalog) {
                if (info.unique) continue;
                
                // Check if building is allowed
                if (has_village_config && !active_config.allowed_buildings.empty()) {
                    bool allowed = false;
                    for (const auto& allowed_id : active_config.allowed_buildings) {
                        if (info.id == allowed_id) { allowed = true; break; }
                    }
                    if (!allowed) continue;
                }
                
                total_weight += info.spawn_weight;
            }
            
            float roll = chance(rng) * total_weight;
            float cumulative = 0.0f;
            const VillageBuildingInfo* chosen = nullptr;
            
            for (const auto& info : catalog) {
                if (info.unique) continue;
                
                // Check if building is allowed
                if (has_village_config && !active_config.allowed_buildings.empty()) {
                    bool allowed = false;
                    for (const auto& allowed_id : active_config.allowed_buildings) {
                        if (info.id == allowed_id) { allowed = true; break; }
                    }
                    if (!allowed) continue;
                }
                
                cumulative += info.spawn_weight;
                if (roll <= cumulative) {
                    chosen = &info;
                    break;
                }
            }
            
            if (chosen) {
                try_place_building(*chosen, center_x, center_y, radius,
                                   zones, CENTER_CLEAR_RADIUS, BUILDING_SPACING, rng);
            }
        }
        
        // Third pass: Add market stalls in market district
        int num_stalls = 2 + (rng() % 4);
        for (int i = 0; i < num_stalls; ++i) {
            const VillageBuildingInfo stall_info = {"market_stall", 5, 4, VillageDistrict::MARKET, 1.0f, false};
            try_place_building(stall_info, center_x, center_y, radius,
                               zones, 12, 2, rng);
        }
        
        // Fourth pass: Add farm plots at edges
        int num_farms = 1 + (rng() % 3);
        for (int i = 0; i < num_farms; ++i) {
            const VillageBuildingInfo farm_info = {"farm_plot", 9, 8, VillageDistrict::INDUSTRIAL, 1.0f, false};
            for (int attempt = 0; attempt < 15; ++attempt) {
                float angle = chance(rng) * 6.28318f;
                float dist = radius * (0.75f + chance(rng) * 0.2f);
                int px = center_x + static_cast<int>(dist * std::cos(angle));
                int py = center_y + static_cast<int>(dist * std::sin(angle));
                
                if (can_place_building_at(px, py, farm_info.width, farm_info.height, 2)) {
                    add_building(farm_info, px, py, VillageDistrict::INDUSTRIAL);
                    break;
                }
            }
        }
    }
    
    bool try_place_building(const VillageBuildingInfo& info, int center_x, int center_y,
                            int radius, const std::vector<DistrictZone>& zones,
                            int center_clear, int spacing, std::mt19937& rng) {
        std::uniform_real_distribution<float> chance(0.0f, 1.0f);
        
        const int MAX_ATTEMPTS = 25;
        
        for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
            float angle = chance(rng) * 6.28318f;
            float min_dist = static_cast<float>(center_clear + info.height / 2);
            float max_dist = static_cast<float>(radius - info.height / 2 - 3);
            float dist = min_dist + chance(rng) * (max_dist - min_dist);
            
            int px = center_x + static_cast<int>(dist * std::cos(angle));
            int py = center_y + static_cast<int>(dist * std::sin(angle));
            
            VillageDistrict pos_district = get_district_at(px, py, center_x, center_y, zones);
            
            bool district_ok = (pos_district == info.preferred_district);
            if (!district_ok && info.preferred_district == VillageDistrict::RESIDENTIAL) {
                district_ok = (pos_district != VillageDistrict::CENTER);
            }
            if (!district_ok && chance(rng) < 0.2f) {
                district_ok = (pos_district != VillageDistrict::CENTER);
            }
            
            if (!district_ok) continue;
            
            int dx = px - center_x;
            int dy = py - center_y;
            if (dx * dx + dy * dy < center_clear * center_clear) continue;
            
            if (px - info.width / 2 < center_x - radius + 3 ||
                px + info.width / 2 > center_x + radius - 3 ||
                py - info.height / 2 < center_y - radius + 3 ||
                py + info.height / 2 > center_y + radius - 3) {
                continue;
            }
            
            if (!can_place_building_at(px, py, info.width, info.height, spacing)) {
                continue;
            }
            
            add_building(info, px, py, pos_district);
            return true;
        }
        
        return false;
    }
    
    bool can_place_building_at(int px, int py, int width, int height, int spacing) {
        VillagePlacement test;
        test.center_x = px;
        test.center_y = py;
        test.width = width;
        test.height = height;
        
        for (const auto& existing : buildings) {
            if (test.intersects(existing, spacing)) {
                return false;
            }
        }
        
        for (const auto& road : roads) {
            if (test.intersects_road(road, 2)) {
                return false;
            }
        }
        
        return true;
    }
    
    void add_building(const VillageBuildingInfo& info, int px, int py, VillageDistrict district) {
        VillagePlacement placement;
        placement.structure_id = info.id;
        placement.center_x = px;
        placement.center_y = py;
        placement.width = info.width;
        placement.height = info.height;
        placement.door_x = px;
        placement.door_y = py + info.height / 2;
        placement.tags = {"building"};
        placement.district = district;
        placement.placed = true;
        buildings.push_back(placement);
    }
    
    void render_village_ground(Chunk* chunk, int center_x, int center_y, int radius,
                               int render_min_x, int render_max_x,
                               int render_min_y, int render_max_y,
                               ChunkCoord chunk_coord, std::mt19937& rng) {
        std::uniform_real_distribution<float> chance(0.0f, 1.0f);
        
        // Use config ground tiles or defaults
        float flower_chance_val = has_village_config ? active_config.flower_chance : 0.02f;
        float dirt_chance_val = has_village_config ? active_config.dirt_patch_chance : 0.08f;
        
        Tile grass(true, ',', "green");
        Tile dirt(true, '.', "brown");
        Tile flowers(true, 
            has_village_config ? active_config.flower_symbol : '*',
            has_village_config ? active_config.flower_color : "yellow");
        
        // If custom ground tiles are defined, use them
        bool use_custom_ground = has_village_config && !active_config.ground_tiles.empty();
        
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                int dist_sq = dx * dx + dy * dy;
                if (dist_sq <= radius * radius) {
                    int wx = center_x + dx;
                    int wy = center_y + dy;
                    
                    Tile tile = grass;
                    
                    if (use_custom_ground) {
                        // Pick from custom ground tiles based on chance
                        float roll = chance(rng);
                        float cumulative = 0.0f;
                        for (size_t i = 0; i < active_config.ground_tiles.size(); ++i) {
                            cumulative += active_config.ground_chances[i];
                            if (roll < cumulative) {
                                tile = Tile(true, active_config.ground_tiles[i].first, 
                                           active_config.ground_tiles[i].second);
                                break;
                            }
                        }
                    } else {
                        if (chance(rng) < flower_chance_val) {
                            tile = flowers;
                        } else if (chance(rng) < dirt_chance_val) {
                            tile = dirt;
                        }
                    }
                    
                    set_tile_world(chunk, wx, wy, tile,
                                  render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
                }
            }
        }
    }
    
    void render_roads_to_chunk(Chunk* chunk, int render_min_x, int render_max_x,
                               int render_min_y, int render_max_y, ChunkCoord chunk_coord) {
        // Get road styles from config
        char main_sym = has_village_config ? active_config.main_road_symbol : '#';
        std::string main_color = has_village_config ? active_config.main_road_color : "gray";
        char side_sym = has_village_config ? active_config.side_road_symbol : ':';
        std::string side_color = has_village_config ? active_config.side_road_color : "gray";
        char path_sym = has_village_config ? active_config.path_symbol : '.';
        std::string path_color = has_village_config ? active_config.path_color : "brown";
        
        // Render all road surfaces with varied patterns
        for (const auto& road : roads) {
            Tile road_tile = road.is_path ? Tile(true, path_sym, path_color) : 
                            (road.is_main ? Tile(true, main_sym, main_color) : Tile(true, side_sym, side_color));
            
            // Draw line from point 1 to point 2 with width
            int dx = (road.x2 > road.x1) ? 1 : ((road.x2 < road.x1) ? -1 : 0);
            int dy = (road.y2 > road.y1) ? 1 : ((road.y2 < road.y1) ? -1 : 0);
            
            int x = road.x1, y = road.y1;
            while (true) {
                for (int w = 0; w < road.width; ++w) {
                    int wx = x + (dy != 0 ? w : 0);
                    int wy = y + (dx != 0 ? w : 0);
                    set_tile_world(chunk, wx, wy, road_tile,
                                  render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
                }
                
                if (x == road.x2 && y == road.y2) break;
                
                // Move toward destination
                if (x != road.x2) x += dx;
                if (y != road.y2) y += dy;
            }
        }
        
        // Render road edges for main/side roads
        Tile road_edge(true, '.', "dark_gray");
        for (const auto& road : roads) {
            if (road.is_path) continue;  // No edges for dirt paths
            
            int x = road.x1, y = road.y1;
            int dx = (road.x2 > road.x1) ? 1 : ((road.x2 < road.x1) ? -1 : 0);
            int dy = (road.y2 > road.y1) ? 1 : ((road.y2 < road.y1) ? -1 : 0);
            
            while (true) {
                // Add edges perpendicular to road direction
                if (dy != 0) {  // Vertical movement, add horizontal edges
                    if (!is_on_road(x - 1, y)) {
                        set_tile_world(chunk, x - 1, y, road_edge,
                                      render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
                    }
                    if (!is_on_road(x + road.width, y)) {
                        set_tile_world(chunk, x + road.width, y, road_edge,
                                      render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
                    }
                }
                if (dx != 0) {  // Horizontal movement, add vertical edges
                    if (!is_on_road(x, y - 1)) {
                        set_tile_world(chunk, x, y - 1, road_edge,
                                      render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
                    }
                    if (!is_on_road(x, y + road.width)) {
                        set_tile_world(chunk, x, y + road.width, road_edge,
                                      render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
                    }
                }
                
                if (x == road.x2 && y == road.y2) break;
                if (x != road.x2) x += dx;
                if (y != road.y2) y += dy;
            }
        }
    }
    
    bool is_on_road(int x, int y) const {
        for (const auto& road : roads) {
            int min_x = std::min(road.x1, road.x2);
            int max_x = std::max(road.x1, road.x2);
            int min_y = std::min(road.y1, road.y2);
            int max_y = std::max(road.y1, road.y2);
            
            // Simple bounding box check
            if (x >= min_x && x <= max_x + road.width - 1 &&
                y >= min_y && y <= max_y + road.width - 1) {
                return true;
            }
        }
        return false;
    }
    
    void render_building_placeholders(Chunk* chunk, int render_min_x, int render_max_x,
                                      int render_min_y, int render_max_y, ChunkCoord chunk_coord) {
        Tile grass(true, ',', "green");
        for (const auto& bld : buildings) {
            if (!bld.placed) continue;
            
            for (int y = bld.top(); y <= bld.bottom(); ++y) {
                for (int x = bld.left(); x <= bld.right(); ++x) {
                    set_tile_world(chunk, x, y, grass,
                                  render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
                }
            }
        }
    }
    
    void render_decorations(Chunk* chunk, int center_x, int center_y, int radius,
                           int render_min_x, int render_max_x,
                           int render_min_y, int render_max_y,
                           ChunkCoord chunk_coord, std::mt19937& rng) {
        std::uniform_real_distribution<float> chance(0.0f, 1.0f);
        
        // Town square with decorative cobblestone
        Tile cobble(true, ':', "gray");
        Tile cobble_accent(true, '+', "gray");
        
        int plaza_radius = 8;
        for (int dy = -plaza_radius; dy <= plaza_radius; ++dy) {
            for (int dx = -plaza_radius; dx <= plaza_radius; ++dx) {
                if (dx * dx + dy * dy <= plaza_radius * plaza_radius) {
                    int ring = static_cast<int>(std::sqrt(dx * dx + dy * dy));
                    Tile tile = (ring % 3 == 0) ? cobble_accent : cobble;
                    set_tile_world(chunk, center_x + dx, center_y + dy, tile,
                                  render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
                }
            }
        }
        
        // Lamp posts along main roads
        Tile lamp(false, 'Y', "yellow");
        for (const auto& node : path_nodes) {
            if (chance(rng) < 0.4f) {
                int dx = node.first - center_x;
                int dy = node.second - center_y;
                int dist = dx * dx + dy * dy;
                
                // Not too close to center, not too far
                if (dist > 100 && dist < (radius * radius * 2 / 3)) {
                    if (!overlaps_building(node.first, node.second)) {
                        set_tile_world(chunk, node.first, node.second, lamp,
                                      render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
                    }
                }
            }
        }
        
        // Planters/flower beds around the plaza
        Tile planter(false, '*', "green");
        int planter_positions[][2] = {{-6, -6}, {6, -6}, {-6, 6}, {6, 6}, {-7, 0}, {7, 0}, {0, -7}, {0, 7}};
        for (const auto& pos : planter_positions) {
            int px = center_x + pos[0];
            int py = center_y + pos[1];
            if (!overlaps_building(px, py) && !is_on_road(px, py)) {
                set_tile_world(chunk, px, py, planter,
                              render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
            }
        }
        
        // Benches around plaza
        Tile bench(true, '=', "brown");
        int bench_positions[][2] = {{-4, -4}, {4, -4}, {-4, 4}, {4, 4}};
        for (const auto& pos : bench_positions) {
            int bx = center_x + pos[0];
            int by = center_y + pos[1];
            if (!overlaps_building(bx, by) && !is_on_road(bx, by)) {
                set_tile_world(chunk, bx, by, bench,
                              render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
            }
        }
    }
    
    void render_village_perimeter(Chunk* chunk, int center_x, int center_y, int radius,
                                  int render_min_x, int render_max_x,
                                  int render_min_y, int render_max_y,
                                  ChunkCoord chunk_coord, std::mt19937& rng) {
        // Check if perimeter is enabled
        if (has_village_config && !active_config.has_perimeter) {
            return;
        }
        
        std::uniform_real_distribution<float> chance(0.0f, 1.0f);
        
        // Get perimeter style from config
        char fence_sym = has_village_config ? active_config.perimeter_fence_symbol : '#';
        std::string fence_color = has_village_config ? active_config.perimeter_fence_color : "brown";
        float tree_chance = has_village_config ? active_config.perimeter_tree_chance : 0.15f;
        char tree_sym = has_village_config ? active_config.perimeter_tree_symbol : 'T';
        std::string tree_color = has_village_config ? active_config.perimeter_tree_color : "green";
        
        Tile hedge(false, '"', "green");
        Tile fence(false, fence_sym, fence_color);
        Tile post(false, 'o', fence_color);
        Tile tree(false, tree_sym, tree_color);
        
        // Draw organic perimeter
        for (int angle_deg = 0; angle_deg < 360; angle_deg += 4) {
            float angle = angle_deg * 3.14159f / 180.0f;
            
            // Vary the radius slightly for organic feel
            float jitter = (chance(rng) - 0.5f) * 4.0f;
            float r = radius + jitter;
            
            int px = center_x + static_cast<int>(r * std::cos(angle));
            int py = center_y + static_cast<int>(r * std::sin(angle));
            
            // Skip gaps for road exits
            bool is_road_exit = false;
            for (const auto& road : roads) {
                int road_end_x = road.x2;
                int road_end_y = road.y2;
                int dist_to_road = std::abs(px - road_end_x) + std::abs(py - road_end_y);
                if (dist_to_road < 6) {
                    is_road_exit = true;
                    break;
                }
            }
            
            if (is_road_exit) continue;
            
            // Vary between hedge, fence, and trees
            float r_val = chance(rng);
            Tile tile;
            if (r_val < tree_chance) {
                tile = tree;
            } else if (r_val < 0.6f) {
                tile = hedge;
            } else if (r_val < 0.9f) {
                tile = fence;
            } else {
                tile = post;  // Occasional fence post
            }
            
            if (!overlaps_building(px, py)) {
                set_tile_world(chunk, px, py, tile,
                              render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
            }
        }
        
        // Add entrance archways at main road exits
        Tile arch_pillar(false, 'I', "white");
        for (const auto& road : roads) {
            if (!road.is_main) continue;
            
            int ex = road.x2;
            int ey = road.y2;
            
            // Calculate perpendicular direction
            int dx = road.x2 - road.x1;
            int dy = road.y2 - road.y1;
            
            // Normalize and get perpendicular
            float len = std::sqrt(dx * dx + dy * dy);
            if (len > 0) {
                int perp_x = static_cast<int>(-dy / len * 2);
                int perp_y = static_cast<int>(dx / len * 2);
                
                if (!overlaps_building(ex + perp_x, ey + perp_y)) {
                    set_tile_world(chunk, ex + perp_x, ey + perp_y, arch_pillar,
                                  render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
                }
                if (!overlaps_building(ex - perp_x, ey - perp_y)) {
                    set_tile_world(chunk, ex - perp_x, ey - perp_y, arch_pillar,
                                  render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
                }
            }
        }
    }
    
    bool overlaps_building(int x, int y) const {
        for (const auto& bld : buildings) {
            if (!bld.placed) continue;
            if (x >= bld.left() - 1 && x <= bld.right() + 1 &&
                y >= bld.top() - 1 && y <= bld.bottom() + 1) {
                return true;
            }
        }
        return false;
    }
};
