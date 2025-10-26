#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 aTangent;

layout(std140, binding = 0) uniform Camera {

    mat4 view;
    mat4 proj;
    vec4 viewPos;
};

uniform mat4 model;

out vec2 TexCoord;
out vec3 Normal;
out vec3 WorldPos;
out vec3 ViewPos;

out mat4 m_Model;
out mat4 m_Projection;
out mat4 m_View;

out vec3 vT; // world space tangent
out vec3 vN; // world space normal
out float vTSign; // tangent.w sign

void main() {

    TexCoord = aUV;
    Normal = aNormal;
    ViewPos = viewPos.xyz;
    WorldPos = vec3(model * vec4(aPos, 1.0));
    m_Model = model;
    m_Projection = proj;
    m_View = view;

    gl_Position = proj * view * model * vec4(aPos, 1.0);

    // normals
    mat3 nMat = transpose(inverse(mat3(model)));
    vec3 N = normalize(nMat * aNormal);
    vec3 T = normalize(mat3(model) * aTangent.xyz);
    T = normalize(T - N * dot(N, T)); // make T orthogonal to N

    vN = N;
    vT = T;
    vTSign = aTangent.w;



    /*
    // normals
    vec3 T = normalize(mat3(model) * inTangent.xyz);//* vec3(1, 0, 0));  //inTangent.xyz); //* vec3(1, 0, 0)); <- this is bugged
    // clip.xyz /= clip.w in bg
    vec3 N = normalize(mat3(model) * normal);
    vec3 B = cross(N, T) * inTangent.w;
    TBN = mat3(T, B, N);
    */
}
