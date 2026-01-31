#pragma once
#include <string>
#include <vector>
#include <map>

// Village district types for jigsaw generation
enum class JigsawDistrict {
    NONE = -1,
    CENTER = 0,
    MARKET = 1,
    RESIDENTIAL = 2,
    INDUSTRIAL = 3,
    RELIGIOUS = 4,
    ENTRANCE = 5
};

// Jigsaw placement properties for individual buildings
struct JigsawProperties {
    bool enabled = false;
    JigsawDistrict district = JigsawDistrict::NONE;
    float spawn_weight = 1.0f;
    bool unique = false;
    int width_override = 0;
    int height_override = 0;
};

// Road style configuration
struct JigsawRoadStyle {
    char main_road_symbol = '#';
    std::string main_road_color = "gray";
    int main_road_width = 3;
    char side_road_symbol = ':';
    std::string side_road_color = "gray";
    int side_road_width = 2;
    char path_symbol = '.';
    std::string path_color = "brown";
    int path_width = 1;
    char edge_symbol = '.';
    std::string edge_color = "dark_gray";
};

// Ground tile configuration
struct JigsawGroundTile {
    char symbol = ',';
    std::string color = "green";
    float chance = 1.0f;
};

// District zone definition
struct JigsawDistrictZone {
    JigsawDistrict type = JigsawDistrict::RESIDENTIAL;
    float inner_radius_pct = 0.0f;
    float outer_radius_pct = 1.0f;
    float angle_start = 0.0f;
    float angle_end = 360.0f;
    bool randomize_angle = true;
};

// Building quota for a district
struct JigsawBuildingQuota {
    JigsawDistrict district = JigsawDistrict::RESIDENTIAL;
    int base_count = 5;
    int per_radius = 15;
};

// Full jigsaw village configuration
struct JigsawVillageConfig {
    bool enabled = false;
    
    // Road generation
    JigsawRoadStyle roads;
    int main_roads_min = 3;
    int main_roads_max = 4;
    int road_segments_min = 3;
    int road_segments_max = 4;
    float ring_road_chance = 0.7f;
    std::vector<float> ring_distances = {0.4f, 0.7f};
    float path_connection_chance = 0.25f;
    
    // Ground tiles
    std::vector<JigsawGroundTile> ground_tiles;
    
    // District configuration
    std::vector<JigsawDistrictZone> districts;
    float center_zone_radius_pct = 0.2f;
    float entrance_angle_start = 70.0f;
    float entrance_angle_end = 110.0f;
    float entrance_inner_pct = 0.7f;
    
    // Building placement
    std::vector<JigsawBuildingQuota> building_quotas;
    int center_clear_radius = 10;
    int building_spacing = 4;
    int edge_margin = 3;
    int max_placement_attempts = 100;
    
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
};
