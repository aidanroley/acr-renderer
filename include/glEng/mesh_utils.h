#pragma once

namespace MeshUtils {

	std::vector<glm::vec4> calculateTangents(
        const std::vector<glm::vec3>& positions,
        const std::vector<uint32_t>& indices,
        const std::vector<glm::vec2>& texCoords,
        const std::vector<glm::vec3>& normals);
}