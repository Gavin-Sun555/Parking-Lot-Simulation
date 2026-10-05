#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec4 inColor;

layout(push_constant) uniform PushConstants {
    vec2 scale;
    vec2 offset;
} ubo;

layout(location = 0) out vec4 fragColor;

void main() {
    // Orthographic projection:
    // inPos is in world units (x: [-140, 140], y: [-105, 105])
    vec2 ndc = (inPos + ubo.offset) * ubo.scale;
    // Vulkan Y is inverted compared to standard OpenGL
    gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
    fragColor = inColor;
}
