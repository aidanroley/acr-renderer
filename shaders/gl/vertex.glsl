#version 420 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec4 inTangent;

out vec2 TexCoord;
out vec3 Normal;
out vec3 WorldPos;
out vec3 ViewPos;
out mat3 TBN;

layout(std140, binding = 0) uniform Camera {

    mat4 view;
    mat4 proj;
    vec4 viewPos;
};

uniform mat4 model;

void main() {

    TexCoord = uv;
    Normal = normal;
    ViewPos = viewPos.xyz;
    WorldPos = vec3(model * vec4(aPos, 1.0));

    gl_Position = proj * view * model * vec4(aPos, 1.0);

    // normals
    vec3 T = normalize(mat3(model) * vec3(1, 0, 0));  //inTangent.xyz); //* vec3(1, 0, 0)); <- this is bugged
    vec3 N = normalize(mat3(model) * normal);
    vec3 B = cross(N, T);// * inTangent.w;
    TBN = mat3(T, B, N);
}
