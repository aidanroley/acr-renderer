#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) out vec3 fragColor;

// For y, the top of the screen is -1 so the top left is (-1,-1), the top right is (1,-1), the bottom left is (-1, 1), the bottom right is (1, 1)
vec2 positions[3] = vec2[](

    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](

    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {

    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}