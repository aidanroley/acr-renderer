#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(set = 0, binding = 0) uniform FrameUBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 viewPos;
} frame;

layout(push_constant) uniform PushConstants { mat4 model; } pc;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec4 inTangent;
//layout(location = 4) in uint isEmissive;

//layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragPos;
layout(location = 4) out vec3 viewPos;
layout(location = 5) out mat3 outTBN;
//layout(location = 4) flat out uint fragEmissive;

void main() {

    gl_Position = frame.proj * frame.view * pc.model * vec4(inPosition, 1.0);
    fragPos = vec3(pc.model * vec4(inPosition, 1.0));
    fragNormal = inNormal;
    fragTexCoord = inTexCoord;
    viewPos = frame.viewPos;

    // normal map calculation

    vec3 T = normalize(mat3(pc.model) * vec3(1, 0, 0));//inTangent.xyz); //* vec3(1, 0, 0));
    vec3 N = normalize(mat3(pc.model) * inNormal);
    vec3 B = cross(N, T);// * inTangent.w;
    outTBN = mat3(T, B, N);
}