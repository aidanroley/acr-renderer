#version 420 core
out vec4 outColor;  
  
in vec2 TexCoord;
in vec3 Normal;
in vec3 WorldPos;
in vec3 ViewPos;
in mat3 TBN;

uniform sampler2D albedoTex;
uniform sampler2D metalRoughTex;
uniform sampler2D occlusionTex;
uniform sampler2D normalTex;
uniform sampler2D transmissionTex;
uniform sampler2D thicknessTex;


layout(std140, binding = 6) uniform MaterialBuffer {
    vec4 colorFactors;
    vec4 metalRoughFactors;

    vec4 volume;             // x=usesVolume, y=thicknessFactor, z=attenuationDistance
    vec4 attenuationColor;   // rgb=attenuationColor

    vec4 transmission;       // x=usesTransmission, y=transmissionFactor
} material;

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

    vec3 lightPos[2] = { vec3(0.6f, 0.4f, 0.5f), vec3(0.6f, 0.4f, 0.5f) };
    vec3 lightColor[2] = { vec3(5.0, 4.75, 4.0), vec3(3.00, 4.0, 5.0) };

    vec3 albedo = texture(albedoTex, TexCoord).rgb * material.colorFactors.rgb;
    float metallic = texture(metalRoughTex, TexCoord).r * material.metalRoughFactors.x;
    float roughness = texture(metalRoughTex, TexCoord).g * material.metalRoughFactors.y;
    /*
    vec3 mr = texture(metalRoughTex, TexCoord).rgb;
    float roughness = clamp(mr.g * material.metalRoughFactors.y, 0.0, 1.0);
    float metallic  = clamp(mr.b * material.metalRoughFactors.x, 0.0, 1.0);
    */

    float ao = texture(occlusionTex, TexCoord).r;

    vec3 normal = texture(normalTex, TexCoord).rgb;
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

    vec3 ambient = vec3(0.13) * albedo * ao;
    vec3 color = ambient + Lo;
    color = color / (color + vec3(1.0));
    /*
    if (material.transmission.x < 0u) {
        //vec2 screenUV = gl_FragCoord.xy / vec2(framebufferWidth, framebufferHeight);
        //vec3 background = texture(sceneColor, screenUV).rgb;

        vec3 transmitted = material.transmission.y * albedo;
        vec3 reflected   = color * (1.0 - material.transmission.y);

        outColor = vec4(reflected + transmitted, 1.0); // alpha stays 1
        return;
    }
    */

   outColor = vec4(color, 1.0);
   //outColor = vec4(TexCoord, 0.0, 1.0);

    //outColor = vec4(normal * 0.5 + 0.5, 1.0);
    //outColor = vec4(vec3(material.transmission.y), 1.0);

}