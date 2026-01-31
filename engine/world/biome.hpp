#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <random>

enum class BiomeType {
    OCEAN = 0,
    DEEP_OCEAN,
    BEACH,
    PLAINS,
    FOREST,
    DENSE_FOREST,
    HILLS,
    MOUNTAINS,
    SNOW_PEAKS,
    DESERT,
    DESERT_DUNES,
    SWAMP,
    TUNDRA,
    FROZEN_TUNDRA,
    SAVANNA,
    JUNGLE,
    TROPICAL_BEACH,
    VOLCANIC,
    MUSHROOM_FOREST,
    DEAD_LANDS,
    RIVER,
    LAKE,
    ROAD,
    COUNT
};

struct TileVariant {
    char symbol;
    std::string color;
    bool walkable;
    float weight;         // Probability weight (higher = more common)
    std::string name;     // For debugging/tooltips
    float movement_cost;  // 1.0 = normal, 0.5 = 2 turns to cross
    
    TileVariant(char sym = '.', const std::string& col = "white", bool walk = true, 
                float w = 1.0f, const std::string& n = "ground", float move_cost = 1.0f)
        : symbol(sym), color(col), walkable(walk), weight(w), name(n), movement_cost(move_cost) {}
};

struct BiomeFeature {
    std::string name;
    char symbol;
    std::string color;
    bool walkable;
    float spawn_chance;   // 0.0 - 1.0
    float cluster_chance; // Chance to spawn adjacent to another of same type
    int min_cluster_size;
    int max_cluster_size;
    
    BiomeFeature(const std::string& n = "", char sym = '?', const std::string& col = "white",
                 bool walk = true, float spawn = 0.01f, float cluster = 0.0f,
                 int min_c = 1, int max_c = 1)
        : name(n), symbol(sym), color(col), walkable(walk), 
          spawn_chance(spawn), cluster_chance(cluster),
          min_cluster_size(min_c), max_cluster_size(max_c) {}
};

struct BiomeData {
    BiomeType type;
    std::string name;
    std::string description;
    
    // Base terrain tiles (weighted random selection)
    std::vector<TileVariant> base_tiles;
    
    // Transition tiles (used when blending with other biomes)
    std::vector<TileVariant> edge_tiles;
    
    // Features that can spawn
    std::vector<BiomeFeature> features;
    
    // Noise thresholds for micro-detail
    float tree_threshold = 0.7f;       // Above this, spawn trees
    float rock_threshold = 0.1f;       // Below this, spawn rocks
    float water_threshold = 0.05f;     // Below this, spawn water features
    float flower_threshold = 0.92f;    // Above this, spawn flowers/decorations
    
    // Movement/gameplay modifiers
    float movement_cost = 1.0f;        // Multiplier for movement speed
    float visibility_range = 1.0f;     // Multiplier for view distance
    bool allows_structures = true;     // Can structures spawn here?
    
    // Ambient description for atmosphere
    std::vector<std::string> ambient_descriptions;
    
    BiomeData() : type(BiomeType::PLAINS), name("Unknown"), description("") {}
    
    BiomeData(BiomeType t, const std::string& n, const std::string& desc = "")
        : type(t), name(n), description(desc) {}
    
    // Add a base tile variant
    BiomeData& add_tile(char sym, const std::string& col, bool walk, float weight, 
                        const std::string& name = "ground", float move_cost = 1.0f) {
        base_tiles.emplace_back(sym, col, walk, weight, name, move_cost);
        return *this;
    }
    
    // Add an edge tile variant
    BiomeData& add_edge_tile(char sym, const std::string& col, bool walk, float weight) {
        edge_tiles.emplace_back(sym, col, walk, weight);
        return *this;
    }
    
    // Add a feature
    BiomeData& add_feature(const std::string& name, char sym, const std::string& col,
                           bool walk, float spawn, float cluster = 0.0f,
                           int min_c = 1, int max_c = 1) {
        features.emplace_back(name, sym, col, walk, spawn, cluster, min_c, max_c);
        return *this;
    }
    
    // Set thresholds
    BiomeData& set_thresholds(float tree, float rock, float water, float flower) {
        tree_threshold = tree;
        rock_threshold = rock;
        water_threshold = water;
        flower_threshold = flower;
        return *this;
    }
    
    // Set modifiers
    BiomeData& set_modifiers(float move, float vis, bool structures = true) {
        movement_cost = move;
        visibility_range = vis;
        allows_structures = structures;
        return *this;
    }
    
    // Add ambient description
    BiomeData& add_ambient(const std::string& desc) {
        ambient_descriptions.push_back(desc);
        return *this;
    }
    
    // Select a random base tile based on weights
    const TileVariant& select_tile(std::mt19937& rng) const {
        if (base_tiles.empty()) {
            static TileVariant default_tile('.', "white", true, 1.0f, "void");
            return default_tile;
        }
        
        float total_weight = 0.0f;
        for (const auto& t : base_tiles) total_weight += t.weight;
        
        std::uniform_real_distribution<float> dist(0.0f, total_weight);
        float roll = dist(rng);
        
        float cumulative = 0.0f;
        for (const auto& t : base_tiles) {
            cumulative += t.weight;
            if (roll <= cumulative) return t;
        }
        
        return base_tiles.back();
    }
};

class BiomeRegistry {
private:
    std::unordered_map<BiomeType, BiomeData> biomes;
    bool initialized = false;
    bool loaded_from_json = false;
    
public:
    BiomeRegistry() {
        initialize_default_biomes();
    }
    
    // Get biome data by type
    const BiomeData& get(BiomeType type) const {
        auto it = biomes.find(type);
        if (it != biomes.end()) return it->second;
        
        static BiomeData unknown(BiomeType::PLAINS, "Unknown");
        unknown.add_tile('?', "magenta", true, 1.0f, "error");
        return unknown;
    }
    
    // Register a custom biome
    void register_biome(const BiomeData& biome) {
        biomes[biome.type] = biome;
    }
    
    // Check if biome exists
    bool has_biome(BiomeType type) const {
        return biomes.find(type) != biomes.end();
    }
    
    // Load biomes from JSON (replaces defaults if successful)
    bool load_from_json(std::unordered_map<BiomeType, BiomeData>& loaded_biomes) {
        if (loaded_biomes.empty()) return false;
        biomes = std::move(loaded_biomes);
        loaded_from_json = true;
        return true;
    }
    
    // Check if loaded from JSON
    bool is_data_driven() const { return loaded_from_json; }
    
    // Get mutable access for loading
    std::unordered_map<BiomeType, BiomeData>& get_biomes() { return biomes; }
    
private:
    void initialize_default_biomes() {
        if (initialized) return;
        initialized = true;
        
        // ====== WATER BIOMES ======
        
        BiomeData ocean(BiomeType::OCEAN, "Ocean", "Vast open waters");
        ocean.add_tile('~', "blue", false, 5.0f, "water")
             .add_tile('~', "cyan", false, 1.0f, "wave")
             .add_tile('.', "dark_blue", false, 0.5f, "deep water")
             .set_modifiers(0.0f, 0.8f, false)
             .add_ambient("Waves lap against unseen shores.")
             .add_ambient("The salt spray fills the air.");
        biomes[BiomeType::OCEAN] = ocean;
        
        BiomeData deep_ocean(BiomeType::DEEP_OCEAN, "Deep Ocean", "Dark, mysterious depths");
        deep_ocean.add_tile('~', "dark_blue", false, 5.0f, "deep water")
                  .add_tile('.', "dark_blue", false, 2.0f, "abyss")
                  .add_tile('~', "blue", false, 1.0f, "current")
                  .set_modifiers(0.0f, 0.5f, false)
                  .add_ambient("The water is dark and foreboding.");
        biomes[BiomeType::DEEP_OCEAN] = deep_ocean;
        
        BiomeData river(BiomeType::RIVER, "River", "Flowing water");
        river.add_tile('~', "cyan", false, 4.0f, "flowing water")
             .add_tile('~', "blue", false, 2.0f, "deep current")
             .add_tile('=', "cyan", true, 0.5f, "shallow ford")
             .set_modifiers(0.3f, 1.0f, false);
        biomes[BiomeType::RIVER] = river;
        
        BiomeData lake(BiomeType::LAKE, "Lake", "Still, peaceful waters");
        lake.add_tile('~', "blue", false, 5.0f, "still water")
            .add_tile('~', "cyan", false, 1.0f, "ripple")
            .add_tile('"', "dark_green", true, 0.3f, "lily pads")
            .set_modifiers(0.0f, 1.0f, false);
        biomes[BiomeType::LAKE] = lake;
        
        // ====== COASTAL BIOMES ======
        
        BiomeData beach(BiomeType::BEACH, "Beach", "Sandy shores");
        beach.add_tile('.', "yellow", true, 5.0f, "sand")
             .add_tile(',', "yellow", true, 2.0f, "fine sand")
             .add_tile('o', "white", true, 0.3f, "shell")
             .add_tile('~', "cyan", true, 0.5f, "tide pool")
             .add_tile('"', "green", true, 0.2f, "seaweed")
             .add_feature("driftwood", '=', "brown", true, 0.02f)
             .add_feature("crab", 'c', "red", true, 0.005f)
             .set_thresholds(0.95f, 0.05f, 0.1f, 0.85f)
             .set_modifiers(0.9f, 1.2f, true)
             .add_ambient("Warm sand shifts beneath your feet.")
             .add_ambient("Seagulls cry in the distance.");
        biomes[BiomeType::BEACH] = beach;
        
        BiomeData tropical_beach(BiomeType::TROPICAL_BEACH, "Tropical Beach", "Paradise shores");
        tropical_beach.add_tile('.', "yellow", true, 4.0f, "white sand")
                      .add_tile(',', "white", true, 2.0f, "fine sand")
                      .add_tile('T', "green", false, 0.5f, "palm tree")
                      .add_tile('*', "magenta", true, 0.3f, "tropical flower")
                      .add_tile('o', "white", true, 0.2f, "coconut")
                      .set_modifiers(0.9f, 1.3f, true)
                      .add_ambient("Palm fronds sway in the warm breeze.");
        biomes[BiomeType::TROPICAL_BEACH] = tropical_beach;
        
        // ====== GRASSLAND BIOMES ======
        
        BiomeData plains(BiomeType::PLAINS, "Plains", "Open grasslands");
        plains.add_tile('.', "green", true, 6.0f, "grass")
              .add_tile(',', "green", true, 3.0f, "short grass")
              .add_tile('"', "green", true, 1.5f, "tall grass")
              .add_tile('"', "dark_green", true, 0.8f, "thick grass")
              .add_tile('*', "yellow", true, 0.4f, "wildflower")
              .add_tile('*', "magenta", true, 0.2f, "flower")
              .add_tile('*', "cyan", true, 0.1f, "blue flower")
              .add_tile('o', "gray", true, 0.15f, "stone")
              .add_feature("bush", '#', "dark_green", true, 0.01f, 0.3f, 1, 3)
              .add_feature("rabbit hole", 'o', "brown", true, 0.003f)
              .set_thresholds(0.92f, 0.05f, 0.02f, 0.88f)
              .set_modifiers(1.0f, 1.2f, true)
              .add_ambient("A gentle breeze ripples through the grass.")
              .add_ambient("Butterflies dance among the wildflowers.")
              .add_ambient("The open sky stretches endlessly above.");
        biomes[BiomeType::PLAINS] = plains;
        
        BiomeData savanna(BiomeType::SAVANNA, "Savanna", "Dry grasslands with scattered trees");
        savanna.add_tile(',', "yellow", true, 5.0f, "dry grass")
               .add_tile('.', "yellow", true, 3.0f, "dirt")
               .add_tile('"', "yellow", true, 1.5f, "tall dry grass")
               .add_tile('T', "brown", false, 0.4f, "acacia tree")
               .add_tile('t', "brown", true, 0.2f, "small acacia")
               .add_tile('o', "brown", true, 0.1f, "termite mound")
               .add_feature("watering hole", '~', "blue", false, 0.005f, 0.8f, 3, 8)
               .set_thresholds(0.88f, 0.08f, 0.03f, 0.85f)
               .set_modifiers(1.0f, 1.3f, true)
               .add_ambient("Heat shimmers on the horizon.")
               .add_ambient("A lone acacia provides precious shade.");
        biomes[BiomeType::SAVANNA] = savanna;
        
        // ====== FOREST BIOMES ======
        
        BiomeData forest(BiomeType::FOREST, "Forest", "Temperate woodland");
        forest.add_tile(',', "dark_green", true, 4.0f, "forest floor")
              .add_tile('.', "brown", true, 2.0f, "dirt path")
              .add_tile('"', "dark_green", true, 2.0f, "underbrush")
              .add_tile('t', "green", true, 1.0f, "sapling")
              .add_tile('T', "green", false, 2.5f, "tree")
              .add_tile('T', "dark_green", false, 1.5f, "oak tree")
              .add_tile('*', "red", true, 0.1f, "mushroom")
              .add_tile('*', "white", true, 0.05f, "white mushroom")
              .add_feature("berry bush", '#', "red", true, 0.01f, 0.2f, 1, 2)
              .add_feature("fallen log", '=', "brown", true, 0.008f)
              .add_feature("deer", 'd', "brown", true, 0.002f)
              .set_thresholds(0.55f, 0.08f, 0.03f, 0.90f)
              .set_modifiers(0.85f, 0.7f, true)
              .add_ambient("Sunlight filters through the canopy.")
              .add_ambient("Birds sing in the branches above.")
              .add_ambient("Leaves crunch softly underfoot.");
        biomes[BiomeType::FOREST] = forest;
        
        BiomeData dense_forest(BiomeType::DENSE_FOREST, "Dense Forest", "Thick, dark woods");
        dense_forest.add_tile(',', "dark_green", true, 3.0f, "mossy floor")
                    .add_tile('"', "dark_green", true, 3.0f, "thick brush")
                    .add_tile('T', "dark_green", false, 4.0f, "ancient tree")
                    .add_tile('T', "green", false, 2.0f, "tree")
                    .add_tile('t', "dark_green", true, 1.0f, "young tree")
                    .add_tile('*', "cyan", true, 0.1f, "glowing mushroom")
                    .add_tile('#', "dark_green", true, 1.0f, "thicket")
                    .set_thresholds(0.35f, 0.05f, 0.02f, 0.95f)
                    .set_modifiers(0.6f, 0.4f, true)
                    .add_ambient("The forest is eerily quiet.")
                    .add_ambient("Ancient trees block out the sky.");
        biomes[BiomeType::DENSE_FOREST] = dense_forest;
        
        BiomeData jungle(BiomeType::JUNGLE, "Jungle", "Dense tropical rainforest");
        jungle.add_tile(',', "green", true, 2.0f, "jungle floor")
              .add_tile('"', "green", true, 4.0f, "vines")
              .add_tile('"', "dark_green", true, 2.0f, "ferns")
              .add_tile('T', "green", false, 4.0f, "jungle tree")
              .add_tile('T', "dark_green", false, 2.0f, "giant tree")
              .add_tile('~', "dark_green", false, 0.5f, "swamp pool")
              .add_tile('*', "magenta", true, 0.3f, "orchid")
              .add_tile('*', "red", true, 0.2f, "exotic flower")
              .add_feature("snake", 's', "green", true, 0.003f)
              .add_feature("parrot", 'p', "red", true, 0.002f)
              .set_thresholds(0.30f, 0.03f, 0.08f, 0.85f)
              .set_modifiers(0.5f, 0.3f, true)
              .add_ambient("Humidity hangs heavy in the air.")
              .add_ambient("Strange calls echo through the trees.")
              .add_ambient("Colorful birds flit between branches.");
        biomes[BiomeType::JUNGLE] = jungle;
        
        BiomeData mushroom_forest(BiomeType::MUSHROOM_FOREST, "Mushroom Forest", "Strange fungal woods");
        mushroom_forest.add_tile(',', "magenta", true, 3.0f, "mycelium")
                       .add_tile('.', "dark_gray", true, 2.0f, "spore-covered ground")
                       .add_tile('*', "magenta", true, 2.0f, "small mushroom")
                       .add_tile('*', "cyan", true, 1.5f, "glowing mushroom")
                       .add_tile('T', "magenta", false, 2.0f, "giant mushroom")
                       .add_tile('T', "red", false, 1.0f, "red cap")
                       .add_tile('o', "yellow", true, 0.3f, "puffball")
                       .set_thresholds(0.50f, 0.05f, 0.02f, 0.70f)
                       .set_modifiers(0.8f, 0.6f, true)
                       .add_ambient("Spores drift through the air.")
                       .add_ambient("The mushrooms pulse with an inner light.");
        biomes[BiomeType::MUSHROOM_FOREST] = mushroom_forest;
        
        // ====== HIGHLAND BIOMES ======
        
        BiomeData hills(BiomeType::HILLS, "Hills", "Rolling highlands");
        hills.add_tile('.', "brown", true, 4.0f, "dirt")
             .add_tile(',', "green", true, 2.0f, "hill grass")
             .add_tile('^', "gray", true, 1.5f, "rocky slope")
             .add_tile('^', "brown", true, 1.0f, "steep slope")
             .add_tile('o', "gray", true, 0.5f, "boulder")
             .add_tile('o', "gray", false, 0.3f, "large boulder")
             .add_feature("cave entrance", 'O', "dark_gray", true, 0.003f)
             .add_feature("goat", 'g', "white", true, 0.002f)
             .set_thresholds(0.90f, 0.12f, 0.02f, 0.88f)
             .set_modifiers(0.8f, 1.0f, true)
             .add_ambient("The wind whistles across the hills.")
             .add_ambient("You can see for miles from here.");
        biomes[BiomeType::HILLS] = hills;
        
        BiomeData mountains(BiomeType::MOUNTAINS, "Mountains", "Towering peaks");
        mountains.add_tile('M', "gray", false, 4.0f, "mountain")
                 .add_tile('^', "gray", true, 3.0f, "rocky path")
                 .add_tile(':', "gray", true, 2.0f, "mountain trail")
                 .add_tile('o', "gray", false, 1.0f, "boulder")
                 .add_tile('A', "gray", false, 0.5f, "peak")
                 .add_tile('M', "white", false, 0.3f, "snow-capped")
                 .set_thresholds(0.95f, 0.15f, 0.01f, 0.95f)
                 .set_modifiers(0.5f, 1.5f, true)
                 .add_ambient("The air grows thin at this altitude.")
                 .add_ambient("Eagles soar on the mountain winds.");
        biomes[BiomeType::MOUNTAINS] = mountains;
        
        BiomeData snow_peaks(BiomeType::SNOW_PEAKS, "Snow Peaks", "Frozen mountain tops");
        snow_peaks.add_tile('A', "white", false, 4.0f, "snowy peak")
                  .add_tile('M', "white", false, 3.0f, "snow mountain")
                  .add_tile(':', "white", true, 2.0f, "snow path")
                  .add_tile('.', "white", true, 1.0f, "deep snow")
                  .add_tile('o', "cyan", false, 0.5f, "ice boulder")
                  .set_thresholds(0.98f, 0.10f, 0.01f, 0.98f)
                  .set_modifiers(0.3f, 2.0f, false)
                  .add_ambient("Bitter cold cuts through you.")
                  .add_ambient("Everything is blanketed in pristine snow.");
        biomes[BiomeType::SNOW_PEAKS] = snow_peaks;
        
        BiomeData volcanic(BiomeType::VOLCANIC, "Volcanic", "Scorched volcanic terrain");
        volcanic.add_tile('.', "dark_gray", true, 4.0f, "ash")
                .add_tile(',', "dark_gray", true, 2.0f, "volcanic sand")
                .add_tile('~', "red", false, 0.5f, "lava")
                .add_tile('~', "yellow", false, 0.3f, "molten rock")
                .add_tile('^', "dark_gray", true, 1.0f, "volcanic rock")
                .add_tile('o', "dark_gray", false, 0.5f, "cooled lava")
                .add_tile('*', "yellow", true, 0.1f, "ember")
                .set_thresholds(0.95f, 0.20f, 0.05f, 0.90f)
                .set_modifiers(0.7f, 0.8f, false)
                .add_ambient("The ground is hot beneath your feet.")
                .add_ambient("Sulfurous fumes rise from cracks in the earth.");
        biomes[BiomeType::VOLCANIC] = volcanic;
        
        // ====== DESERT BIOMES ======
        
        BiomeData desert(BiomeType::DESERT, "Desert", "Arid wasteland");
        desert.add_tile('.', "yellow", true, 5.0f, "sand")
              .add_tile(',', "yellow", true, 2.0f, "fine sand")
              .add_tile('~', "yellow", true, 0.5f, "sand ripple")
              .add_tile('o', "brown", true, 0.2f, "rock")
              .add_tile('#', "green", true, 0.1f, "cactus")
              .add_tile('*', "yellow", true, 0.05f, "desert flower")
              .add_feature("bones", 'x', "white", true, 0.005f)
              .add_feature("scorpion", 's', "brown", true, 0.002f)
              .set_thresholds(0.95f, 0.08f, 0.01f, 0.92f)
              .set_modifiers(0.9f, 1.5f, true)
              .add_ambient("The sun beats down mercilessly.")
              .add_ambient("Heat shimmers distort the horizon.");
        biomes[BiomeType::DESERT] = desert;
        
        BiomeData desert_dunes(BiomeType::DESERT_DUNES, "Sand Dunes", "Endless rolling dunes");
        desert_dunes.add_tile('~', "yellow", true, 5.0f, "dune slope")
                    .add_tile('.', "yellow", true, 3.0f, "dune top")
                    .add_tile(',', "yellow", true, 2.0f, "dune valley")
                    .set_thresholds(0.98f, 0.02f, 0.005f, 0.95f)
                    .set_modifiers(0.7f, 1.2f, false)
                    .add_ambient("The dunes shift and change constantly.")
                    .add_ambient("Sand whispers as wind reshapes the landscape.");
        biomes[BiomeType::DESERT_DUNES] = desert_dunes;
        
        // ====== WETLAND BIOMES ======
        
        BiomeData swamp(BiomeType::SWAMP, "Swamp", "Murky wetlands");
        swamp.add_tile(',', "brown", true, 3.0f, "mud")
             .add_tile('.', "brown", true, 2.0f, "wet earth")
             .add_tile('~', "dark_green", false, 3.0f, "murky water")
             .add_tile('"', "dark_green", true, 2.0f, "reeds")
             .add_tile('t', "brown", true, 1.0f, "dead tree")
             .add_tile('T', "brown", false, 0.5f, "swamp tree")
             .add_tile('*', "green", true, 0.2f, "moss clump")
             .add_feature("frog", 'f', "green", true, 0.005f)
             .add_feature("will-o-wisp", '*', "cyan", true, 0.002f)
             .set_thresholds(0.70f, 0.05f, 0.20f, 0.88f)
             .set_modifiers(0.6f, 0.6f, true)
             .add_ambient("Your boots squelch in the muck.")
             .add_ambient("Strange lights flicker in the mist.")
             .add_ambient("The smell of decay hangs in the air.");
        biomes[BiomeType::SWAMP] = swamp;
        
        // ====== COLD BIOMES ======
        
        BiomeData tundra(BiomeType::TUNDRA, "Tundra", "Frozen plains");
        tundra.add_tile('.', "white", true, 4.0f, "snow")
              .add_tile(',', "white", true, 2.0f, "patchy snow")
              .add_tile('"', "cyan", true, 1.0f, "frozen grass")
              .add_tile('o', "white", false, 0.5f, "snow mound")
              .add_tile('*', "cyan", true, 0.2f, "ice crystal")
              .add_feature("wolf tracks", ':', "gray", true, 0.003f)
              .add_feature("frozen pond", '~', "cyan", false, 0.01f, 0.7f, 3, 10)
              .set_thresholds(0.92f, 0.10f, 0.08f, 0.90f)
              .set_modifiers(0.8f, 1.2f, true)
              .add_ambient("The cold bites at exposed skin.")
              .add_ambient("A pale sun hangs low on the horizon.");
        biomes[BiomeType::TUNDRA] = tundra;
        
        BiomeData frozen_tundra(BiomeType::FROZEN_TUNDRA, "Frozen Tundra", "Permafrost wasteland");
        frozen_tundra.add_tile('.', "white", true, 4.0f, "ice")
                     .add_tile(',', "cyan", true, 2.0f, "frost")
                     .add_tile('~', "cyan", false, 1.0f, "frozen pool")
                     .add_tile('o', "white", false, 1.0f, "ice block")
                     .add_tile('*', "white", true, 0.3f, "icicle")
                     .set_thresholds(0.95f, 0.08f, 0.10f, 0.92f)
                     .set_modifiers(0.6f, 1.0f, false)
                     .add_ambient("Nothing survives in this frozen waste.")
                     .add_ambient("The ice groans and cracks beneath you.");
        biomes[BiomeType::FROZEN_TUNDRA] = frozen_tundra;
        
        // ====== DEAD/CORRUPTED BIOMES ======
        
        BiomeData dead_lands(BiomeType::DEAD_LANDS, "Dead Lands", "Blighted, corrupted terrain");
        dead_lands.add_tile('.', "dark_gray", true, 4.0f, "dead earth")
                  .add_tile(',', "gray", true, 2.0f, "ash")
                  .add_tile('t', "dark_gray", true, 1.0f, "dead tree")
                  .add_tile('x', "dark_gray", true, 0.5f, "bones")
                  .add_tile('~', "magenta", false, 0.2f, "corrupted pool")
                  .add_tile('*', "dark_gray", true, 0.1f, "withered plant")
                  .set_thresholds(0.90f, 0.15f, 0.05f, 0.95f)
                  .set_modifiers(0.9f, 0.8f, true)
                  .add_ambient("Nothing grows here anymore.")
                  .add_ambient("An oppressive silence weighs on you.")
                  .add_ambient("Dark energy pulses beneath the earth.");
        biomes[BiomeType::DEAD_LANDS] = dead_lands;
        
        // ====== ROAD ======
        
        BiomeData road(BiomeType::ROAD, "Road", "Well-traveled path");
        road.add_tile(':', "brown", true, 5.0f, "dirt road")
            .add_tile('.', "brown", true, 2.0f, "path")
            .add_tile(',', "gray", true, 1.0f, "gravel")
            .add_tile('=', "gray", true, 0.5f, "paved section")
            .set_modifiers(1.3f, 1.0f, false)  // Roads are faster
            .add_ambient("The road stretches on toward the horizon.");
        biomes[BiomeType::ROAD] = road;
    }
};

// Global registry instance (header-only singleton pattern)
inline BiomeRegistry& get_biome_registry() {
    static BiomeRegistry registry;
    return registry;
}
