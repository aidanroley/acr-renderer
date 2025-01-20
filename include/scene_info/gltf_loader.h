#pragma once
/*
class gltfData {

public:

	std::unordered_map<std::string, std::unique_ptr<MeshAsset>> meshes;
	std::unordered_map<std::string, std::unique_ptr<Node>> nodes;
	std::unordered_map<std::string, AllocatedImage> images;
	std::unordered_map<std::string, std::unique_ptr<gltfMaterial>> materials;

	// nodes that dont have a parent, for iterating through the file in tree order
	std::vector<std::shared_ptr<Node>> topNodes;

	std::vector<VkSampler> samplers;

	DescriptorAllocatorGrowable descriptorPool;

	AllocatedBuffer materialDataBuffer;

	~gltfData() { destroyAll(); };

private:

	VulkanContext* context;

	void destroyAll();
};
*/

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