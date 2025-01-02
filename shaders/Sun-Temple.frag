#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(binding = 0) uniform UniformBufferObject {

    mat4 model;
    mat4 view;
    mat4 proj;

    vec3 lightPos;
    vec3 lightColor;
    vec3 viewPos;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;
layout(location = 4) flat in uint fragEmissive;

layout(location = 0) out vec4 outColor;

void main() {

    outColor = vec4(0.0, 0.0, 1.0, 1.0); //vec4(1.0, 1.0, 1.0, 1.0);
}