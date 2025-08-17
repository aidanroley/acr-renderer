#include "pch.h"
#include "Engine/mesh_utils.h"

namespace MeshUtils {

	std::vector<glm::vec4> calculateTangents(const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indices,
        const std::vector<glm::vec2> texCoords, const std::vector<glm::vec3> normals) {

        std::vector<glm::vec3> tangents(positions.size(), glm::vec3(0.0f));
        std::vector<glm::vec3> bitangents(positions.size(), glm::vec3(0.0f));

        for (size_t i = 0; i < indices.size(); i += 3) {

            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];

            glm::vec3 tangent;
            glm::vec3 bitangent;

            glm::vec3 v1 = positions[indices[i]];
            glm::vec3 v2 = positions[indices[i + 1]];
            glm::vec3 v3 = positions[indices[i + 2]];

            glm::vec2 uv1 = texCoords[indices[i]];
            glm::vec2 uv2 = texCoords[indices[i + 1]];
            glm::vec2 uv3 = texCoords[indices[i + 2]];

            glm::vec3 e1 = v2 - v1;
            glm::vec3 e2 = v3 - v1;
            glm::vec2 deltaUV1 = uv2 - uv1;
            glm::vec2 deltaUV2 = uv3 - uv1;

            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

            tangent.x = f * (deltaUV2.y * e1.x - deltaUV1.y * e2.x);
            tangent.y = f * (deltaUV2.y * e1.y - deltaUV1.y * e2.y);
            tangent.z = f * (deltaUV2.y * e1.z - deltaUV1.y * e2.z);

            bitangent.x = f * (-deltaUV2.x * e1.x + deltaUV1.x * e2.x);
            bitangent.y = f * (-deltaUV2.x * e1.y + deltaUV1.x * e2.y);
            bitangent.z = f * (-deltaUV2.x * e1.z + deltaUV1.x * e2.z);

            tangents[i0] += tangent;
            tangents[i1] += tangent;
            tangents[i2] += tangent;

            bitangents[i0] += bitangent;
            bitangents[i1] += bitangent;
            bitangents[i2] += bitangent;
        }

        std::vector<glm::vec4> result;
        result.reserve(positions.size());
        for (size_t i = 0; i < positions.size(); i++) {

            glm::vec3 n = normals[i];
            glm::vec3 t = tangents[i];

            t = glm::normalize(t - n * glm::dot(n, t));
            float w = (glm::dot(glm::cross(n, t), bitangents[i]) < 0.0f) ? -1.0f : 1.0f;

            result.emplace_back(t, w);
        }
        return result;
	}
}