#pragma once
#include "vk_types.h"

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
struct MaterialInstance {

    MaterialPipeline* pipeline;
    std::vector<VkDescriptorSet> materialSet;
    MaterialPass type;
    VkDescriptorSet imageSamplerSet;
};

struct gltfMaterial {

    MaterialInstance data;
};


// represents each subset of a mesh w/ its own material
struct GeoSurface {

    uint32_t startIndex;
    uint32_t count;
    Bounds bounds;
    std::shared_ptr<gltfMaterial> material;
};


// complete mesh
struct MeshAsset {

    std::string name;
    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers meshBuffers;
    glm::mat4 transform;

    std::shared_ptr<MeshAsset> clone() const {

        auto copy = std::make_shared<MeshAsset>(*this);
        return copy;
    }
};

struct RenderObject {

    // put these four in DrawData struct
    uint32_t numIndices;
    uint32_t idxStart;
    VkBuffer indexBuffer;
    VkBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress; // do i even use this . (the answer is no)
    
    glm::mat4 transform; // probably should be part of matinstance

    std::shared_ptr<gltfMaterial> material;
    std::vector<VkDescriptorSet> materialSet; // why is this in here and not just the MaterialInstance struct ?? 
};

struct DrawContext {

    std::vector<RenderObject> surfaces;
    // differential between opaque and transparent later.
};

class PBRMaterialSystem {

public:

    PBRMaterialSystem() = default;
    PBRMaterialSystem(DescriptorManager* descriptorManager) {

        _descriptorManager = descriptorManager;
    }

    MaterialPipeline opaquePipeline;
    MaterialPipeline transparentPipeline;
    VkDescriptorSetLayout materialLayout;

    struct MaterialPBRConstants {

        glm::vec4 colorFactors;
        glm::vec4 metalRoughFactors;
        uint32_t colorTexID;
        uint32_t metalRoughTexID;

        // padding that makes it 256 bytes total
        uint32_t pad1;
        uint32_t pad2;
        glm::vec4 extra[13];
    };

    struct TextureBinding {

        AllocatedImage image;
        VkSampler sampler;
    };

    struct MaterialResources {

        TextureBinding textures[4]; // 0 = albedo, 1 = metalRough, 2 = occ, 3 = normal map
        AllocatedImage colorImage;
        VkSampler colorSampler;
        AllocatedImage metalRoughImage;
        VkSampler metalRoughSampler;
        AllocatedImage occImage;
        VkSampler occSampler;
        AllocatedImage normalImage;
        VkSampler normalSampler;
        VkBuffer dataBuffer;
        uint32_t dataBufferOffset;
    };

    void initDescriptorSetLayouts();
    VkDescriptorSetLayout buildPipelines(VkEngine* engine);
    MaterialInstance writeMaterial(MaterialPass pass, const PBRMaterialSystem::MaterialResources& resources, VkDevice& device);

    VkDescriptorSetLayout _descriptorSetLayoutCamera;
    VkDescriptorSetLayout _descriptorSetLayoutMat;

    // TODO: implement clearResources()

    void setDescriptorManager(DescriptorManager* dm) {

        _descriptorManager = dm;
    }

private:

    DescriptorManager* _descriptorManager;

    enum MaterialTextureSlot {

        MATERIAL_TEX_ALBEDO = 0,
        MATERIAL_TEX_METAL_ROUGH = 1,
        MATERIAL_TEX_OCCULUSION = 2,
        MATERIAL_TEX_NORMAL = 3,
        UBO_INDEX = 4
    };
};
