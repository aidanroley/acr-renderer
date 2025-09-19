#pragma once
#include "vk_types.h"
#include "vkEng/vk_helper_funcs.h"
#include "vkEng/Descriptor/vk_descriptor.h"
#include "vkEng/pbr_pipeline.h"

// forward decs
class VkEngine;
struct VertexData;
std::optional<AllocatedImage> loadImage(VkEngine* engine, fastgltf::Asset& asset, fastgltf::Image& image);

struct Node;
class gltfData {
public:

	struct GltfLoadContext {

		std::shared_ptr<gltfData> scene;
		fastgltf::Asset* gltf;
		VkEngine* engine;
	};

	gltfData() = default;

	// nodes that dont have a parent, for iterating through the file in tree order
	std::vector<std::shared_ptr<Node>> topNodes;
	AllocatedBuffer materialDataBuffer;

	static std::shared_ptr<gltfData> Load(VkEngine* engine, std::filesystem::path path);
	void drawNodes(DrawContext& ctx);

	//~gltfData() { destroyAll(); };

private:

	fastgltf::Asset getGltfAsset(std::filesystem::path path);
	std::vector<VkSampler> createSamplers(GltfLoadContext ctx);
	std::vector<AllocatedImage> createImages(GltfLoadContext ctx);
	std::vector<std::shared_ptr<gltfMaterial>> loadMaterials(GltfLoadContext ctx, std::vector<VkSampler>& samplers, std::vector<AllocatedImage>& images);
	void fetchPBRTextures(fastgltf::Material& mat, GltfLoadContext ctx, PBRMaterialSystem::MaterialResources& materialResources, std::vector<VkSampler>& samplers, std::vector<AllocatedImage>& images);
	std::vector<std::shared_ptr<MeshAsset>> loadMeshes(GltfLoadContext ctx, std::vector<std::shared_ptr<gltfMaterial>> materials);
	std::vector<std::shared_ptr<Node>> loadNodes(GltfLoadContext ctx, std::vector<std::shared_ptr<MeshAsset>> vecMeshes);

	//void destroyAll();

	
};

struct Node {

	std::weak_ptr<Node> parent;
	std::vector<std::shared_ptr<Node>> children; // this is a shared pointer because if there are no references to a child it can be removed

	glm::mat4 localTransform;
	glm::mat4 worldTransform;

	std::string name;
	// this makes it so only MeshNode draws actually do anything,, otherwise just a recursive call for node structure.
	virtual void Draw(DrawContext& ctx)
	{
		// draw children (pre order transversal)
		for (auto& c : children) {
			c->Draw(ctx);
		}
	}

};

struct MeshNode : public Node {

	std::shared_ptr<MeshAsset> mesh;
	RenderObject createRenderObject(const GeoSurface& surface);
	virtual void Draw(DrawContext& ctx) override;
};


VkFilter extract_filter(fastgltf::Filter filter);
VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter);