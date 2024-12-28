#version 450

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

    float ambientStrength = 0.1;
    vec3 ambientLightColor = {0.8f, 0.7f, 0.6f};

    vec3 norm = normalize(fragNormal);
    vec3 lightDirection = normalize(ubo.lightPos - fragPos);
    float diff = max(dot(norm, lightDirection), 0.0);
    
    vec3 ambient = ambientLightColor * ambientStrength;
    vec3 diffuse = diff * ubo.lightColor;
    vec3 combinedAmbientDiffuse = ambient * fragColor * diffuse;
    
    // I was wondering what specular would look like w/ no rounded objects. 
    float specularStrength = 0.1;
    vec3 viewDirection = normalize(ubo.viewPos - fragPos);
    vec3 reflectDirection = reflect(-lightDirection, norm);
    float spec = pow(max(dot(viewDirection, reflectDirection), 0.0), 32);
    vec3 specular = specularStrength * spec * ubo.lightColor;

    vec3 finalColor = combinedAmbientDiffuse + specular * fragColor;

    // This is for the color of the rectangle that outputs the light
    vec3 emissive = vec3(17.0f, 12.0f, 4.0f);
    if (fragEmissive != 0) {
        
        finalColor += emissive;
    }
    outColor = vec4(finalColor, 1.0);
}
