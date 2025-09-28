#include "pch.h"
#include "glEng/Debug/debug_light.h"

SphereMesh generateDebugSphere(unsigned int X_SEGMENTS, unsigned int Y_SEGMENTS) {

	SphereMesh sphere;
	float M_PI = 3.14159;

	for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {

		for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {

			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			float xPos = std::cos(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);
			float yPos = std::cos(ySegment * M_PI);
			float zPos = std::sin(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);

			DebugVertex vert{};
			vert.pos = glm::vec3(xPos, yPos, zPos);
			vert.normal = glm::vec3(xPos, yPos, zPos); // unit sphere normal
			vert.uv = glm::vec2(xSegment, ySegment);

			sphere.vertices.push_back(vert);
		}
	}

	bool oddRow = false;
	for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {

		for (unsigned int x = 0; x < X_SEGMENTS; ++x) {

			uint32_t i0 = y * (X_SEGMENTS + 1) + x;
			uint32_t i1 = (y + 1) * (X_SEGMENTS + 1) + x;
			uint32_t i2 = (y + 1) * (X_SEGMENTS + 1) + x + 1;
			uint32_t i3 = y * (X_SEGMENTS + 1) + x + 1;

			sphere.indices.push_back(i0);
			sphere.indices.push_back(i1);
			sphere.indices.push_back(i2);

			sphere.indices.push_back(i0);
			sphere.indices.push_back(i2);
			sphere.indices.push_back(i3);
		}
	}

	return sphere;
}

DebugMesh createDebugMesh(const std::vector<DebugVertex>& vertices, const std::vector<uint32_t>& indices) {
    DebugMesh mesh{};
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(DebugVertex),
        vertices.data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(uint32_t),
        indices.data(),
        GL_STATIC_DRAW);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        sizeof(DebugVertex),
        (void*)offsetof(DebugVertex, pos));

    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
        sizeof(DebugVertex),
        (void*)offsetof(DebugVertex, normal));

    // UV
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
        sizeof(DebugVertex),
        (void*)offsetof(DebugVertex, uv));

    glBindVertexArray(0);

    mesh.indexCount = static_cast<GLsizei>(indices.size());
    return mesh;
}
