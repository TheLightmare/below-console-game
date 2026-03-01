#version 450

// =============================================================================
// Sprite Fragment Shader
// =============================================================================
// Samples the bound texture and multiplies by the per-vertex color tint.
// Discards fully transparent fragments (alpha test).
// =============================================================================

// Descriptor set 1, binding 0: Texture sampler
layout(set = 1, binding = 0) uniform sampler2D tex_sampler;

// Inputs from vertex shader
layout(location = 0) in vec2 frag_texcoord;
layout(location = 1) in vec4 frag_color;

// Output
layout(location = 0) out vec4 out_color;

void main() {
    vec4 tex_color = texture(tex_sampler, frag_texcoord);
    vec4 final_color = tex_color * frag_color;

    // Alpha test: discard near-invisible fragments
    if (final_color.a < 0.01) {
        discard;
    }

    out_color = final_color;
}
