# Console Game Makefile
# Target executable
TARGET = game.exe

# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2 -g -Iengine -I.
LDFLAGS = 

# Engine source files
ENGINE_SOURCES = engine/src/console.cpp \
                 engine/src/ui.cpp \
                 engine/src/component.cpp \
                 engine/src/combat_system.cpp \
                 engine/src/inventory_system.cpp \
                 engine/src/interaction_system.cpp \
                 engine/src/ai_system.cpp

# Game source files
GAME_SOURCES = main.cpp

# All source files
SOURCES = $(ENGINE_SOURCES) $(GAME_SOURCES)

# Header files (for dependency tracking)
HEADERS = engine/ecs/component_manager.hpp \
          engine/ecs/component.hpp \
          engine/ecs/component_serialization.hpp \
          engine/ecs/entity.hpp \
          engine/ecs/entity_loader.hpp \
          engine/ecs/types.hpp \
          engine/ecs/components/component_base.hpp \
          engine/ecs/components/position_component.hpp \
          engine/ecs/components/render_component.hpp \
          engine/ecs/components/stats_component.hpp \
          engine/ecs/components/inventory_component.hpp \
          engine/ecs/components/item_component.hpp \
          engine/ecs/components/movement_component.hpp \
          engine/ecs/components/interactable_component.hpp \
          engine/ecs/components/dialogue_component.hpp \
          engine/ecs/components/npc_component.hpp \
          engine/ecs/components/shop_component.hpp \
          engine/ecs/components/faction_component.hpp \
          engine/ecs/components/status_effects_component.hpp \
          engine/ecs/components/corpse_component.hpp \
          engine/render/console.hpp \
          engine/render/renderer.hpp \
          engine/render/ui.hpp \
          engine/input/input.hpp \
          engine/scene/scene.hpp \
          engine/scene/scene_manager.hpp \
          engine/scene/base_gameplay_scene.hpp \
          engine/scene/menu_window.hpp \
          engine/scene/new_game_window.hpp \
          engine/scene/save_load_window.hpp \
          engine/scene/common_scenes.hpp \
          engine/systems/combat_system.hpp \
          engine/systems/inventory_system.hpp \
          engine/systems/interaction_system.hpp \
          engine/systems/ai_system.hpp \
          engine/systems/dialogue_system.hpp \
          engine/systems/utility_ai.hpp \
          engine/systems/status_effect_system.hpp \
          engine/util/json.hpp \
          engine/util/rng.hpp \
          engine/util/save_system.hpp \
          engine/world/world.hpp \
          engine/world/chunk.hpp \
          engine/world/chunk_coord.hpp \
          engine/world/chunk_generator.hpp \
          engine/world/perlin_noise.hpp \
          engine/world/procedural_generator.hpp \
          engine/world/structure_loader.hpp \
          engine/world/jigsaw_generator.hpp \
          engine/world/dungeon_state.hpp \
          engine/world/biome.hpp \
          engine/world/biome_features.hpp \
          engine/world/tile_drawing_utils.hpp \
          engine/world/tileset.hpp \
          engine/world/game_state.hpp \
          game/scenes/exploration_scene.hpp \
          game/scenes/dungeon_scene.hpp \
          game/scenes/structure_debug_scene.hpp \
          game/entities/entity_factory.hpp \
          game/entities/effect_registry_init.hpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Default rule
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build successful! Run with: ./$(TARGET)"

# Compile source files to object files
%.o: %.cpp $(HEADERS)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run the game
run: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(OBJECTS) $(TARGET)
	@echo "Clean complete."

# Force rebuild
rebuild: clean all

# Debug build with extra flags
debug: CXXFLAGS += -DDEBUG -g3 -O0
debug: clean $(TARGET)

# Release build with optimizations
release: CXXFLAGS += -DNDEBUG -O3
release: clean $(TARGET)

# Check for compilation errors without building
check:
	$(CXX) $(CXXFLAGS) -fsyntax-only $(SOURCES)

# Help message
help:
	@echo "Console Game Makefile"
	@echo "====================="
	@echo ""
	@echo "Available targets:"
	@echo "  all      - Build the game (default)"
	@echo "  run      - Build and run the game"
	@echo "  clean    - Remove build artifacts"
	@echo "  rebuild  - Clean and rebuild"
	@echo "  debug    - Build with debug symbols"
	@echo "  release  - Build optimized release version"
	@echo "  check    - Check syntax without building"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Usage: make [target]"

# Phony targets (not actual files)
.PHONY: all run clean rebuild debug release check help
