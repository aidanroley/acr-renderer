#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 1) in vec2 TexCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 WorldPos;
layout(location = 4) in vec3 ViewPos;
layout(location = 5) in mat3 TBN;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(set = 1, binding = 1) uniform sampler2D metalRoughSampler;
layout(set = 1, binding = 2) uniform sampler2D occSampler;
layout(set = 1, binding = 3) uniform sampler2D normalSampler;
layout(set = 1, binding = 4) uniform sampler2D trSampler;
layout(set = 1, binding = 5) uniform sampler2D thicknessSampler;

layout(set = 1, binding = 6, std140) uniform MaterialBuffer {
    vec4 colorFactors;
    vec4 metalRoughFactors;

    vec4 volume;             // x=usesVolume, y=thicknessFactor, z=attenuationDistance
    vec4 attenuationColor;   // rgb=attenuationColor

    vec4 transmission;       // x=usesTransmission, y=transmissionFactor
} material;

layout(set = 0, binding = 0) uniform FrameUBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 viewPos;
} frame;

const float PI = 3.1415926;

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 beerLambert(vec3 attenuationColor, float attenuationDistance, float pathLen) {
    vec3 safeColor = clamp(attenuationColor, vec3(1e-6), vec3(0.999999));
    float safeDist = max(attenuationDistance, 1e-6);
    vec3 sigma_a = -log(safeColor) / safeDist;
    return exp(-sigma_a * max(pathLen, 0.0));
}

float averageObjectScale(mat4 model) {
    return (length(vec3(model[0])) + length(vec3(model[1])) + length(vec3(model[2]))) / 3.0;
}

void main() {

    vec3 lightPos[2] = { vec3(2.0, 0.4, 0.5), vec3(0.5, 1.2, 0.2) };
    vec3 lightColor[2] = { vec3(5.0, 4.75, 4.0), vec3(3.0, 4.0, 5.0) };

    vec3 albedo = texture(texSampler, TexCoord).rgb * material.colorFactors.rgb;
    float metallic = texture(metalRoughSampler, TexCoord).r * material.metalRoughFactors.x;
    float roughness = texture(metalRoughSampler, TexCoord).g * material.metalRoughFactors.y;
    float ao = texture(occSampler, TexCoord).r;

    vec3 normal = texture(normalSampler, TexCoord).rgb;
    normal = normalize(TBN * (normal * 2.0 - 1.0));

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);
    vec3 N = normalize(normal);
    vec3 V = normalize(ViewPos - WorldPos);

    for (int i = 0; i < 2; i++) {
        vec3 L = normalize(lightPos[i] - WorldPos);
        vec3 H = normalize(V + L);
        float dist = length(lightPos[i] - WorldPos);
        float attenuation = 1.0 / (dist * dist);
        vec3 radiance = lightColor[i] * attenuation;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;
    color = color / (color + vec3(1.0));

    if (material.transmission.x < 0u) {
        //vec2 screenUV = gl_FragCoord.xy / vec2(framebufferWidth, framebufferHeight);
        //vec3 background = texture(sceneColor, screenUV).rgb;

        vec3 transmitted = material.transmission.y * albedo;
        vec3 reflected   = color * (1.0 - material.transmission.y);

        outColor = vec4(reflected + transmitted, 1.0); // alpha stays 1
        return;
    }

    outColor = vec4(color, 1.0);

    //outColor = vec4(normal * 0.5 + 0.5, 1.0);
    //outColor = vec4(vec3(material.transmission.y), 1.0);

}
