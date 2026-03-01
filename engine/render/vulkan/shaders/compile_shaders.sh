#!/bin/bash
# =============================================================================
# Compile GLSL shaders to SPIR-V using glslc (from the Vulkan SDK)
# =============================================================================

SHADER_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT_DIR="${SHADER_DIR}/../../../../build/shaders"

mkdir -p "$OUT_DIR"

echo "Compiling sprite.vert..."
glslc "$SHADER_DIR/sprite.vert" -o "$OUT_DIR/sprite.vert.spv" || { echo "FAILED"; exit 1; }

echo "Compiling sprite.frag..."
glslc "$SHADER_DIR/sprite.frag" -o "$OUT_DIR/sprite.frag.spv" || { echo "FAILED"; exit 1; }

echo "All shaders compiled successfully to $OUT_DIR"
