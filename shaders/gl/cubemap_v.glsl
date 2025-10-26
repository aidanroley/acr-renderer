// cubemap_v.glsl
#version 430 core
layout (location = 0) in vec3 aPos;
out vec3 localPos;

layout(std140, binding = 0) uniform Camera {

    mat4 view;
    mat4 proj;
    vec4 viewPos;   
};

uniform mat4 cubemapView;     // make sure it's 'uniform'
uniform mat4 cubemapProj;

void main() {
    localPos = aPos;
    mat4 v = mat4(mat3(cubemapView));  // strip translation
    vec4 p = cubemapProj * v * vec4(localPos, 1.0);
    gl_Position = vec4(p.xy, p.w, p.w); // == p.xyww -> depth = 1.0
}