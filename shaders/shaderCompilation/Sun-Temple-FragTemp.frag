#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 lightColor;
    vec3 viewPos;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;
layout(location = 4) flat in uint fragEmissive;

layout(location = 0) out vec4 outColor;

void main() {
    //outColor = vec4(1.0);
    vec4 texColor = texture(texSampler, fragTexCoord);
    //vec4 texColor = textureLod(texSampler, fragTexCoord, 0.0);
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * vec3(1.0);
    vec3 resultColor = texColor.rgb;
    //outColor = vec4(resultColor, texColor.a);
    outColor = texture(texSampler, fragTexCoord); // Must write
}
