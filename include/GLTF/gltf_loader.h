#pragma once
#include "vk_types.h"
#include "Engine/vk_helper_funcs.h"
#include "Descriptor/vk_descriptor.h"

// forward decs
class VkEngine;
struct VertexData;
std::optional<AllocatedImage> loadImage(VkEngine* engine, fastgltf::Asset& asset, fastgltf::Image& image);





struct Node;
class gltfData {

public:

	gltfData() = default;

	std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
	std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
	std::unordered_map<std::string, AllocatedImage> images;
	std::unordered_map<std::string, std::shared_ptr<gltfMaterial>> materials;

	// nodes that dont have a parent, for iterating through the file in tree order
	std::vector<std::shared_ptr<Node>> topNodes;

	std::vector<VkSampler> samplers;

	//DescriptorAllocatorGrowable descriptorPool;

	AllocatedBuffer materialDataBuffer;

	std::shared_ptr<gltfData> loadGltf(VkEngine* engine, std::filesystem::path path);
	void setDevice(VkDevice device, VkSampler dsl, AllocatedImage wi);

	void drawNodes(DrawContext& ctx);

	//~gltfData() { destroyAll(); };

private:

	//VulkanContext* context;
	struct EnginePassed {

		VkDevice device;
		AllocatedImage whiteImage;
		VkSampler defaultSamplerLinear;
	};

	EnginePassed passed = {};

	VkFilter extract_filter(fastgltf::Filter filter);
	VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter);
	//void destroyAll();
};

struct Node {

	std::weak_ptr<Node> parent;
	std::vector<std::shared_ptr<Node>> children; // this is a shared pointer because if there are no references to a child it can be removed

	glm::mat4 localTransform;
	glm::mat4 worldTransform;

	// this makes it so only MeshNode draws actually do anything,, otherwise just a recursive call for node structure.
	virtual void Draw(DrawContext& ctx)
	{
		// draw children (pre order transversal)
		for (auto& c : children) {
			c->Draw(ctx);
		}
	}

};

struct GLTFMetallicRoughness {

	MaterialPipeline opaquePipeline;
	MaterialPipeline transparentPipeline;
	VkDescriptorSetLayout materialLayout;

	struct MaterialConstants {

		glm::vec4 colorFactors;
		glm::vec4 metalRoughFactors;
		uint32_t colorTexID;
		uint32_t metalRoughTexID;

		// padding that makes it 256 bytes total
		uint32_t pad1;
		uint32_t pad2;
		glm::vec4 extra[13];
	};

	struct MaterialResources {

		AllocatedImage colorImage;
		VkSampler colorSampler;
		AllocatedImage metalRoughImage;
		VkSampler metalRoughSampler;
		VkBuffer dataBuffer;
		uint32_t dataBufferOffset;
	};

	//build pipelines;
	//clear resources;
	VkDescriptorSetLayout buildPipelines(VkEngine* engine);

	MaterialInstance writeMaterial(MaterialPass pass, const GLTFMetallicRoughness::MaterialResources& resources, DescriptorManager& descriptorManager, VkDevice& device);

};

struct MeshNode : public Node {

	std::shared_ptr<MeshAsset> mesh;
	virtual void Draw(DrawContext& ctx) override;
};



namespace fastgltf {

	class Asset;
	struct Primitive;
}
/*
struct VertexHash {

	std::size_t operator()(const Vertex& v) const {

		std::size_t h1 = std::hash<glm::vec3>()(v.pos);
		std::size_t h2 = std::hash<glm::vec2>()(v.texCoord);
		return h1 ^ (h2 << 1);
	}
};
*/

//void loadModel_SunTemple(VertexData& vertexData);
//std::pair<Mesh, MeshRenderer>
//void optimizeVertexBuffer(VertexData& vertexData);
void loadMeshData(const fastgltf::Asset& asset, VertexData& vertexData);
void loadMeshVertices(const fastgltf::Asset& asset, const fastgltf::Primitive* primitive, VertexData& vertexData);
void loadMeshIndices(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, VertexData& vertexData);
