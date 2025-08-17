#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 1) in vec2 TexCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 WorldPos;
layout(location = 4) in vec3 ViewPos;
layout(location = 5) in mat3 TBN;

layout(location = 0) out vec4 outColor;

// samplers
layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(set = 1, binding = 1) uniform sampler2D metalRoughSampler; // r=metallic (unused), g=roughness
layout(set = 1, binding = 2) uniform sampler2D occSampler; // occulusion
layout(set = 1, binding = 3) uniform sampler2D normalSampler;

// material factors
layout(set = 1, binding = 4) uniform MaterialBuffer {
    vec4 colorFactors;       // rgb multiplied with baseColor texture is albedo, a = alpha
    vec4 metalRoughFactors;  // x = metallicFactor (unused), y = roughnessFactor
} material;

const float PI = 3.1415926;
// calculate reflectivity given base reflectivity of a surface
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

void main() {

    // Hardcoded single point light in world space (TEMPORARY)!!!
    const vec3 lightPos   = vec3(0.0, 2.0, 0.0); // position in world space
    const vec3 lightColor = vec3(1.0, 0.95, 0.8);   // warm white

    vec3 toLight = normalize(lightPos - WorldPos); // fragWorldPos from VS
    float diff   = max(dot(normalize(Normal), toLight), 0.0);

    vec3 lighting = lightColor * diff;
    // TEMPORARY *********************************************
    // set up base PBR factors
    vec3 albedo = texture(texSampler, TexCoord).rgb * material.colorFactors.rgb; // fetch albedo
    float metallic = texture(metalRoughSampler, TexCoord).r * material.metalRoughFactors.x; 
    float roughness = texture(metalRoughSampler, TexCoord).g * material.metalRoughFactors.y;
    float ao = texture(occSampler, TexCoord).r;
    // set up normals
    vec3 normal = texture(normalSampler, TexCoord).rgb;
    normal = normal * 2.0 - 1.0;
    normal = normalize(TBN * normal);


    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // reflectance
    vec3 Lo = vec3(0.0);
    
    vec3 N = normalize(normal);
    vec3 V = normalize(ViewPos - WorldPos);

    // start per-light
    vec3 L = normalize(lightPos - WorldPos);
    vec3 H = normalize(V + L);
    float dist = length(lightPos - WorldPos);
    float attenuation = 1.0 / (dist * dist);
    vec3 radiance = lightColor * attenuation;

    // cook torrance brdf
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
    //color = pow(color, vec3(1.0/2.2));


    outColor = vec4(color, 1.0);
}
