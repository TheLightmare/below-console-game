#version 450

// =============================================================================
// Sprite Vertex Shader
// =============================================================================
// Transforms 2D positions by the camera's orthographic projection matrix
// and passes through UVs and color tint to the fragment shader.
// =============================================================================

// Descriptor set 0, binding 0: Camera UBO
layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 mvp;
} camera;

// Vertex inputs (matching Vertex2D)
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texcoord;
layout(location = 2) in vec4 in_color;

// Outputs to fragment shader
layout(location = 0) out vec2 frag_texcoord;
layout(location = 1) out vec4 frag_color;

void main() {
    gl_Position   = camera.mvp * vec4(in_position, 0.0, 1.0);
    frag_texcoord = in_texcoord;
    frag_color    = in_color;
}
