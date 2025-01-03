#pragma once

namespace fastgltf {

	class Asset;
	struct Primitive;
}

struct VertexHash {

	std::size_t operator()(const Vertex& v) const {

		std::size_t h1 = std::hash<glm::vec3>()(v.pos);
		std::size_t h2 = std::hash<glm::vec2>()(v.texCoord);
		return h1 ^ (h2 << 1);
	}
};

void loadModel_SunTemple(VertexData& vertexData);
//std::pair<Mesh, MeshRenderer>
void optimizeVertexBuffer(VertexData& vertexData);
void loadMeshData(const fastgltf::Asset& asset, VertexData& vertexData);
void loadMeshVertices(const fastgltf::Asset& asset, const fastgltf::Primitive* primitive, VertexData& vertexData);
void loadMeshIndices(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, VertexData& vertexData);