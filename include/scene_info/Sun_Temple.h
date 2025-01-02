#pragma once

namespace fastgltf {

	class Asset;
	struct Primitive;
}

struct Vec3Hash {

	std::size_t operator()(const glm::vec3& v) const {

		std::size_t h1 = std::hash<float>()(v.x);
		std::size_t h2 = std::hash<float>()(v.y);
		std::size_t h3 = std::hash<float>()(v.z);
		return h1 ^ (h2 << 1) ^ (h3 << 2);
	}
};

bool areVec3Equal(const glm::vec3& a, const glm::vec3& b, float epsilon = 0.0001f);

struct Vec3Equal {

	bool operator()(const glm::vec3& a, const glm::vec3& b) const {

		return areVec3Equal(a, b);
	}
};

void loadModel_SunTemple(VertexData& vertexData);
//std::pair<Mesh, MeshRenderer>
void loadMeshData(const fastgltf::Asset& asset, VertexData& vertexData);
void loadMeshVertices(const fastgltf::Asset& asset, const fastgltf::Primitive* primitive, VertexData& vertexData, std::unordered_map<glm::vec3, uint32_t, Vec3Hash, Vec3Equal>& uniqueVertices);
void loadMeshIndices(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, VertexData& vertexData);