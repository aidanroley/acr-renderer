#pragma once
#include "glEng/pbr_pipeline.h"
#include "glEng/gl_types.h"

struct Node;

struct gltfMaterial {

	GLuint dataBuffer;
	GLintptr dataBufferOffset;
	PBRSystem::MaterialInstance data;
};


struct SubMesh {

	uint32_t startIndex;
	uint32_t count;
	std::shared_ptr<gltfMaterial> material;
};

struct GPUMeshBuffers {

	GLuint vao = 0; // array obj
	GLuint vbo = 0; // vtx obj
	GLuint ebo = 0; // indices (called element buffer ?>????:<><><>
	GLsizei indexCount = 0;
};

struct MeshAsset {

	std::vector<SubMesh> submeshes;
	GPUMeshBuffers meshBuffers;
	glm::mat4 transform;

	std::shared_ptr<MeshAsset> clone() const {

		auto copy = std::make_shared<MeshAsset>(*this);
		return copy;
	}
};

struct RenderObject {

	uint32_t numIndices;
	uint32_t idxStart;

	GPUMeshBuffers meshBuffers;
	glm::mat4 transform;
	std::shared_ptr<gltfMaterial> material;

};

struct DrawContext {

	std::vector<RenderObject> submeshes;
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
	RenderObject createRenderObject(const SubMesh& submesh);
	virtual void Draw(DrawContext& ctx) override;
};

class gltfData {
public:

	struct GltfLoadContext {

		std::shared_ptr<gltfData> scene;
		fastgltf::Asset* gltf;
		glEngine* engine;
	};

	gltfData() = default;

	// nodes that dont have a parent, for iterating through the file in tree order
	std::vector<std::shared_ptr<Node>> topNodes;

	static std::shared_ptr<gltfData> Load(glEngine* engine, std::filesystem::path path);
	void drawNodes(DrawContext& ctx);

	GLuint materialDataBuffer;

	//~gltfData() { destroyAll(); };

private:

	fastgltf::Asset getGltfAsset(std::filesystem::path path);
	std::vector<GLSampler> createSamplers(GltfLoadContext ctx);
	std::vector<GLImage> createImages(GltfLoadContext ctx);
	std::vector<std::shared_ptr<gltfMaterial>> loadMaterials(GltfLoadContext ctx, std::vector<GLSampler>& samplers, std::vector<GLImage>& images);
	void fetchPBRTextures(fastgltf::Material& mat, GltfLoadContext ctx, PBRSystem::MaterialResources& materialResources, std::vector<GLSampler>& samplers, std::vector<GLImage>& images);
	std::vector<std::shared_ptr<MeshAsset>> loadMeshes(GltfLoadContext ctx, std::vector<std::shared_ptr<gltfMaterial>> materials);
	std::vector<std::shared_ptr<Node>> loadNodes(GltfLoadContext ctx, std::vector<std::shared_ptr<MeshAsset>> vecMeshes);

	PBRSystem pbrSystem;

};

GLenum extract_filter(fastgltf::Filter f);
GLenum extract_mipmap_filter(fastgltf::Filter f);
GLenum extract_wrap(fastgltf::Wrap w);
