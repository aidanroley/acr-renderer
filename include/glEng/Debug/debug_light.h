#pragma once
#include "glEng/gl_types.h"

struct DebugMesh {

    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLsizei indexCount;
};

struct DebugVertex {

    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};


struct SphereMesh {

    std::vector<DebugVertex> vertices;
    std::vector<uint32_t> indices;
};

SphereMesh generateDebugSphere(unsigned int X_SEGMENTS = 32, unsigned int Y_SEGMENTS = 16);
DebugMesh createDebugMesh(const std::vector<DebugVertex>& vertices, const std::vector<uint32_t>& indices);