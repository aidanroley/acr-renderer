#pragma once
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/util.hpp>
#include "../../include/vk_types.h"
//#include "../../include/vk_setup.h"

// forward decs
//struct gltfMaterial;
std::optional<AllocatedImage> loadImage(VkEngine* engine, fastgltf::Asset& asset, fastgltf::Image& image);


// GPU buffers and stores GPU memory address for shaders
struct GPUMeshBuffers {

	AllocatedBuffer indexBuffer;
	AllocatedBuffer vertexBuffer;
	VkDeviceAddress vertexBufferAddress;
};

// geometry bounding 
struct Bounds {

	glm::vec3 origin;
	float sphereRadius;
	glm::vec3 extents;
};

enum class MaterialPass :uint8_t {

	MainColor,
	Transparent,
	Other
};

struct MaterialPipeline {

	VkPipeline* pipeline;
	VkPipelineLayout* layout;
};

// materialinstance and pipeline
struct gltfMaterial {

	MaterialPipeline* pipeline;
	VkDescriptorSet materialSet;
	MaterialPass type;
};


// represents each subset of a mesh w/ its own material
struct GeoSurface {

	uint32_t startIndex;
	uint32_t count;
	Bounds bounds;
	std::shared_ptr<gltfMaterial> material;
};


// complete model
struct MeshAsset {

	std::string name;
	std::vector<GeoSurface> surfaces;
	GPUMeshBuffers meshBuffers;
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
	/*
	build pipelines;
	clear resources;

	writeMaterial();
	*/
};

struct Node;
class gltfData {

public:

	std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
	std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
	std::unordered_map<std::string, AllocatedImage> images;
	std::unordered_map<std::string, std::shared_ptr<gltfMaterial>> materials;

	// nodes that dont have a parent, for iterating through the file in tree order
	std::vector<std::shared_ptr<Node>> topNodes;

	std::vector<VkSampler> samplers;

	//DescriptorAllocatorGrowable descriptorPool;

	AllocatedBuffer materialDataBuffer;

	std::shared_ptr<gltfData> loadGltf(VkEngine* engine, std::string_view filePath);

	~gltfData() { destroyAll(); };

private:

	//VulkanContext* context;

	VkFilter extract_filter(fastgltf::Filter filter);
	VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter);
	void destroyAll();
};

struct Node {

	std::weak_ptr<Node> parent;
	std::vector<std::shared_ptr<Node>> children; // this is a shared pointer because if there are no references to a child it can be removed

	glm::mat4 localTransform;
	glm::mat4 worldTransform;

	std::shared_ptr<MeshAsset> mesh;

	virtual void Draw(const glm::mat4& topMatrix)
	{
		// draw children
		for (auto& c : children) {
			c->Draw(topMatrix);
		}
	}

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
