#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "world.hpp"

// Tile categories for organization
enum class TileCategory {
    TERRAIN_NATURAL,    // Natural ground tiles (grass, dirt, sand, etc.)
    TERRAIN_WATER,      // Water-related tiles
    TERRAIN_ROCK,       // Stone, mountains, cliffs
    TERRAIN_SNOW,       // Snow and ice
    VEGETATION,         // Trees, plants, flowers
    STRUCTURE_WALL,     // Building walls
    STRUCTURE_FLOOR,    // Building floors
    STRUCTURE_DOOR,     // Doors and entrances
    STRUCTURE_WINDOW,   // Windows
    FURNITURE,          // Indoor furniture
    DECORATION,         // Decorative items
    SPECIAL,            // Special/magical tiles
    PATHWAY,            // Roads and paths
    INTERACTIVE         // Interactable objects
};

// Core tile definition
struct TileDef {
    char symbol;
    std::string color;
    bool walkable;
    std::string name;
    std::string description;
    TileCategory category;
    
    // Gameplay modifiers
    float movement_cost = 1.0f;    // 1.0 = normal, <1 = slow, >1 = fast
    float visibility_mod = 1.0f;   // Affects view distance
    bool blocks_sight = false;     // Does this tile block line of sight?
    bool is_liquid = false;        // Is this water/lava?
    bool is_dangerous = false;     // Does this damage the player?
    
    TileDef() : symbol('.'), color("white"), walkable(true), name("unknown"), 
                description(""), category(TileCategory::TERRAIN_NATURAL) {}
    
    TileDef(char sym, const std::string& col, bool walk, const std::string& n,
            const std::string& desc = "", TileCategory cat = TileCategory::TERRAIN_NATURAL)
        : symbol(sym), color(col), walkable(walk), name(n), description(desc), category(cat) {}
    
    // Fluent setters for optional properties
    TileDef& movement(float cost) { movement_cost = cost; return *this; }
    TileDef& visibility(float mod) { visibility_mod = mod; return *this; }
    TileDef& blocking() { blocks_sight = true; return *this; }
    TileDef& liquid() { is_liquid = true; return *this; }
    TileDef& dangerous() { is_dangerous = true; return *this; }
};

// TILE IDs
namespace TileID {
    // TERRAIN: NATURAL GROUND
    constexpr const char* GRASS              = "grass";
    constexpr const char* GRASS_TALL         = "grass_tall";
    constexpr const char* GRASS_SHORT        = "grass_short";
    constexpr const char* DIRT               = "dirt";
    constexpr const char* DIRT_PATH          = "dirt_path";
    constexpr const char* MUD                = "mud";
    constexpr const char* SAND               = "sand";
    constexpr const char* SAND_FINE          = "sand_fine";
    constexpr const char* ASH                = "ash";
    constexpr const char* MYCELIUM           = "mycelium";
    constexpr const char* DEAD_GROUND        = "dead_ground";
    
    // TERRAIN: WATER
    constexpr const char* WATER              = "water";
    constexpr const char* WATER_DEEP         = "water_deep";
    constexpr const char* WATER_SHALLOW      = "water_shallow";
    constexpr const char* WATER_MURKY        = "water_murky";
    constexpr const char* WAVE               = "wave";
    constexpr const char* LILY_PADS          = "lily_pads";
    constexpr const char* ICE                = "ice";
    constexpr const char* LAVA               = "lava";
    constexpr const char* LAVA_COOLING       = "lava_cooling";
    
    // === TERRAIN: ROCK ===
    constexpr const char* STONE              = "stone";
    constexpr const char* STONE_DARK         = "stone_dark";
    constexpr const char* ROCK               = "rock";
    constexpr const char* BOULDER            = "boulder";
    constexpr const char* CLIFF              = "cliff";
    constexpr const char* MOUNTAIN           = "mountain";
    constexpr const char* MOUNTAIN_PEAK      = "mountain_peak";
    constexpr const char* VOLCANIC_ROCK      = "volcanic_rock";
    
    // === TERRAIN: SNOW & ICE ===
    constexpr const char* SNOW               = "snow";
    constexpr const char* SNOW_DEEP          = "snow_deep";
    constexpr const char* FROST              = "frost";
    constexpr const char* ICE_BLOCK          = "ice_block";
    constexpr const char* ICICLE             = "icicle";
    
    // === VEGETATION: TREES ===
    constexpr const char* TREE               = "tree";
    constexpr const char* TREE_PINE          = "tree_pine";
    constexpr const char* TREE_OAK           = "tree_oak";
    constexpr const char* TREE_PALM          = "tree_palm";
    constexpr const char* TREE_DEAD          = "tree_dead";
    constexpr const char* TREE_ANCIENT       = "tree_ancient";
    constexpr const char* SAPLING            = "sapling";
    constexpr const char* STUMP              = "stump";
    
    // === VEGETATION: PLANTS ===
    constexpr const char* BUSH               = "bush";
    constexpr const char* BUSH_BERRY         = "bush_berry";
    constexpr const char* FERN               = "fern";
    constexpr const char* REEDS              = "reeds";
    constexpr const char* VINES              = "vines";
    constexpr const char* UNDERBRUSH         = "underbrush";
    constexpr const char* THICKET            = "thicket";
    constexpr const char* CACTUS             = "cactus";
    
    // === VEGETATION: MUSHROOMS ===
    constexpr const char* MUSHROOM           = "mushroom";
    constexpr const char* MUSHROOM_GLOW      = "mushroom_glow";
    constexpr const char* MUSHROOM_GIANT     = "mushroom_giant";
    constexpr const char* PUFFBALL           = "puffball";
    
    // === VEGETATION: FLOWERS ===
    constexpr const char* FLOWER_WILD        = "flower_wild";
    constexpr const char* FLOWER_RED         = "flower_red";
    constexpr const char* FLOWER_BLUE        = "flower_blue";
    constexpr const char* FLOWER_YELLOW      = "flower_yellow";
    constexpr const char* FLOWER_ORCHID      = "flower_orchid";
    constexpr const char* FLOWER_DESERT      = "flower_desert";
    
    // === STRUCTURE: WALLS ===
    constexpr const char* WALL_WOOD          = "wall_wood";
    constexpr const char* WALL_STONE         = "wall_stone";
    constexpr const char* WALL_BRICK         = "wall_brick";
    constexpr const char* WALL_CASTLE        = "wall_castle";
    constexpr const char* WALL_RUINED        = "wall_ruined";
    constexpr const char* PILLAR             = "pillar";
    constexpr const char* FENCE              = "fence";
    constexpr const char* FENCE_GATE         = "fence_gate";
    
    // === STRUCTURE: FLOORS ===
    constexpr const char* FLOOR_WOOD         = "floor_wood";
    constexpr const char* FLOOR_STONE        = "floor_stone";
    constexpr const char* FLOOR_TILE         = "floor_tile";
    constexpr const char* FLOOR_CARPET       = "floor_carpet";
    constexpr const char* FLOOR_CARPET_FANCY = "floor_carpet_fancy";
    constexpr const char* FLOOR_DIRT         = "floor_dirt";
    
    // === STRUCTURE: DOORS & WINDOWS ===
    constexpr const char* DOOR_WOOD          = "door_wood";
    constexpr const char* DOOR_IRON          = "door_iron";
    constexpr const char* DOOR_ORNATE        = "door_ornate";
    constexpr const char* WINDOW             = "window";
    constexpr const char* WINDOW_BARRED      = "window_barred";
    
    // === STRUCTURE: ROOFS ===
    constexpr const char* ROOF_THATCH        = "roof_thatch";
    constexpr const char* ROOF_TILE          = "roof_tile";
    constexpr const char* CHIMNEY            = "chimney";
    
    // === FURNITURE: SEATING ===
    constexpr const char* CHAIR              = "chair";
    constexpr const char* BENCH              = "bench";
    constexpr const char* THRONE             = "throne";
    constexpr const char* STOOL              = "stool";
    
    // === FURNITURE: TABLES ===
    constexpr const char* TABLE              = "table";
    constexpr const char* COUNTER            = "counter";
    constexpr const char* WORKBENCH          = "workbench";
    constexpr const char* ANVIL              = "anvil";
    
    // === FURNITURE: STORAGE ===
    constexpr const char* CHEST              = "chest";
    constexpr const char* CHEST_LOCKED       = "chest_locked";
    constexpr const char* BARREL             = "barrel";
    constexpr const char* CRATE              = "crate";
    constexpr const char* SHELF              = "shelf";
    constexpr const char* BOOKSHELF          = "bookshelf";
    constexpr const char* CABINET            = "cabinet";
    
    // === FURNITURE: BEDS ===
    constexpr const char* BED                = "bed";
    constexpr const char* BED_FANCY          = "bed_fancy";
    constexpr const char* BEDROLL            = "bedroll";
    
    // === FURNITURE: UTILITY ===
    constexpr const char* FORGE              = "forge";
    constexpr const char* OVEN               = "oven";
    constexpr const char* CAULDRON           = "cauldron";
    constexpr const char* WELL               = "well";
    constexpr const char* FOUNTAIN           = "fountain";
    constexpr const char* ALTAR              = "altar";
    
    // === DECORATION: LIGHTING ===
    constexpr const char* TORCH              = "torch";
    constexpr const char* LANTERN            = "lantern";
    constexpr const char* CANDLE             = "candle";
    constexpr const char* BRAZIER            = "brazier";
    constexpr const char* CAMPFIRE           = "campfire";
    constexpr const char* FIREPLACE          = "fireplace";
    
    // === DECORATION: ITEMS ===
    constexpr const char* POT                = "pot";
    constexpr const char* VASE               = "vase";
    constexpr const char* RUG                = "rug";
    constexpr const char* PAINTING           = "painting";
    constexpr const char* STATUE             = "statue";
    constexpr const char* GRAVE              = "grave";
    constexpr const char* TOMBSTONE          = "tombstone";
    constexpr const char* BONES              = "bones";
    constexpr const char* SKULL              = "skull";
    constexpr const char* COBWEB             = "cobweb";
    
    // === DECORATION: NATURE ===
    constexpr const char* LOG                = "log";
    constexpr const char* DRIFTWOOD          = "driftwood";
    constexpr const char* SHELL              = "shell";
    constexpr const char* SEAWEED            = "seaweed";
    constexpr const char* MOSS               = "moss";
    
    // === PATHWAYS ===
    constexpr const char* ROAD_DIRT          = "road_dirt";
    constexpr const char* ROAD_COBBLE        = "road_cobble";
    constexpr const char* ROAD_STONE         = "road_stone";
    constexpr const char* PATH_GRAVEL        = "path_gravel";
    constexpr const char* BRIDGE_WOOD        = "bridge_wood";
    constexpr const char* BRIDGE_STONE       = "bridge_stone";
    constexpr const char* STAIRS_UP          = "stairs_up";
    constexpr const char* STAIRS_DOWN        = "stairs_down";
    
    // === SPECIAL: DUNGEON ===
    constexpr const char* DUNGEON_FLOOR      = "dungeon_floor";
    constexpr const char* DUNGEON_WALL       = "dungeon_wall";
    constexpr const char* DUNGEON_DOOR       = "dungeon_door";
    constexpr const char* TRAP               = "trap";
    constexpr const char* TRAP_HIDDEN        = "trap_hidden";
    constexpr const char* SPIKE              = "spike";
    constexpr const char* PIT                = "pit";
    
    // === SPECIAL: MAGICAL ===
    constexpr const char* PORTAL             = "portal";
    constexpr const char* RUNE               = "rune";
    constexpr const char* MAGIC_CIRCLE       = "magic_circle";
    constexpr const char* CRYSTAL            = "crystal";
    constexpr const char* CORRUPTED          = "corrupted";
    
    // === INTERACTIVE ===
    constexpr const char* LEVER              = "lever";
    constexpr const char* BUTTON             = "button";
    constexpr const char* SIGN               = "sign";
    constexpr const char* NOTICE_BOARD       = "notice_board";
}

class Tileset {
private:
    std::unordered_map<std::string, TileDef> tiles;
    bool initialized = false;
    
public:
    Tileset() {
        initialize();
    }
    
    // Get a tile definition by ID
    const TileDef& get(const std::string& id) const {
        auto it = tiles.find(id);
        if (it != tiles.end()) return it->second;
        
        static TileDef unknown('?', "magenta", true, "unknown", "Missing tile definition");
        return unknown;
    }
    
    // Check if a tile exists
    bool has(const std::string& id) const {
        return tiles.find(id) != tiles.end();
    }
    
    // Get all tiles in a category
    std::vector<const TileDef*> get_by_category(TileCategory cat) const {
        std::vector<const TileDef*> result;
        for (const auto& pair : tiles) {
            if (pair.second.category == cat) {
                result.push_back(&pair.second);
            }
        }
        return result;
    }
    
    // Register a custom tile
    void register_tile(const std::string& id, const TileDef& tile) {
        tiles[id] = tile;
    }
    
private:
    void initialize() {
        if (initialized) return;
        initialized = true;
        
        // =====================================================================
        // TERRAIN: NATURAL GROUND
        // =====================================================================
        tiles[TileID::GRASS] = TileDef('.', "green", true, "Grass", 
            "Soft green grass", TileCategory::TERRAIN_NATURAL);
        tiles[TileID::GRASS_TALL] = TileDef('"', "green", true, "Tall Grass",
            "Knee-high grass that sways in the wind", TileCategory::TERRAIN_NATURAL)
            .movement(0.9f);
        tiles[TileID::GRASS_SHORT] = TileDef(',', "green", true, "Short Grass",
            "Well-trimmed grass", TileCategory::TERRAIN_NATURAL);
        tiles[TileID::DIRT] = TileDef('.', "brown", true, "Dirt",
            "Packed earth", TileCategory::TERRAIN_NATURAL);
        tiles[TileID::DIRT_PATH] = TileDef(':', "brown", true, "Dirt Path",
            "A worn path through the earth", TileCategory::TERRAIN_NATURAL);
        tiles[TileID::MUD] = TileDef(',', "brown", true, "Mud",
            "Wet, squelching mud", TileCategory::TERRAIN_NATURAL)
            .movement(0.6f);
        tiles[TileID::SAND] = TileDef('.', "yellow", true, "Sand",
            "Fine golden sand", TileCategory::TERRAIN_NATURAL)
            .movement(0.9f);
        tiles[TileID::SAND_FINE] = TileDef(',', "yellow", true, "Fine Sand",
            "Soft powdery sand", TileCategory::TERRAIN_NATURAL)
            .movement(0.85f);
        tiles[TileID::ASH] = TileDef('.', "dark_gray", true, "Ash",
            "Grey volcanic ash", TileCategory::TERRAIN_NATURAL);
        tiles[TileID::MYCELIUM] = TileDef(',', "magenta", true, "Mycelium",
            "Fungal growth covers the ground", TileCategory::TERRAIN_NATURAL);
        tiles[TileID::DEAD_GROUND] = TileDef('.', "dark_gray", true, "Dead Earth",
            "Nothing grows here", TileCategory::TERRAIN_NATURAL);
        
        // =====================================================================
        // TERRAIN: WATER
        // =====================================================================
        tiles[TileID::WATER] = TileDef('~', "blue", false, "Water",
            "Clear water", TileCategory::TERRAIN_WATER)
            .liquid();
        tiles[TileID::WATER_DEEP] = TileDef('~', "dark_blue", false, "Deep Water",
            "Dark, deep water", TileCategory::TERRAIN_WATER)
            .liquid().visibility(0.5f);
        tiles[TileID::WATER_SHALLOW] = TileDef('=', "cyan", true, "Shallow Water",
            "Ankle-deep water you can wade through", TileCategory::TERRAIN_WATER)
            .movement(0.7f).liquid();
        tiles[TileID::WATER_MURKY] = TileDef('~', "dark_green", false, "Murky Water",
            "Dark, stagnant water", TileCategory::TERRAIN_WATER)
            .liquid().visibility(0.3f);
        tiles[TileID::WAVE] = TileDef('~', "cyan", false, "Wave",
            "Rolling waves", TileCategory::TERRAIN_WATER)
            .liquid();
        tiles[TileID::LILY_PADS] = TileDef('"', "green", true, "Lily Pads",
            "Floating lily pads", TileCategory::TERRAIN_WATER)
            .movement(0.5f);
        tiles[TileID::ICE] = TileDef('.', "cyan", true, "Ice",
            "Slippery ice", TileCategory::TERRAIN_WATER)
            .movement(1.3f);  // Faster but slippery
        tiles[TileID::LAVA] = TileDef('~', "red", false, "Lava",
            "Molten rock", TileCategory::TERRAIN_WATER)
            .liquid().dangerous();
        tiles[TileID::LAVA_COOLING] = TileDef('~', "yellow", false, "Cooling Lava",
            "Lava beginning to cool", TileCategory::TERRAIN_WATER)
            .liquid().dangerous();
        
        // =====================================================================
        // TERRAIN: ROCK
        // =====================================================================
        tiles[TileID::STONE] = TileDef('.', "gray", true, "Stone",
            "Solid stone ground", TileCategory::TERRAIN_ROCK);
        tiles[TileID::STONE_DARK] = TileDef('.', "dark_gray", true, "Dark Stone",
            "Dark volcanic stone", TileCategory::TERRAIN_ROCK);
        tiles[TileID::ROCK] = TileDef('o', "gray", true, "Rock",
            "A small rock", TileCategory::TERRAIN_ROCK);
        tiles[TileID::BOULDER] = TileDef('O', "gray", false, "Boulder",
            "A large boulder", TileCategory::TERRAIN_ROCK)
            .blocking();
        tiles[TileID::CLIFF] = TileDef('^', "gray", false, "Cliff",
            "A steep cliff face", TileCategory::TERRAIN_ROCK)
            .blocking();
        tiles[TileID::MOUNTAIN] = TileDef('M', "gray", false, "Mountain",
            "Impassable mountain terrain", TileCategory::TERRAIN_ROCK)
            .blocking();
        tiles[TileID::MOUNTAIN_PEAK] = TileDef('A', "white", false, "Mountain Peak",
            "A snow-capped peak", TileCategory::TERRAIN_ROCK)
            .blocking();
        tiles[TileID::VOLCANIC_ROCK] = TileDef('^', "dark_gray", true, "Volcanic Rock",
            "Jagged volcanic rock", TileCategory::TERRAIN_ROCK)
            .movement(0.7f);
        
        // =====================================================================
        // TERRAIN: SNOW & ICE
        // =====================================================================
        tiles[TileID::SNOW] = TileDef('.', "white", true, "Snow",
            "Fresh white snow", TileCategory::TERRAIN_SNOW)
            .movement(0.8f);
        tiles[TileID::SNOW_DEEP] = TileDef(',', "white", true, "Deep Snow",
            "Waist-deep snow", TileCategory::TERRAIN_SNOW)
            .movement(0.5f);
        tiles[TileID::FROST] = TileDef(',', "cyan", true, "Frost",
            "Icy frost", TileCategory::TERRAIN_SNOW);
        tiles[TileID::ICE_BLOCK] = TileDef('o', "cyan", false, "Ice Block",
            "A solid block of ice", TileCategory::TERRAIN_SNOW)
            .blocking();
        tiles[TileID::ICICLE] = TileDef('*', "white", true, "Icicle",
            "Hanging icicles", TileCategory::TERRAIN_SNOW);
        
        // =====================================================================
        // VEGETATION: TREES
        // =====================================================================
        tiles[TileID::TREE] = TileDef('T', "green", false, "Tree",
            "A leafy tree", TileCategory::VEGETATION)
            .blocking();
        tiles[TileID::TREE_PINE] = TileDef('T', "dark_green", false, "Pine Tree",
            "A tall pine tree", TileCategory::VEGETATION)
            .blocking();
        tiles[TileID::TREE_OAK] = TileDef('T', "green", false, "Oak Tree",
            "A mighty oak", TileCategory::VEGETATION)
            .blocking();
        tiles[TileID::TREE_PALM] = TileDef('T', "green", false, "Palm Tree",
            "A tropical palm tree", TileCategory::VEGETATION)
            .blocking();
        tiles[TileID::TREE_DEAD] = TileDef('t', "brown", true, "Dead Tree",
            "A leafless dead tree", TileCategory::VEGETATION);
        tiles[TileID::TREE_ANCIENT] = TileDef('T', "dark_green", false, "Ancient Tree",
            "A massive ancient tree", TileCategory::VEGETATION)
            .blocking();
        tiles[TileID::SAPLING] = TileDef('t', "green", true, "Sapling",
            "A young tree", TileCategory::VEGETATION);
        tiles[TileID::STUMP] = TileDef('o', "brown", true, "Stump",
            "A tree stump", TileCategory::VEGETATION);
        
        // =====================================================================
        // VEGETATION: PLANTS
        // =====================================================================
        tiles[TileID::BUSH] = TileDef('#', "dark_green", true, "Bush",
            "A dense bush", TileCategory::VEGETATION)
            .movement(0.7f);
        tiles[TileID::BUSH_BERRY] = TileDef('#', "red", true, "Berry Bush",
            "A bush with ripe berries", TileCategory::VEGETATION)
            .movement(0.7f);
        tiles[TileID::FERN] = TileDef('"', "green", true, "Fern",
            "Leafy ferns", TileCategory::VEGETATION);
        tiles[TileID::REEDS] = TileDef('"', "dark_green", true, "Reeds",
            "Tall reeds", TileCategory::VEGETATION)
            .movement(0.8f);
        tiles[TileID::VINES] = TileDef('"', "green", true, "Vines",
            "Hanging vines", TileCategory::VEGETATION);
        tiles[TileID::UNDERBRUSH] = TileDef('"', "dark_green", true, "Underbrush",
            "Thick underbrush", TileCategory::VEGETATION)
            .movement(0.6f).visibility(0.8f);
        tiles[TileID::THICKET] = TileDef('#', "dark_green", true, "Thicket",
            "A dense thicket", TileCategory::VEGETATION)
            .movement(0.5f).visibility(0.6f);
        tiles[TileID::CACTUS] = TileDef('#', "green", false, "Cactus",
            "A prickly cactus", TileCategory::VEGETATION)
            .dangerous();
        
        // =====================================================================
        // VEGETATION: MUSHROOMS
        // =====================================================================
        tiles[TileID::MUSHROOM] = TileDef('*', "red", true, "Mushroom",
            "A red-capped mushroom", TileCategory::VEGETATION);
        tiles[TileID::MUSHROOM_GLOW] = TileDef('*', "cyan", true, "Glowing Mushroom",
            "A softly glowing mushroom", TileCategory::VEGETATION);
        tiles[TileID::MUSHROOM_GIANT] = TileDef('T', "magenta", false, "Giant Mushroom",
            "A towering mushroom", TileCategory::VEGETATION)
            .blocking();
        tiles[TileID::PUFFBALL] = TileDef('o', "yellow", true, "Puffball",
            "A round puffball mushroom", TileCategory::VEGETATION);
        
        // =====================================================================
        // VEGETATION: FLOWERS
        // =====================================================================
        tiles[TileID::FLOWER_WILD] = TileDef('*', "yellow", true, "Wildflower",
            "A pretty wildflower", TileCategory::VEGETATION);
        tiles[TileID::FLOWER_RED] = TileDef('*', "red", true, "Red Flower",
            "A bright red flower", TileCategory::VEGETATION);
        tiles[TileID::FLOWER_BLUE] = TileDef('*', "blue", true, "Blue Flower",
            "A delicate blue flower", TileCategory::VEGETATION);
        tiles[TileID::FLOWER_YELLOW] = TileDef('*', "yellow", true, "Yellow Flower",
            "A sunny yellow flower", TileCategory::VEGETATION);
        tiles[TileID::FLOWER_ORCHID] = TileDef('*', "magenta", true, "Orchid",
            "An exotic orchid", TileCategory::VEGETATION);
        tiles[TileID::FLOWER_DESERT] = TileDef('*', "yellow", true, "Desert Flower",
            "A hardy desert flower", TileCategory::VEGETATION);
        
        // =====================================================================
        // STRUCTURE: WALLS
        // =====================================================================
        tiles[TileID::WALL_WOOD] = TileDef('#', "brown", false, "Wooden Wall",
            "A wooden wall", TileCategory::STRUCTURE_WALL)
            .blocking();
        tiles[TileID::WALL_STONE] = TileDef('#', "gray", false, "Stone Wall",
            "A solid stone wall", TileCategory::STRUCTURE_WALL)
            .blocking();
        tiles[TileID::WALL_BRICK] = TileDef('#', "red", false, "Brick Wall",
            "A red brick wall", TileCategory::STRUCTURE_WALL)
            .blocking();
        tiles[TileID::WALL_CASTLE] = TileDef('#', "dark_gray", false, "Castle Wall",
            "A fortified castle wall", TileCategory::STRUCTURE_WALL)
            .blocking();
        tiles[TileID::WALL_RUINED] = TileDef('%', "gray", true, "Ruined Wall",
            "A crumbling wall section", TileCategory::STRUCTURE_WALL);
        tiles[TileID::PILLAR] = TileDef('O', "gray", false, "Pillar",
            "A stone pillar", TileCategory::STRUCTURE_WALL)
            .blocking();
        tiles[TileID::FENCE] = TileDef('|', "brown", false, "Fence",
            "A wooden fence", TileCategory::STRUCTURE_WALL)
            .blocking();
        tiles[TileID::FENCE_GATE] = TileDef('+', "brown", true, "Fence Gate",
            "A gate in the fence", TileCategory::STRUCTURE_WALL);
        
        // =====================================================================
        // STRUCTURE: FLOORS
        // =====================================================================
        tiles[TileID::FLOOR_WOOD] = TileDef('.', "brown", true, "Wooden Floor",
            "Polished wooden planks", TileCategory::STRUCTURE_FLOOR);
        tiles[TileID::FLOOR_STONE] = TileDef('.', "gray", true, "Stone Floor",
            "Cold stone tiles", TileCategory::STRUCTURE_FLOOR);
        tiles[TileID::FLOOR_TILE] = TileDef('.', "white", true, "Tile Floor",
            "Clean white tiles", TileCategory::STRUCTURE_FLOOR);
        tiles[TileID::FLOOR_CARPET] = TileDef('=', "red", true, "Carpet",
            "A warm carpet", TileCategory::STRUCTURE_FLOOR);
        tiles[TileID::FLOOR_CARPET_FANCY] = TileDef('=', "magenta", true, "Fancy Carpet",
            "An ornate carpet", TileCategory::STRUCTURE_FLOOR);
        tiles[TileID::FLOOR_DIRT] = TileDef('.', "brown", true, "Dirt Floor",
            "Packed dirt floor", TileCategory::STRUCTURE_FLOOR);
        
        // =====================================================================
        // STRUCTURE: DOORS & WINDOWS
        // =====================================================================
        tiles[TileID::DOOR_WOOD] = TileDef('+', "yellow", true, "Wooden Door",
            "A sturdy wooden door", TileCategory::STRUCTURE_DOOR);
        tiles[TileID::DOOR_IRON] = TileDef('+', "gray", true, "Iron Door",
            "A heavy iron door", TileCategory::STRUCTURE_DOOR);
        tiles[TileID::DOOR_ORNATE] = TileDef('+', "yellow", true, "Ornate Door",
            "A beautifully carved door", TileCategory::STRUCTURE_DOOR);
        tiles[TileID::WINDOW] = TileDef('"', "cyan", false, "Window",
            "A glass window", TileCategory::STRUCTURE_WINDOW);
        tiles[TileID::WINDOW_BARRED] = TileDef('#', "gray", false, "Barred Window",
            "A window with iron bars", TileCategory::STRUCTURE_WINDOW)
            .blocking();
        
        // =====================================================================
        // STRUCTURE: ROOFS
        // =====================================================================
        tiles[TileID::ROOF_THATCH] = TileDef('^', "yellow", false, "Thatch Roof",
            "A thatched roof", TileCategory::STRUCTURE_WALL)
            .blocking();
        tiles[TileID::ROOF_TILE] = TileDef('^', "red", false, "Tile Roof",
            "Clay roof tiles", TileCategory::STRUCTURE_WALL)
            .blocking();
        tiles[TileID::CHIMNEY] = TileDef('~', "red", false, "Chimney",
            "A smoking chimney", TileCategory::STRUCTURE_WALL)
            .blocking();
        
        // =====================================================================
        // FURNITURE: SEATING
        // =====================================================================
        tiles[TileID::CHAIR] = TileDef('h', "brown", true, "Chair",
            "A wooden chair", TileCategory::FURNITURE);
        tiles[TileID::BENCH] = TileDef('=', "brown", true, "Bench",
            "A wooden bench", TileCategory::FURNITURE);
        tiles[TileID::THRONE] = TileDef('H', "yellow", true, "Throne",
            "An ornate throne", TileCategory::FURNITURE);
        tiles[TileID::STOOL] = TileDef('o', "brown", true, "Stool",
            "A small stool", TileCategory::FURNITURE);
        
        // =====================================================================
        // FURNITURE: TABLES
        // =====================================================================
        tiles[TileID::TABLE] = TileDef('T', "brown", true, "Table",
            "A wooden table", TileCategory::FURNITURE);
        tiles[TileID::COUNTER] = TileDef('C', "brown", false, "Counter",
            "A serving counter", TileCategory::FURNITURE)
            .blocking();
        tiles[TileID::WORKBENCH] = TileDef('W', "brown", false, "Workbench",
            "A craftsman's workbench", TileCategory::FURNITURE)
            .blocking();
        tiles[TileID::ANVIL] = TileDef('A', "gray", false, "Anvil",
            "A blacksmith's anvil", TileCategory::FURNITURE)
            .blocking();
        
        // =====================================================================
        // FURNITURE: STORAGE
        // =====================================================================
        tiles[TileID::CHEST] = TileDef('=', "brown", true, "Chest",
            "A wooden chest", TileCategory::FURNITURE);
        tiles[TileID::CHEST_LOCKED] = TileDef('=', "yellow", true, "Locked Chest",
            "A locked chest", TileCategory::FURNITURE);
        tiles[TileID::BARREL] = TileDef('o', "brown", false, "Barrel",
            "A wooden barrel", TileCategory::FURNITURE)
            .blocking();
        tiles[TileID::CRATE] = TileDef('#', "yellow", false, "Crate",
            "A wooden crate", TileCategory::FURNITURE)
            .blocking();
        tiles[TileID::SHELF] = TileDef('S', "brown", false, "Shelf",
            "A wall shelf", TileCategory::FURNITURE)
            .blocking();
        tiles[TileID::BOOKSHELF] = TileDef('B', "brown", false, "Bookshelf",
            "A bookshelf full of books", TileCategory::FURNITURE)
            .blocking();
        tiles[TileID::CABINET] = TileDef('C', "brown", false, "Cabinet",
            "A storage cabinet", TileCategory::FURNITURE)
            .blocking();
        
        // =====================================================================
        // FURNITURE: BEDS
        // =====================================================================
        tiles[TileID::BED] = TileDef('=', "blue", true, "Bed",
            "A comfortable bed", TileCategory::FURNITURE);
        tiles[TileID::BED_FANCY] = TileDef('=', "magenta", true, "Fancy Bed",
            "A luxurious bed", TileCategory::FURNITURE);
        tiles[TileID::BEDROLL] = TileDef('=', "green", true, "Bedroll",
            "A simple bedroll", TileCategory::FURNITURE);
        
        // =====================================================================
        // FURNITURE: UTILITY
        // =====================================================================
        tiles[TileID::FORGE] = TileDef('F', "red", false, "Forge",
            "A blacksmith's forge", TileCategory::FURNITURE)
            .blocking();
        tiles[TileID::OVEN] = TileDef('O', "red", false, "Oven",
            "A cooking oven", TileCategory::FURNITURE)
            .blocking();
        tiles[TileID::CAULDRON] = TileDef('U', "dark_gray", false, "Cauldron",
            "A bubbling cauldron", TileCategory::FURNITURE)
            .blocking();
        tiles[TileID::WELL] = TileDef('O', "blue", false, "Well",
            "A stone well", TileCategory::FURNITURE)
            .blocking();
        tiles[TileID::FOUNTAIN] = TileDef('O', "cyan", false, "Fountain",
            "An ornate fountain", TileCategory::FURNITURE)
            .blocking();
        tiles[TileID::ALTAR] = TileDef('I', "white", false, "Altar",
            "A sacred altar", TileCategory::FURNITURE)
            .blocking();
        
        // =====================================================================
        // DECORATION: LIGHTING
        // =====================================================================
        tiles[TileID::TORCH] = TileDef('*', "yellow", true, "Torch",
            "A flickering torch", TileCategory::DECORATION);
        tiles[TileID::LANTERN] = TileDef('*', "yellow", true, "Lantern",
            "A hanging lantern", TileCategory::DECORATION);
        tiles[TileID::CANDLE] = TileDef('*', "yellow", true, "Candle",
            "A small candle", TileCategory::DECORATION);
        tiles[TileID::BRAZIER] = TileDef('o', "red", false, "Brazier",
            "A burning brazier", TileCategory::DECORATION)
            .blocking();
        tiles[TileID::CAMPFIRE] = TileDef('*', "red", false, "Campfire",
            "A crackling campfire", TileCategory::DECORATION)
            .dangerous();
        tiles[TileID::FIREPLACE] = TileDef('~', "red", false, "Fireplace",
            "A warm fireplace", TileCategory::DECORATION)
            .blocking();
        
        // =====================================================================
        // DECORATION: ITEMS
        // =====================================================================
        tiles[TileID::POT] = TileDef('o', "brown", true, "Pot",
            "A clay pot", TileCategory::DECORATION);
        tiles[TileID::VASE] = TileDef('o', "cyan", true, "Vase",
            "A decorative vase", TileCategory::DECORATION);
        tiles[TileID::RUG] = TileDef('=', "red", true, "Rug",
            "A woven rug", TileCategory::DECORATION);
        tiles[TileID::PAINTING] = TileDef('p', "brown", true, "Painting",
            "A framed painting", TileCategory::DECORATION);
        tiles[TileID::STATUE] = TileDef('S', "gray", false, "Statue",
            "A stone statue", TileCategory::DECORATION)
            .blocking();
        tiles[TileID::GRAVE] = TileDef('+', "gray", true, "Grave",
            "A simple grave marker", TileCategory::DECORATION);
        tiles[TileID::TOMBSTONE] = TileDef('I', "gray", false, "Tombstone",
            "A weathered tombstone", TileCategory::DECORATION)
            .blocking();
        tiles[TileID::BONES] = TileDef('x', "white", true, "Bones",
            "Scattered bones", TileCategory::DECORATION);
        tiles[TileID::SKULL] = TileDef('o', "white", true, "Skull",
            "A bleached skull", TileCategory::DECORATION);
        tiles[TileID::COBWEB] = TileDef('%', "white", true, "Cobweb",
            "Dusty cobwebs", TileCategory::DECORATION);
        
        // =====================================================================
        // DECORATION: NATURE
        // =====================================================================
        tiles[TileID::LOG] = TileDef('=', "brown", true, "Fallen Log",
            "A fallen tree trunk", TileCategory::DECORATION);
        tiles[TileID::DRIFTWOOD] = TileDef('=', "brown", true, "Driftwood",
            "Sun-bleached driftwood", TileCategory::DECORATION);
        tiles[TileID::SHELL] = TileDef('o', "white", true, "Shell",
            "A sea shell", TileCategory::DECORATION);
        tiles[TileID::SEAWEED] = TileDef('"', "green", true, "Seaweed",
            "Tangled seaweed", TileCategory::DECORATION);
        tiles[TileID::MOSS] = TileDef(',', "green", true, "Moss",
            "Soft moss", TileCategory::DECORATION);
        
        // =====================================================================
        // PATHWAYS
        // =====================================================================
        tiles[TileID::ROAD_DIRT] = TileDef(':', "brown", true, "Dirt Road",
            "A well-worn dirt road", TileCategory::PATHWAY)
            .movement(1.2f);
        tiles[TileID::ROAD_COBBLE] = TileDef('=', "gray", true, "Cobblestone Road",
            "A paved cobblestone road", TileCategory::PATHWAY)
            .movement(1.3f);
        tiles[TileID::ROAD_STONE] = TileDef('.', "gray", true, "Stone Road",
            "A smooth stone road", TileCategory::PATHWAY)
            .movement(1.3f);
        tiles[TileID::PATH_GRAVEL] = TileDef(',', "gray", true, "Gravel Path",
            "A crunchy gravel path", TileCategory::PATHWAY)
            .movement(1.1f);
        tiles[TileID::BRIDGE_WOOD] = TileDef('=', "brown", true, "Wooden Bridge",
            "A sturdy wooden bridge", TileCategory::PATHWAY);
        tiles[TileID::BRIDGE_STONE] = TileDef('=', "gray", true, "Stone Bridge",
            "A solid stone bridge", TileCategory::PATHWAY);
        tiles[TileID::STAIRS_UP] = TileDef('<', "gray", true, "Stairs Up",
            "Stairs leading up", TileCategory::PATHWAY);
        tiles[TileID::STAIRS_DOWN] = TileDef('>', "gray", true, "Stairs Down",
            "Stairs leading down", TileCategory::PATHWAY);
        
        // =====================================================================
        // SPECIAL: DUNGEON
        // =====================================================================
        tiles[TileID::DUNGEON_FLOOR] = TileDef('.', "dark_gray", true, "Dungeon Floor",
            "Cold stone dungeon floor", TileCategory::SPECIAL);
        tiles[TileID::DUNGEON_WALL] = TileDef('#', "dark_gray", false, "Dungeon Wall",
            "Damp dungeon wall", TileCategory::SPECIAL)
            .blocking();
        tiles[TileID::DUNGEON_DOOR] = TileDef('+', "brown", true, "Dungeon Door",
            "A heavy wooden door", TileCategory::SPECIAL);
        tiles[TileID::TRAP] = TileDef('^', "red", true, "Trap",
            "A visible trap", TileCategory::SPECIAL)
            .dangerous();
        tiles[TileID::TRAP_HIDDEN] = TileDef('.', "dark_gray", true, "Hidden Trap",
            "A concealed trap", TileCategory::SPECIAL)
            .dangerous();
        tiles[TileID::SPIKE] = TileDef('^', "gray", false, "Spikes",
            "Sharp spikes", TileCategory::SPECIAL)
            .dangerous().blocking();
        tiles[TileID::PIT] = TileDef(' ', "black", false, "Pit",
            "A dark pit", TileCategory::SPECIAL)
            .dangerous();
        
        // =====================================================================
        // SPECIAL: MAGICAL
        // =====================================================================
        tiles[TileID::PORTAL] = TileDef('O', "magenta", true, "Portal",
            "A shimmering portal", TileCategory::SPECIAL);
        tiles[TileID::RUNE] = TileDef('*', "cyan", true, "Rune",
            "A glowing rune", TileCategory::SPECIAL);
        tiles[TileID::MAGIC_CIRCLE] = TileDef('o', "magenta", true, "Magic Circle",
            "An arcane circle", TileCategory::SPECIAL);
        tiles[TileID::CRYSTAL] = TileDef('*', "cyan", false, "Crystal",
            "A glowing crystal", TileCategory::SPECIAL)
            .blocking();
        tiles[TileID::CORRUPTED] = TileDef('~', "magenta", true, "Corruption",
            "Corrupted ground", TileCategory::SPECIAL)
            .dangerous().movement(0.7f);
        
        // =====================================================================
        // INTERACTIVE
        // =====================================================================
        tiles[TileID::LEVER] = TileDef('/', "gray", true, "Lever",
            "A mechanical lever", TileCategory::INTERACTIVE);
        tiles[TileID::BUTTON] = TileDef('o', "gray", true, "Button",
            "A pressure button", TileCategory::INTERACTIVE);
        tiles[TileID::SIGN] = TileDef('I', "brown", true, "Sign",
            "A wooden sign", TileCategory::INTERACTIVE);
        tiles[TileID::NOTICE_BOARD] = TileDef('#', "brown", false, "Notice Board",
            "A community notice board", TileCategory::INTERACTIVE)
            .blocking();
    }
};

// Global tileset instance
inline Tileset& get_tileset() {
    static Tileset tileset;
    return tileset;
}

// ============================================================================
// HELPER FUNCTIONS - Quick access to tile properties
// ============================================================================
namespace TileUtils {
    // Create a Tile struct from a TileDef (for compatibility with existing code)
    // Note: Tile constructor is (walkable, symbol, color)
    inline Tile from_def(const TileDef& def) {
        return Tile(def.walkable, def.symbol, def.color);
    }
    
    // Get a Tile from a TileID
    inline Tile get_tile(const std::string& id) {
        const TileDef& def = get_tileset().get(id);
        return Tile(def.walkable, def.symbol, def.color);
    }
    
    // Quick access for common tiles
    inline Tile grass() { return get_tile(TileID::GRASS); }
    inline Tile dirt() { return get_tile(TileID::DIRT); }
    inline Tile water() { return get_tile(TileID::WATER); }
    inline Tile sand() { return get_tile(TileID::SAND); }
    inline Tile stone() { return get_tile(TileID::STONE); }
    inline Tile tree() { return get_tile(TileID::TREE); }
    inline Tile wall_wood() { return get_tile(TileID::WALL_WOOD); }
    inline Tile wall_stone() { return get_tile(TileID::WALL_STONE); }
    inline Tile floor_wood() { return get_tile(TileID::FLOOR_WOOD); }
    inline Tile floor_stone() { return get_tile(TileID::FLOOR_STONE); }
    inline Tile door_wood() { return get_tile(TileID::DOOR_WOOD); }
    inline Tile road_dirt() { return get_tile(TileID::ROAD_DIRT); }
}
