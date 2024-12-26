#version 450

layout(binding = 0) uniform UniformBufferObject {

    mat4 model;
    mat4 view;
    mat4 proj;

    vec3 lightPos;
    vec3 lightColor;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

void main() {

    vec3 norm = normalize(fragNormal);
    vec3 lightDirection = normalize(ubo.lightPos - fragPos);
    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = diff * ubo.lightColor;
    vec3 result = diffuse * fragColor;
    outColor = vec4(result, 1.0);
    
    //outColor = vec4(fragColor, 1.0);
    //outColor = vec4(fragColor * texture(texSampler, fragTexCoord).rgb, 1.0);
    //outColor = texture(texSampler, fragTexCoord);
}
