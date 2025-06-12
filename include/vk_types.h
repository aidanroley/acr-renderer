#pragma once

#include <vulkan/vulkan.hpp> 
#include <vma/vk_mem_alloc.h>

//fix this later
#ifndef NDEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = true;
#endif

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

//const std::string MODEL_PATH = "models/viking_room.obj";
const std::string MATERIAL_PATH = "assets";
const std::string TEXTURE_PATH = "assets/viking_room.png";

const int MAX_FRAMES_IN_FLIGHT = 2; // double buffering


// Change as needed; self explanatory
const std::vector<const char*> validationLayers = {

    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {

    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
};

struct Vertex {

    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    glm::vec3 normal;
    uint32_t isEmissive;
    uint16_t texIndex = 0;

    // tells vulkan how to pass the data into the shader
    static VkVertexInputBindingDescription getBindingDescription() {

        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex); // space between each entry
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }
    // We need 3: one for color and position (each attribute) and texture coords
    static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions() {

        std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, normal);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32_UINT;
        attributeDescriptions[4].offset = offsetof(Vertex, isEmissive);

        return attributeDescriptions;
    }

    // For comparing 2 of these struct instances
    bool operator==(const Vertex& other) const {

        return pos == other.pos && color == other.color && texCoord == other.texCoord && normal == other.normal;
    }
};

// hash function for unordered map
namespace std {

    template<> struct hash<Vertex> {

        size_t operator()(Vertex const& vertex) const {

            return ((hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

// These 2 structs are "helper" structs for initialization functions

struct QueueFamilyIndices {

    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {

        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {

    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


struct AllocatedBuffer {

    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

struct AllocatedImage {

    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

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
    VkDescriptorSet materialSet;
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
};



struct RenderObject {

    uint32_t numIndices;
    uint32_t idxStart;
    VkBuffer indexBuffer;

    VkDeviceAddress vertexBufferAddress;
    VkBuffer vertexBuffer;
    glm::mat4 transform;

    std::shared_ptr<gltfMaterial> material;
};

struct DrawContext {

    std::vector<RenderObject> surfaces;
    // differential between opaque and transparent later.
};