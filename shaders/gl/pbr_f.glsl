#version 430 core
out vec4 outColor;  
  
in vec2 TexCoord;
in vec3 Normal;
in vec3 WorldPos;
in vec3 ViewPos;

in mat4 m_Model;
in mat4 m_Projection;
in mat4 m_View;

// normals
in vec3 vT; // world space tangent
in vec3 vN; // world space normal
in float vTSign; // tangent.w sign

uniform sampler2D albedoTex;
uniform sampler2D metalRoughTex;
uniform sampler2D occlusionTex;
uniform sampler2D normalTex;
uniform sampler2D transmissionTex;
uniform sampler2D thicknessTex;
uniform samplerCube irradianceMap;

uniform sampler2D uPrevDepth;   
uniform sampler2D uSceneColor;


layout(std140, binding = 8) uniform MaterialBuffer {
    vec4 colorFactors;
    vec4 metalRoughFactors; // x = metallic, y = roughness

    vec4 volume;             // x=usesVolume, y=thicknessFactor, z=attenuationDistance
    vec4 attenuationColor;   // rgb=attenuationColor

    vec4 transmission;       // x=usesTransmission, y=transmissionFactor
} material;

//vec3 baseColor = (texture(albedoTex, TexCoord) * material.colorFactors).rgb;
const float PI = 3.1415926;
vec3 g_transDiffuse = vec3(0.0);

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
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
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

vec3 getVolumeTransmissionRay(vec3 N, vec3 V, float thickness, float ior, mat4 model) {

    vec3 refractionVector = refract(-V, normalize(N), 1.0 / ior);
    vec3 modelScale;
    modelScale.x = length(vec3(model[0].xyz));
    modelScale.y = length(vec3(model[1].xyz));
    modelScale.z = length(vec3(model[2].xyz));

    return normalize(refractionVector) * thickness * modelScale;
}

vec3 getTransmissionSample(vec2 fragCoord, float roughness, float ior) {

    // un-hard code this res pls!
    //float framebufferLod = log2(float(1440)) * (roughness * clamp(ior * 2.0 - 2.0, 0.0, 1.0));
    //vec3 transmittedLight = textureLod(uSceneColor, fragCoord.xy, framebufferLod).rgb;

    float maxMip  = float(textureQueryLevels(uSceneColor) - 1);
    float r       = clamp(roughness, 0.0, 1.0);
    float lodHint = r*r * 0.3 * maxMip;     // gentle mapping
    float maxSafe = max(0.0, maxMip - 2.0); // avoid last 2 tiny levels
    float lod     = clamp(lodHint, 0.0, maxSafe);

    // manual trilinear
    float l0=floor(lod), l1=min(l0+1.0, maxSafe), w=lod-l0;
    vec3 c0 = textureLod(uSceneColor, fragCoord, l0).rgb;
    vec3 c1 = textureLod(uSceneColor, fragCoord, l1).rgb;
    vec3 transmitted = mix(c0, c1, w);

    return transmitted;
}

vec3 applyVolumeAttenuation(vec3 radiance, float transmissionDistance, vec3 attenuationColor, float attenuationDistance)
{
    if (attenuationDistance == 0.0) {

        return radiance;
    }
    else {
        // beers law
        vec3 transmittance = pow(attenuationColor, vec3(transmissionDistance / attenuationDistance));
        return transmittance * radiance;
    }
}

vec3 getWorldNormal() {
    
    vec3 N = normalize(vN);
    vec3 T = normalize(vT - N * dot(N, vT));
    vec3 B = normalize(cross(N, T)) * vTSign;

    mat3 TBN = mat3(T, B, N);

    // [0, 1] -> [-1, 1] (cuz its gltf!)
    vec3 n_ts = texture(normalTex, TexCoord).xyz * 2.0 - 1.0;
    return normalize(TBN * n_ts);
}

void PBR() {

    vec4 texColor = texture(albedoTex, TexCoord);
    //if (texColor.a < 0.5) discard;

    vec3 lightPos[2] = { vec3(0.6f, 1.2f, 0.5f), vec3(1.0f, 1.0f, -0.8f) };
    vec3 lightColor[2] = { vec3(5.0), vec3(3.0) };


    vec3 albedo = texture(albedoTex, TexCoord).rgb * material.colorFactors.rgb;
    float metallic = texture(metalRoughTex, TexCoord).b * material.metalRoughFactors.x;
    float roughness = texture(metalRoughTex, TexCoord).g * material.metalRoughFactors.y;

    float ao = texture(occlusionTex, TexCoord).r;

    vec3 normal = texture(normalTex, TexCoord).rgb;
    //normal = normalize(TBN * (normal * 2.0 - 1.0));

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 Lo = vec3(0.0);
    //vec3 N = normalize(normal);
    vec3 N = getWorldNormal();
    //vec3 N = Normal;
    vec3 V = normalize(ViewPos - WorldPos); // view direction (fragment to camera)

    // https://github.com/KhronosGroup/glTF-Sample-Renderer
    // https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0
    // used as reference for transmission/volume 
    if (material.transmission.x > 0) {

        float ior = 1.5f;
        vec3 transmissionRay = getVolumeTransmissionRay(N, V, material.volume.y, ior, m_Model);
        float transmissionRayLength = length(transmissionRay);
        //vec3 refractedRayExit = ViewPos + transmissionRay; // try changing this to worldPos...
        vec3 refractedRayExit = WorldPos + transmissionRay; 

        // we need to figure out where the point is going to be on the screen, so reproduce the whole projection calculation (that the GPU does).
        vec4 ndcPos = m_Projection *  m_View * vec4(refractedRayExit, 1.0);
        vec2 refractionCoords = ndcPos.xy / ndcPos.w; 
        refractionCoords += 1.0;
        refractionCoords /= 2.0;

        //vec3 transmittedLight = getTransmissionSample(refractionCoords, material.colorFactors.y, ior); // material.colorFactors.y is perceptual roughness
        vec3 transmittedLight = getTransmissionSample(refractionCoords, roughness, ior);

        vec3 attenuatedColor = applyVolumeAttenuation(transmittedLight, transmissionRayLength, material.attenuationColor.rgb, material.volume.z);
        vec3 specularTransmission = attenuatedColor; // * albedo
        vec3 diffuse = vec3(0.0);
        g_transDiffuse = mix(vec3(0.0), specularTransmission, material.transmission.y);

        // debug
       
    }
    
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

    // ambient lighting using IR
    vec3 kS = fresnelSchlick(max(dot(N, V), 0.0), F0);
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse      = irradiance * albedo;
    //vec3 ambient = (kD * diffuse) * ao;
    vec3 ambient = irradiance * albedo * ao;
    vec3 color = ambient + Lo + g_transDiffuse;
    color = color / (color + vec3(1.0));


   outColor = vec4(color, 1.0);


}

void main() {

    vec2 screenUV = gl_FragCoord.xy / vec2(textureSize(uSceneColor, 0));


   // depth peel discard
    if (material.transmission.x > 0.0) {
        vec2 uv = gl_FragCoord.xy / vec2(textureSize(uPrevDepth, 0));
        float prevZ = texture(uPrevDepth, uv).r;   // prev (opaque or last peel) depth
        float z     = gl_FragCoord.z;              // current fragment depth

        // <1.0 means we have previous surface
        if (prevZ < 1.0) {

            float EPS = max(1e-4, 2.0 * fwidth(z));
            if (z >= prevZ - EPS) discard;  
        }
    }

    PBR();


    // maybe composite texture later
    if (material.transmission.x > 0) {

        vec3 bg = texture(uSceneColor, screenUV).rgb;

        // How much of the PBR layer to show (tweak 0..1)
        // maybe try material transmission.y as well
        float mixAmt = 1;

        vec3 blended = mix(bg, outColor.rgb, mixAmt);

        // Write with partial alpha so composite shows layering
        outColor = vec4(blended, mixAmt);
    }

}