#include "pch.h"
#include "vkEng/mesh_utils.h"

namespace MeshUtils {

    std::vector<glm::vec4> calculateTangents(
        const std::vector<glm::vec3>& positions,
        const std::vector<uint32_t>& indices,
        const std::vector<glm::vec2>& texCoords,
        const std::vector<glm::vec3>& normals)
    {
        std::vector<glm::vec3> tan1(positions.size(), glm::vec3(0.0f));
        std::vector<glm::vec3> tan2(positions.size(), glm::vec3(0.0f));

        for (size_t i = 0; i < indices.size(); i += 3) {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];

            glm::vec3 p0 = positions[i0];
            glm::vec3 p1 = positions[i1];
            glm::vec3 p2 = positions[i2];

            glm::vec2 uv0 = texCoords[i0];
            glm::vec2 uv1 = texCoords[i1];
            glm::vec2 uv2 = texCoords[i2];

            glm::vec3 e1 = p1 - p0;
            glm::vec3 e2 = p2 - p0;
            glm::vec2 d1 = uv1 - uv0;
            glm::vec2 d2 = uv2 - uv0;

            float denom = d1.x * d2.y - d2.x * d1.y;
            if (fabs(denom) < 1e-8f) continue;

            float r = 1.0f / denom;
            glm::vec3 sdir = (e1 * d2.y - e2 * d1.y) * r;
            glm::vec3 tdir = (e2 * d1.x - e1 * d2.x) * r;

            tan1[i0] += sdir; tan1[i1] += sdir; tan1[i2] += sdir;
            tan2[i0] += tdir; tan2[i1] += tdir; tan2[i2] += tdir;
        }

        std::vector<glm::vec4> result(positions.size());
        for (size_t i = 0; i < positions.size(); i++) {
            glm::vec3 n = glm::normalize(normals[i]);
            glm::vec3 t = tan1[i];

            // Gram?Schmidt
            t = glm::normalize(t - n * glm::dot(n, t));

            float w = (glm::dot(glm::cross(n, t), tan2[i]) < 0.0f) ? -1.0f : 1.0f;
            result[i] = glm::vec4(t, w);
        }
        return result;
    }

}