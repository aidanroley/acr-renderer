#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <vulkan/vulkan.hpp> //"..\Vulkan-Hpp\Vulkan-Hpp-1.3.295\vulkan\vulkan.hpp"
#include <vma/vk_mem_alloc.h>

#include "../include/graphics_setup.h"

#include <optional>

//fix this later
#ifndef NDEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = true;
#endif

class VkEngine {
public:

    Camera& camera;

    // Vulkan context-related variables
    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkQueue presentQueue;

    // Swapchain-related variables
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    bool framebufferResized = false;

    // Pipeline-related variables
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkPipeline graphicsPipeline;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    // Command-related variables
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    // Synchronization-related variables
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    // Uniform-related variables
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    // Texture-related variables
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;
    uint32_t mipLevels;

    // Pixel-related variables
    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

    // Depth-related variables
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    // Constructor
    VkEngine(Camera& cameraPass) : camera(cameraPass) {} 

    void initVulkan(VertexData& vertexData);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, VertexData& vertexData);

    void drawFrame(GraphicsSetup& graphics);
    void recreateSwapChain();
    void cleanupVkObjects();

private:

    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createColorResources();
    void createDepthResources();
    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();
    void createVertexBuffer(VertexData& vertexData);
    void createIndexBuffer(VertexData& vertexData);
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers();
    void createSyncObjects();
    void cleanupSwapChain();
};

// Forward decs from graphics_setup.h
struct GraphicsSetup;
struct VertexData;

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

    VK_KHR_SWAPCHAIN_EXTENSION_NAME
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



