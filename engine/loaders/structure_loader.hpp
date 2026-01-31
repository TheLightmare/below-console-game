#pragma once
#include "../util/json.hpp"
#include "../world/chunk.hpp"
#include "../world/jigsaw_generator.hpp"
#include "../world/tile_drawing_utils.hpp"
#include "../world/tileset.hpp"
#include "../world/village_config.hpp"
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <filesystem>
#include <random>

// Import tile drawing utilities
using namespace TileDrawing;

// Represents a single tile placement instruction
struct TilePlacement {
    int offset_x = 0;
    int offset_y = 0;
    char symbol = '.';
    std::string color = "white";
    bool walkable = true;
    float chance = 1.0f;
    
    TilePlacement() = default;
    TilePlacement(int ox, int oy, char sym, const std::string& col, bool walk, float prob = 1.0f)
        : offset_x(ox), offset_y(oy), symbol(sym), color(col), walkable(walk), chance(prob) {}
};

// Represents a shape to draw (rect, circle, line, etc.)
struct ShapeDefinition {
    std::string type;
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    int radius = 0, width = 1;
    char symbol = '.';
    std::string color = "white";
    bool walkable = true, filled = false;
    float chance = 1.0f;
};

// Represents an entity to spawn
struct EntitySpawn {
    std::string type, name, subtype;
    int offset_x = 0, offset_y = 0;
    int level_min = -1, level_max = -1;  // -1 means use template's level
    int count_min = 1, count_max = 1;
    float chance = 1.0f;
    int radius = 0;
    std::map<std::string, std::string> properties;
};

// Represents a sub-structure placement for composite structures
struct ComponentPlacement {
    std::string structure_id;
    int count_min = 1, count_max = 1;
    int offset_x = 0, offset_y = 0;
    int radius = 0, spacing = 3;
    int avoid_center = 0, avoid_edges = 2;
    float chance = 1.0f;
    bool random_rotation = false;
    std::vector<std::string> avoid_tags, require_tags;
};

// Complete structure definition loaded from JSON
struct StructureDefinition {
    std::string id, name, description;
    int structure_type_id = -1;
    std::map<int, float> biome_spawn_chances;
    int min_size = 5, max_size = 10;
    std::vector<TilePlacement> tiles;
    std::vector<ShapeDefinition> shapes;
    std::vector<EntitySpawn> spawns;
    std::vector<ComponentPlacement> components;
    std::vector<std::string> tags;
    JigsawProperties jigsaw;
    JigsawVillageConfig village_config;
    bool valid = false;
    
    bool is_composite() const { return !components.empty(); }
    bool is_jigsaw_village() const { return village_config.enabled; }
    bool has_tag(const std::string& tag) const {
        for (const auto& t : tags) if (t == tag) return true;
        return false;
    }
    int get_width() const {
        if (jigsaw.width_override > 0) return jigsaw.width_override;
        int max_x = 0;
        for (const auto& tile : tiles) max_x = std::max(max_x, tile.offset_x + 1);
        return max_x > 0 ? max_x : min_size;
    }
    int get_height() const {
        if (jigsaw.height_override > 0) return jigsaw.height_override;
        int max_y = 0;
        for (const auto& tile : tiles) max_y = std::max(max_y, tile.offset_y + 1);
        return max_y > 0 ? max_y : min_size;
    }
};

// Loads and manages structure definitions from JSON files
class StructureLoader {
private:
    std::map<std::string, StructureDefinition> structures_by_id;
    std::map<int, StructureDefinition*> structures_by_type;
    std::string structures_path;
    JigsawGenerator jigsaw_generator;
    
    // === Compact JSON parsing helpers ===
    static std::string str(const json::Value& v, const std::string& def = "") { return v.get_string(def); }
    static int num(const json::Value& v, int def = 0) { return v.get_int(def); }
    static float flt(const json::Value& v, float def = 0.f) { return static_cast<float>(v.get_number(def)); }
    static bool flag(const json::Value& v, bool def = false) { return v.get_bool(def); }
    static char sym(const json::Value& v, char def = '.') { auto s = v.get_string(""); return s.empty() ? def : s[0]; }
    static std::string parse_color(const json::Value& val, const std::string& def = "white") {
        return val.is_string() ? val.as_string() : def;
    }
    
    // Parse a tile placement from JSON (supports tileset refs via "_tile" key)
    TilePlacement parse_tile(const json::Value& j) {
        TilePlacement t;
        t.offset_x = num(j["x"]); t.offset_y = num(j["y"]);
        if (j.has("_tile") && !str(j["_tile"]).empty()) {
            const auto& def = get_tileset().get(str(j["_tile"]));
            t.symbol = def.symbol; t.color = def.color; t.walkable = def.walkable;
        } else {
            t.symbol = sym(j["symbol"], '.'); t.color = parse_color(j["color"]);
            t.walkable = flag(j["walkable"], true);
        }
        t.chance = flt(j["chance"], 1.f);
        return t;
    }
    
    // Parse a shape definition from JSON
    ShapeDefinition parse_shape(const json::Value& j) {
        ShapeDefinition s;
        s.type = str(j["type"], "rect");
        s.x1 = num(j["x1"], num(j["x"])); s.y1 = num(j["y1"], num(j["y"]));
        s.x2 = num(j["x2"], s.x1); s.y2 = num(j["y2"], s.y1);
        s.radius = num(j["radius"], 3); s.width = num(j["width"], 1);
        s.symbol = sym(j["symbol"], '.'); s.color = parse_color(j["color"]);
        s.walkable = flag(j["walkable"], true); s.filled = flag(j["filled"]);
        s.chance = flt(j["chance"], 1.f);
        return s;
    }
    
    // Parse an entity spawn from JSON
    EntitySpawn parse_spawn(const json::Value& j) {
        EntitySpawn s;
        s.type = str(j["type"], "enemy"); s.name = str(j["name"]); s.subtype = str(j["subtype"]);
        s.offset_x = num(j["x"]); s.offset_y = num(j["y"]);
        // -1 means "use entity template's level", only override if explicitly specified
        s.level_min = num(j["level_min"], num(j["level"], -1)); s.level_max = num(j["level_max"], s.level_min);
        s.count_min = num(j["count_min"], num(j["count"], 1)); s.count_max = num(j["count_max"], s.count_min);
        s.chance = flt(j["chance"], 1.f); s.radius = num(j["radius"]);
        if (j.has("properties") && j["properties"].is_object())
            for (const auto& [k, v] : j["properties"].as_object()) s.properties[k] = v.get_string("");
        return s;
    }
    
    // Parse a component placement from JSON
    ComponentPlacement parse_component(const json::Value& j) {
        ComponentPlacement c;
        c.structure_id = str(j["structure"], str(j["id"]));
        c.count_min = num(j["count_min"], num(j["count"], 1)); c.count_max = num(j["count_max"], c.count_min);
        c.offset_x = num(j["x"]); c.offset_y = num(j["y"]); c.radius = num(j["radius"]);
        c.spacing = num(j["spacing"], 3); c.avoid_center = num(j["avoid_center"]);
        c.avoid_edges = num(j["avoid_edges"], 2); c.chance = flt(j["chance"], 1.f);
        c.random_rotation = flag(j["random_rotation"]);
        auto parse_tags = [](const json::Value& arr, std::vector<std::string>& out) {
            if (arr.is_array()) for (size_t i = 0; i < arr.size(); ++i) out.push_back(arr[i].get_string(""));
        };
        parse_tags(j["avoid_tags"], c.avoid_tags); parse_tags(j["require_tags"], c.require_tags);
        return c;
    }
    
    // Parse biome spawn chances
    void parse_biome_chances(StructureDefinition& def, const json::Value& biomes_json) {
        if (!biomes_json.is_object()) return;
        
        // Map biome names to IDs
        static const std::map<std::string, int> biome_ids = {
            {"water", 0}, {"beach", 1}, {"plains", 2}, {"forest", 3},
            {"hills", 4}, {"mountains", 5}, {"desert", 6}, {"swamp", 7},
            {"all", -1}  // Special: applies to all biomes
        };
        
        for (const auto& [biome_name, chance_val] : biomes_json.as_object()) {
            auto it = biome_ids.find(biome_name);
            if (it != biome_ids.end()) {
                float chance = static_cast<float>(chance_val.get_number(0.0));
                if (it->second == -1) {
                    // "all" - set for all biomes
                    for (int i = 0; i <= 7; ++i) {
                        if (def.biome_spawn_chances.find(i) == def.biome_spawn_chances.end()) {
                            def.biome_spawn_chances[i] = chance;
                        }
                    }
                } else {
                    def.biome_spawn_chances[it->second] = chance;
                }
            }
        }
    }
    
    // Represents a tile legend entry for ASCII art patterns
    struct TileLegend {
        char symbol = '.';
        std::string color = "white";
        bool walkable = true;
        float chance = 1.0f;
        bool is_spawn = false;         // If true, this is a spawn marker not a tile
        std::string spawn_type;        // "enemy", "npc", "item"
        std::string spawn_subtype;     // Specific type
        std::string spawn_name;
    };
    
    // Parse tile legend/palette from JSON
    // Supports tileset references via "_tile" key for consistent tile definitions
    std::map<char, TileLegend> parse_legend(const json::Value& legend_json) {
        std::map<char, TileLegend> legend;
        
        if (!legend_json.is_object()) return legend;
        
        for (const auto& [key, val] : legend_json.as_object()) {
            if (key.empty()) continue;
            char c = key[0];
            
            TileLegend entry;
            entry.symbol = c;  // Default: use same symbol
            
            if (val.is_string()) {
                // Simple format: just a color
                entry.color = val.as_string();
            } else if (val.is_object()) {
                // Check for tileset reference first
                if (val.has("_tile")) {
                    std::string tile_id = val["_tile"].get_string("");
                    if (!tile_id.empty()) {
                        const TileDef& def = get_tileset().get(tile_id);
                        entry.symbol = def.symbol;
                        entry.color = def.color;
                        entry.walkable = def.walkable;
                        entry.chance = static_cast<float>(val["chance"].get_number(1.0));
                    }
                } else {
                    // Full format with all properties
                    std::string sym_str = val["symbol"].get_string(key);
                    entry.symbol = sym_str.empty() ? c : sym_str[0];
                    entry.color = parse_color(val["color"], "white");
                    entry.walkable = val["walkable"].get_bool(true);
                    entry.chance = static_cast<float>(val["chance"].get_number(1.0));
                }
                
                // Check if this is a spawn marker
                if (val.has("spawn")) {
                    entry.is_spawn = true;
                    entry.spawn_type = val["spawn"].get_string("enemy");
                    entry.spawn_subtype = val["spawn_subtype"].get_string(val["subtype"].get_string(""));
                    entry.spawn_name = val["spawn_name"].get_string(val["name"].get_string(""));
                }
            }
            
            legend[c] = entry;
        }
        
        // Default entries for common symbols (can be overridden)
        if (legend.find(' ') == legend.end()) {
            // Space = skip/transparent
        }
        if (legend.find('.') == legend.end()) {
            legend['.'] = TileLegend{'.', "gray", true, 1.0f, false, "", "", ""};
        }
        if (legend.find('#') == legend.end()) {
            legend['#'] = TileLegend{'#', "gray", false, 1.0f, false, "", "", ""};
        }
        
        return legend;
    }
    
    // Parse ASCII art pattern and convert to tiles
    void parse_pattern(StructureDefinition& def, const json::Value& pattern_json, 
                       const std::map<char, TileLegend>& legend) {
        if (!pattern_json.is_array()) return;
        
        // Get all lines
        std::vector<std::string> lines;
        for (size_t i = 0; i < pattern_json.size(); ++i) {
            lines.push_back(pattern_json[i].get_string(""));
        }
        
        if (lines.empty()) return;
        
        // Find dimensions and center
        size_t max_width = 0;
        for (const auto& line : lines) {
            max_width = std::max(max_width, line.size());
        }
        
        int height = static_cast<int>(lines.size());
        int width = static_cast<int>(max_width);
        
        // Center offset (pattern is centered on structure center)
        int offset_x = -width / 2;
        int offset_y = -height / 2;
        
        // Parse each character
        for (int y = 0; y < height; ++y) {
            const std::string& line = lines[y];
            for (int x = 0; x < static_cast<int>(line.size()); ++x) {
                char c = line[x];
                
                // Skip spaces (transparent)
                if (c == ' ') continue;
                
                // Look up in legend
                auto it = legend.find(c);
                if (it != legend.end()) {
                    const TileLegend& entry = it->second;
                    
                    if (entry.is_spawn) {
                        // Add as entity spawn
                        EntitySpawn spawn;
                        spawn.type = entry.spawn_type;
                        spawn.subtype = entry.spawn_subtype;
                        spawn.name = entry.spawn_name;
                        spawn.offset_x = offset_x + x;
                        spawn.offset_y = offset_y + y;
                        spawn.chance = entry.chance;
                        spawn.count_min = 1;
                        spawn.count_max = 1;
                        def.spawns.push_back(spawn);
                    } else {
                        // Add as tile
                        TilePlacement tile;
                        tile.offset_x = offset_x + x;
                        tile.offset_y = offset_y + y;
                        tile.symbol = entry.symbol;
                        tile.color = entry.color;
                        tile.walkable = entry.walkable;
                        tile.chance = entry.chance;
                        def.tiles.push_back(tile);
                    }
                } else {
                    // No legend entry - use character as-is with default properties
                    TilePlacement tile;
                    tile.offset_x = offset_x + x;
                    tile.offset_y = offset_y + y;
                    tile.symbol = c;
                    tile.color = "white";
                    tile.walkable = true;
                    tile.chance = 1.0f;
                    def.tiles.push_back(tile);
                }
            }
        }
    }
    
    // Helper to parse district from string
    JigsawDistrict parse_district_string(const std::string& s) {
        static const std::map<std::string, JigsawDistrict> m = {
            {"center", JigsawDistrict::CENTER}, {"market", JigsawDistrict::MARKET},
            {"residential", JigsawDistrict::RESIDENTIAL}, {"industrial", JigsawDistrict::INDUSTRIAL},
            {"religious", JigsawDistrict::RELIGIOUS}, {"entrance", JigsawDistrict::ENTRANCE}
        };
        auto it = m.find(s); return it != m.end() ? it->second : JigsawDistrict::RESIDENTIAL;
    }
    
    // Parse road style helper
    void parse_road_style(const json::Value& s, char& symOut, std::string& colOut, int& wOut, char defSym, int defW) {
        symOut = sym(s["symbol"], defSym); colOut = str(s["color"], "gray"); wOut = num(s["width"], defW);
    }
    
    // Parse string array helper
    void parse_str_array(const json::Value& arr, std::vector<std::string>& out) {
        if (arr.is_array()) for (size_t i = 0; i < arr.size(); ++i) out.push_back(arr[i].get_string(""));
    }
    
    // Parse full village configuration from JSON (compact)
    void parse_village_config(JigsawVillageConfig& c, const json::Value& j) {
        c.enabled = true;
        if (j.has("roads")) {
            const auto& r = j["roads"];
            c.main_roads_min = num(r["main_roads_min"], 3); c.main_roads_max = num(r["main_roads_max"], 4);
            c.road_segments_min = num(r["segments_min"], 3); c.road_segments_max = num(r["segments_max"], 4);
            c.ring_road_chance = flt(r["ring_chance"], 0.7f); c.path_connection_chance = flt(r["path_chance"], 0.25f);
            if (r.has("ring_distances") && r["ring_distances"].is_array()) {
                c.ring_distances.clear();
                for (size_t i = 0; i < r["ring_distances"].size(); ++i)
                    c.ring_distances.push_back(flt(r["ring_distances"][i], 0.5f));
            }
            if (r.has("main_style")) parse_road_style(r["main_style"], c.roads.main_road_symbol, c.roads.main_road_color, c.roads.main_road_width, '#', 3);
            if (r.has("side_style")) parse_road_style(r["side_style"], c.roads.side_road_symbol, c.roads.side_road_color, c.roads.side_road_width, ':', 2);
            if (r.has("path_style")) parse_road_style(r["path_style"], c.roads.path_symbol, c.roads.path_color, c.roads.path_width, '.', 1);
        }
        if (j.has("ground") && j["ground"].is_array()) {
            c.ground_tiles.clear();
            for (size_t i = 0; i < j["ground"].size(); ++i) {
                const auto& t = j["ground"][i];
                c.ground_tiles.push_back({sym(t["symbol"], ','), str(t["color"], "green"), flt(t["chance"], 1.f)});
            }
        }
        if (j.has("districts") && j["districts"].is_array()) {
            c.districts.clear();
            for (size_t i = 0; i < j["districts"].size(); ++i) {
                const auto& d = j["districts"][i];
                c.districts.push_back({parse_district_string(str(d["type"], "residential")),
                    flt(d["inner_radius"], 0.f), flt(d["outer_radius"], 1.f),
                    flt(d["angle_start"], 0.f), flt(d["angle_end"], 360.f), flag(d["randomize"], true)});
            }
        }
        c.center_zone_radius_pct = flt(j["center_radius"], 0.2f);
        c.entrance_angle_start = flt(j["entrance_angle_start"], 70.f);
        c.entrance_angle_end = flt(j["entrance_angle_end"], 110.f);
        c.entrance_inner_pct = flt(j["entrance_inner"], 0.7f);
        if (j.has("building_quotas") && j["building_quotas"].is_array()) {
            c.building_quotas.clear();
            for (size_t i = 0; i < j["building_quotas"].size(); ++i) {
                const auto& q = j["building_quotas"][i];
                c.building_quotas.push_back({parse_district_string(str(q["district"], "residential")),
                    num(q["base_count"], 5), num(q["per_radius"], 15)});
            }
        }
        c.center_clear_radius = num(j["center_clear"], 10); c.building_spacing = num(j["building_spacing"], 4);
        c.edge_margin = num(j["edge_margin"], 3); c.max_placement_attempts = num(j["max_attempts"], 100);
        if (j.has("perimeter")) {
            const auto& p = j["perimeter"];
            c.has_perimeter = flag(p["enabled"], true);
            c.perimeter_fence_symbol = sym(p["fence_symbol"], '#'); c.perimeter_fence_color = str(p["fence_color"], "brown");
            c.perimeter_tree_chance = flt(p["tree_chance"], 0.15f);
            c.perimeter_tree_symbol = sym(p["tree_symbol"], 'T'); c.perimeter_tree_color = str(p["tree_color"], "green");
        }
        if (j.has("decorations")) {
            const auto& d = j["decorations"];
            c.flower_chance = flt(d["flower_chance"], 0.02f); c.flower_symbol = sym(d["flower_symbol"], '*');
            c.flower_color = str(d["flower_color"], "yellow"); c.dirt_patch_chance = flt(d["dirt_chance"], 0.08f);
        }
        parse_str_array(j["allowed_buildings"], c.allowed_buildings);
        parse_str_array(j["required_buildings"], c.required_buildings);
    }
    
public:
    StructureLoader() = default;
    
    void set_structures_path(const std::string& path) {
        structures_path = path;
    }
    
    // Load a single structure from JSON
    bool load_structure(const std::string& filepath) {
        try {
            json::Value root = json::parse_file(filepath);
            
            StructureDefinition def;
            def.id = root["id"].get_string("");
            def.name = root["name"].get_string(def.id);
            def.description = root["description"].get_string("");
            def.structure_type_id = root["type_id"].get_int(-1);
            def.min_size = root["min_size"].get_int(5);
            def.max_size = root["max_size"].get_int(10);
            
            // Parse biome spawn chances
            if (root.has("biomes")) {
                parse_biome_chances(def, root["biomes"]);
            }
            
            // Parse legend/palette for ASCII patterns
            std::map<char, TileLegend> legend;
            if (root.has("legend")) {
                legend = parse_legend(root["legend"]);
            }
            
            // Parse ASCII art pattern (converted to tiles)
            if (root.has("pattern") && root["pattern"].is_array()) {
                parse_pattern(def, root["pattern"], legend);
            }
            
            // Parse individual tiles (can be combined with pattern)
            if (root.has("tiles") && root["tiles"].is_array()) {
                for (size_t i = 0; i < root["tiles"].size(); ++i) {
                    def.tiles.push_back(parse_tile(root["tiles"][i]));
                }
            }
            
            // Parse shapes
            if (root.has("shapes") && root["shapes"].is_array()) {
                for (size_t i = 0; i < root["shapes"].size(); ++i) {
                    def.shapes.push_back(parse_shape(root["shapes"][i]));
                }
            }
            
            // Parse entity spawns
            if (root.has("spawns") && root["spawns"].is_array()) {
                for (size_t i = 0; i < root["spawns"].size(); ++i) {
                    def.spawns.push_back(parse_spawn(root["spawns"][i]));
                }
            }
            
            // Parse sub-structure components (for composite structures)
            if (root.has("components") && root["components"].is_array()) {
                for (size_t i = 0; i < root["components"].size(); ++i) {
                    def.components.push_back(parse_component(root["components"][i]));
                }
            }
            
            // Parse tags
            if (root.has("tags") && root["tags"].is_array()) {
                for (size_t i = 0; i < root["tags"].size(); ++i) {
                    def.tags.push_back(root["tags"][i].get_string(""));
                }
            }
            
            // Parse jigsaw properties for village generation
            if (root.has("jigsaw")) {
                const json::Value& jigsaw = root["jigsaw"];
                def.jigsaw.enabled = true;
                
                // Parse district
                std::string district_str = jigsaw["district"].get_string("residential");
                if (district_str == "center") def.jigsaw.district = JigsawDistrict::CENTER;
                else if (district_str == "market") def.jigsaw.district = JigsawDistrict::MARKET;
                else if (district_str == "residential") def.jigsaw.district = JigsawDistrict::RESIDENTIAL;
                else if (district_str == "industrial") def.jigsaw.district = JigsawDistrict::INDUSTRIAL;
                else if (district_str == "religious") def.jigsaw.district = JigsawDistrict::RELIGIOUS;
                else if (district_str == "entrance") def.jigsaw.district = JigsawDistrict::ENTRANCE;
                
                def.jigsaw.spawn_weight = static_cast<float>(jigsaw["spawn_weight"].get_number(1.0));
                def.jigsaw.unique = jigsaw["unique"].get_bool(false);
                def.jigsaw.width_override = jigsaw["width"].get_int(0);
                def.jigsaw.height_override = jigsaw["height"].get_int(0);
            }
            
            // Parse village_config for jigsaw village generation
            if (root.has("village_config")) {
                parse_village_config(def.village_config, root["village_config"]);
            }
            
            def.valid = true;
            
            // Store by ID
            structures_by_id[def.id] = def;
            
            // Store by type ID if valid
            if (def.structure_type_id >= 0) {
                structures_by_type[def.structure_type_id] = &structures_by_id[def.id];
            }
            
            return true;
        } catch (const std::exception& e) {
            // Log error but don't crash
            (void)e;
            return false;
        }
    }
    
    // Load all structures from a directory
    int load_all_structures(const std::string& directory) {
        int count = 0;
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                if (entry.path().extension() == ".json") {
                    if (load_structure(entry.path().string())) {
                        count++;
                    }
                }
            }
        } catch (...) {
            // Directory doesn't exist or can't be read
        }
        
        // Update jigsaw generator with loaded structure data
        jigsaw_generator.set_building_catalog_from_structures(structures_by_id);
        
        return count;
    }
    
    // Get structure by ID
    const StructureDefinition* get_structure(const std::string& id) const {
        auto it = structures_by_id.find(id);
        return it != structures_by_id.end() ? &it->second : nullptr;
    }
    
    // Get structure by type ID
    const StructureDefinition* get_structure_by_type(int type_id) const {
        auto it = structures_by_type.find(type_id);
        return it != structures_by_type.end() ? it->second : nullptr;
    }
    
    // Get all loaded structures
    const std::map<std::string, StructureDefinition>& get_all_structures() const {
        return structures_by_id;
    }
    
    // Check if a structure type has a JSON definition
    bool has_json_definition(int type_id) const {
        return structures_by_type.find(type_id) != structures_by_type.end();
    }
    
    // Generate a structure from JSON definition onto a chunk
    // depth parameter prevents infinite recursion for composite structures
    void generate_structure(Chunk* chunk, const StructureDefinition& def, 
                           int center_x, int center_y, std::mt19937& rng, int depth = 0) {
        // Prevent infinite recursion (max 5 levels of nesting)
        if (depth > 5) return;
        
        std::uniform_real_distribution<float> chance_dist(0.0f, 1.0f);
        
        // Draw shapes first (background)
        for (const auto& shape : def.shapes) {
            if (chance_dist(rng) > shape.chance) continue;
            
            Tile tile(shape.walkable, shape.symbol, shape.color);
            
            if (shape.type == "rect" || shape.type == "rect_filled") {
                fill_rect(chunk, center_x + shape.x1, center_y + shape.y1,
                         center_x + shape.x2, center_y + shape.y2, tile);
            }
            else if (shape.type == "rect_outline") {
                draw_rect_outline(chunk, center_x + shape.x1, center_y + shape.y1,
                                 center_x + shape.x2, center_y + shape.y2, tile);
            }
            else if (shape.type == "circle" || shape.type == "circle_filled") {
                draw_circle(chunk, center_x + shape.x1, center_y + shape.y1,
                           shape.radius, tile, true);
            }
            else if (shape.type == "circle_outline") {
                draw_circle(chunk, center_x + shape.x1, center_y + shape.y1,
                           shape.radius, tile, false);
            }
            else if (shape.type == "line") {
                draw_line(chunk, center_x + shape.x1, center_y + shape.y1,
                         center_x + shape.x2, center_y + shape.y2, tile, shape.width);
            }
        }
        
        // Place individual tiles (foreground)
        for (const auto& tp : def.tiles) {
            if (chance_dist(rng) > tp.chance) continue;
            
            int x = center_x + tp.offset_x;
            int y = center_y + tp.offset_y;
            
            if (x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_SIZE) {
                chunk->set_tile(x, y, Tile(tp.walkable, tp.symbol, tp.color));
            }
        }
        
        // Place sub-structure components (for composite structures)
        generate_components(chunk, def, center_x, center_y, rng, depth);
    }
    
    // Generate a structure using world coordinates (for multi-chunk support)
    void generate_structure_world(Chunk* chunk, const StructureDefinition& def, 
                                  int center_x, int center_y, std::mt19937& rng,
                                  int render_min_x, int render_max_x,
                                  int render_min_y, int render_max_y,
                                  ChunkCoord chunk_coord, int depth = 0) {
        // Prevent infinite recursion (max 5 levels of nesting)
        if (depth > 5) return;
        
        std::uniform_real_distribution<float> chance_dist(0.0f, 1.0f);
        
        // Draw shapes first (background)
        for (const auto& shape : def.shapes) {
            if (chance_dist(rng) > shape.chance) continue;
            
            Tile tile(shape.walkable, shape.symbol, shape.color);
            
            if (shape.type == "rect" || shape.type == "rect_filled") {
                fill_rect_world(chunk, center_x + shape.x1, center_y + shape.y1,
                               center_x + shape.x2, center_y + shape.y2, tile,
                               render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
            }
            else if (shape.type == "rect_outline") {
                draw_rect_outline_world(chunk, center_x + shape.x1, center_y + shape.y1,
                                       center_x + shape.x2, center_y + shape.y2, tile,
                                       render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
            }
            else if (shape.type == "circle" || shape.type == "circle_filled") {
                draw_circle_world(chunk, center_x + shape.x1, center_y + shape.y1,
                                 shape.radius, tile, true,
                                 render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
            }
            else if (shape.type == "circle_outline") {
                draw_circle_world(chunk, center_x + shape.x1, center_y + shape.y1,
                                 shape.radius, tile, false,
                                 render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
            }
            else if (shape.type == "line") {
                draw_line_world(chunk, center_x + shape.x1, center_y + shape.y1,
                               center_x + shape.x2, center_y + shape.y2, tile, shape.width,
                               render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
            }
        }
        
        // Place individual tiles (foreground)
        for (const auto& tp : def.tiles) {
            if (chance_dist(rng) > tp.chance) continue;
            
            int x = center_x + tp.offset_x;
            int y = center_y + tp.offset_y;
            
            set_tile_world(chunk, x, y, Tile(tp.walkable, tp.symbol, tp.color),
                          render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
        }
    }
    
    // Render JSON buildings planned by jigsaw generator (world coordinates)
    void render_village_buildings_world(Chunk* chunk, int /*center_x*/, int /*center_y*/,
                                        int /*village_radius*/, std::mt19937& rng,
                                        int render_min_x, int render_max_x,
                                        int render_min_y, int render_max_y,
                                        ChunkCoord chunk_coord) {
        // Get the planned buildings from jigsaw generator (already generated roads)
        const auto& planned_buildings = jigsaw_generator.get_buildings();
        
        // Render actual JSON structures for each planned building
        for (const auto& planned : planned_buildings) {
            if (!planned.placed) continue;
            
            // Get the JSON structure definition
            const StructureDefinition* struct_def = get_structure(planned.structure_id);
            if (struct_def && struct_def->valid) {
                // Render the actual JSON structure at this position
                generate_structure_world(chunk, *struct_def, 
                                        planned.center_x, planned.center_y, rng,
                                        render_min_x, render_max_x,
                                        render_min_y, render_max_y, chunk_coord, 1);
            }
        }
    }
    
    // Access to jigsaw generator for external callers
    JigsawGenerator& get_jigsaw_generator() { return jigsaw_generator; }
    
private:
    // Track placed component positions for spacing checks
    struct PlacedComponent {
        int x, y;
        int width, height;
        std::string structure_id;
        std::vector<std::string> tags;
    };
    
public:
    // Represents a placed building for village generation
    struct PlacedBuilding {
        int x, y;           // Center position
        int width, height;  // Bounds
        std::string id;
        int door_x, door_y; // Door/entrance position
        std::vector<std::string> tags;
        
        int center_x() const { return x; }
        int center_y() const { return y; }
        
        // Get bounds
        int left() const { return x - width / 2; }
        int right() const { return x + width / 2; }
        int top() const { return y - height / 2; }
        int bottom() const { return y + height / 2; }
        
        bool intersects(const PlacedBuilding& other, int padding = 2) const {
            return left() - padding < other.right() + padding &&
                   right() + padding > other.left() - padding &&
                   top() - padding < other.bottom() + padding &&
                   bottom() + padding > other.top() - padding;
        }
    };
    
    // Generate a village using jigsaw generation
    std::vector<PlacedBuilding> generate_village(Chunk* chunk, const StructureDefinition& def,
                                                  int center_x, int center_y, std::mt19937& rng) {
        std::vector<PlacedBuilding> buildings;
        int village_radius = (def.max_size + def.min_size) / 2;
        if (village_radius < 15) village_radius = 52;
        
        jigsaw_generator.generate_village(chunk, center_x, center_y, village_radius, rng);
        
        for (const auto& planned : jigsaw_generator.get_buildings()) {
            if (!planned.placed) continue;
            const StructureDefinition* struct_def = get_structure(planned.structure_id);
            if (struct_def && struct_def->valid) {
                generate_structure(chunk, *struct_def, planned.center_x, planned.center_y, rng, 1);
                buildings.push_back({planned.center_x, planned.center_y, planned.width, planned.height,
                    planned.structure_id, planned.door_x, planned.door_y, planned.tags});
            }
        }
        return buildings;
    }
    
private:
    // Generate sub-structure components (for composite structures)
    void generate_components(Chunk* chunk, const StructureDefinition& def,
                            int center_x, int center_y, std::mt19937& rng, int depth) {
        if (def.components.empty()) return;
        std::uniform_real_distribution<float> chance_dist(0.0f, 1.0f);
        std::vector<PlacedComponent> placed;
        
        for (const auto& comp : def.components) {
            const StructureDefinition* sub_def = get_structure(comp.structure_id);
            if (!sub_def || !sub_def->valid) continue;
            
            int count = comp.count_min;
            if (comp.count_max > comp.count_min) {
                std::uniform_int_distribution<int> count_dist(comp.count_min, comp.count_max);
                count = count_dist(rng);
            }
            
            for (int i = 0; i < count; ++i) {
                if (chance_dist(rng) > comp.chance) continue;
                int px, py;
                if (find_component_position(comp, center_x, center_y, placed, rng, px, py)) {
                    generate_structure(chunk, *sub_def, px, py, rng, depth + 1);
                    placed.push_back({px, py, 0, 0, comp.structure_id, {}});
                }
            }
        }
    }
    
    // Find valid position for component placement
    bool find_component_position(const ComponentPlacement& comp, int cx, int cy,
                                 const std::vector<PlacedComponent>& placed,
                                 std::mt19937& rng, int& ox, int& oy) {
        for (int attempt = 0; attempt < 20; ++attempt) {
            int px, py;
            if (comp.radius > 0) {
                std::uniform_int_distribution<int> off(-comp.radius, comp.radius);
                px = cx + comp.offset_x + off(rng); py = cy + comp.offset_y + off(rng);
            } else {
                px = cx + comp.offset_x; py = cy + comp.offset_y;
            }
            if (comp.avoid_center > 0) {
                int dx = px - cx, dy = py - cy;
                if (dx*dx + dy*dy < comp.avoid_center * comp.avoid_center) continue;
            }
            if (comp.avoid_edges > 0 && (px < comp.avoid_edges || px >= CHUNK_SIZE - comp.avoid_edges ||
                py < comp.avoid_edges || py >= CHUNK_SIZE - comp.avoid_edges)) continue;
            bool too_close = false;
            for (const auto& o : placed) {
                int dx = px - o.x, dy = py - o.y;
                if (dx*dx + dy*dy < comp.spacing * comp.spacing) { too_close = true; break; }
            }
            if (too_close) continue;
            ox = px; oy = py; return true;
        }
        return false;
    }
};
