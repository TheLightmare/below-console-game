# Console Engine Makefile
# Target executable
TARGET = engine_demo.exe

# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2 -g -Iengine -I.
LDFLAGS = 

# Engine source files
ENGINE_SOURCES = engine/src/console.cpp \
                 engine/src/ui.cpp

# Main entry point
SOURCES = main.cpp $(ENGINE_SOURCES)

# Header files (for dependency tracking)
HEADERS = engine/ecs/component_manager.hpp \
          engine/ecs/component.hpp \
          engine/ecs/entity.hpp \
          engine/ecs/types.hpp \
          engine/ecs/components/component_base.hpp \
          engine/ecs/components/position_component.hpp \
          engine/ecs/components/render_component.hpp \
          engine/ecs/components/movement_component.hpp \
          engine/ecs/components/properties_component.hpp \
          engine/render/console.hpp \
          engine/render/renderer.hpp \
          engine/render/ui.hpp \
          engine/input/input.hpp \
          engine/scene/scene.hpp \
          engine/scene/scene_manager.hpp \
          engine/util/json.hpp \
          engine/util/rng.hpp \
          engine/world/world.hpp \
          engine/world/chunk.hpp \
          engine/world/chunk_coord.hpp \
          engine/world/perlin_noise.hpp \
          engine/world/tile_drawing_utils.hpp

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
	@echo "Console Engine Makefile"
	@echo "======================"
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
