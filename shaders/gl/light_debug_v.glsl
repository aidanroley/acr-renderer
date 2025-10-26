#version 420 core
layout(location = 0) in vec3 aPos;

layout(std140, binding = 0) uniform Camera {

    mat4 view;
    mat4 proj;
    vec4 viewPos;
};

uniform mat4 model;

void main()
{
    gl_Position = proj * view * model * vec4(aPos, 1.0);
}
