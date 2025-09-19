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

    Opaque,
    Transparent,
    Transmission,
    Other
};
//debug
inline std::ostream& operator <<(std::ostream& os, MaterialPass pass) {

    switch (pass) {
    case MaterialPass::Opaque:       return os << "Opaque";
    case MaterialPass::Transparent:  return os << "Transparent";
    case MaterialPass::Transmission: return os << "Transmission";
    default:                         return os << "Unknown";
    }
}

struct MaterialPipeline {

    VkPipeline pipeline;
    VkPipelineLayout layout;
};

// materialinstance and pipeline
struct MaterialInstance {

    MaterialPipeline matPipeline;
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
    std::vector<VkDescriptorSet> materialSet;
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


    struct alignas(16) MaterialPBRConstants {

        glm::vec4 colorFactors;        // 16 bytes
        glm::vec4 metalRoughFactors;   // 16 bytes

        glm::vec4 volume;              // usesVolume (x), thicknessFactor (y), attenuationDistance (z), unused (w)
        glm::vec4 attenuationColor;    // attenuationColor.rgb, .a unused

        glm::vec4 transmission;        // usesTransmission (x), transmissionFactor (y), unused (zw)

        glm::vec4 extra[11];           // pad to 256 bytes if you need
    };

    struct TextureBinding {

        AllocatedImage image;
        VkSampler sampler;
    };

    struct MaterialResources {

        TextureBinding albedo;
        TextureBinding metalRough;
        TextureBinding occlusion;
        TextureBinding normalMap;
        TextureBinding transmission;
        TextureBinding volumeThickness;

        VkBuffer dataBuffer;
        uint32_t dataBufferOffset;
    };

    void initDescriptorSetLayouts();
    VkDescriptorSetLayout buildPipelines(VkEngine* engine);
    MaterialInstance writeMaterial(MaterialPass pass, const PBRMaterialSystem::MaterialResources& resources, VkEngine* engine);
    VkWriteDescriptorSet makeImageWrite(VkDescriptorSet dstSet, uint32_t dstBinding, const TextureBinding& tb, VkDescriptorImageInfo& outInfo);
    MaterialPipeline getPipeline(MaterialPass pass, VkEngine* engine);

    VkDescriptorSetLayout _descriptorSetLayoutCamera;
    VkDescriptorSetLayout _descriptorSetLayoutMat;

    // TODO: implement clearResources()

    void setDescriptorManager(DescriptorManager* dm) {

        _descriptorManager = dm;
    }

    struct TransmissionPass {

        VkRenderPass renderPass;
        AllocatedImage sceneColorImage;
        VkSampler sampler;
        VkPipeline pipeline;

        bool isValid() const { return renderPass != VK_NULL_HANDLE && sceneColorImage.image != VK_NULL_HANDLE; }
    };

    void initTransmissionPass(int swapW, int swapH, VkDevice device, VmaAllocator allocator);
    TransmissionPass trPass;

private:

    DescriptorManager* _descriptorManager;

    enum MaterialTextureSlot {

        MATERIAL_TEX_ALBEDO = 0,
        MATERIAL_TEX_METAL_ROUGH = 1,
        MATERIAL_TEX_OCCLUSION = 2,
        MATERIAL_TEX_NORMAL = 3,
        MATERIAL_TEX_TRANSMISSION = 4,
        MATERIAL_TEX_THICKNESS = 5,
        UBO_INDEX = 6
    };
};
