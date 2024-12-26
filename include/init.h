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

#include "../include/graphics_setup.h"

#include <optional>

// Forward decs from graphics_setup.h
struct GraphicsSetup;
struct VertexData;

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

//const std::string MODEL_PATH = "models/viking_room.obj";
const std::string MODEL_PATH = "assets/CornellBox-Original.obj";
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

    // tells vulkan how to pass the data into the shader
    static VkVertexInputBindingDescription getBindingDescription() {

        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex); // space between each entry
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }
    // We need 3: one for color and position (each attribute) and texture coords
    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {

        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
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

// These first structs are just for storing vulkan structures/variables needed for the renderer
struct VulkanContext {

    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
};

struct SwapChainInfo {

    VkSwapchainKHR* swapChain; // Pointer to the one in VulkanContext
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    bool framebufferResized = false;

    // Constructor to initialize swapChain
    SwapChainInfo(VkSwapchainKHR* swapChain)

        : swapChain(swapChain),
        swapChainImageFormat(VK_FORMAT_UNDEFINED),
        swapChainExtent{ 0, 0 }
    {}
};

struct PipelineInfo {

    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkPipeline graphicsPipeline;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
};

struct CommandInfo {

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
};

struct SyncObjects {

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
};

struct UniformData {

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    // Move these last 2 out here
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
};

struct TextureData {

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;
    uint32_t mipLevels;
};

struct PixelInfo {

    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;
};

struct DepthInfo {

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
};

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

// struct for the main app
struct VulkanSetup {

    VulkanContext* context;
    SwapChainInfo* swapChainInfo;
    PipelineInfo* pipelineInfo;
    CommandInfo* commandInfo;
    SyncObjects* syncObjects;
    UniformData* uniformData;
    TextureData* textureData;
    DepthInfo* depthInfo;
    PixelInfo* pixelInfo;

    VulkanSetup(VulkanContext* ctx, SwapChainInfo* sci, PipelineInfo* pi, CommandInfo* ci, SyncObjects* so, UniformData* ud, TextureData* td, DepthInfo* di, PixelInfo* pixi)
        : context(ctx), swapChainInfo(sci), pipelineInfo(pi), commandInfo(ci), syncObjects(so), uniformData(ud), textureData(td), depthInfo(di), pixelInfo(pixi) {}
};

/*
const std::vector<Vertex> vertices = {

    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},// Top left
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, // Top right
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // Bottom right
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // Bottom left

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {

    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};
*/



// forward function decs for initialization
void frameBufferResizeCallback(GLFWwindow* window, int width, int height);
void initVulkan(VulkanSetup& setup, GraphicsSetup& graphics);
void createInstance(VulkanContext& context);
bool checkValidationLayerSupport();
std::vector<const char*> getRequiredExtensions();
void setupDebugMessenger(VulkanContext& context);
void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* createInfo);
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
void pickPhysicalDevice(VulkanContext& context);
VkSampleCountFlagBits getMaxUsableSampleCount(VulkanContext& context);
bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
void createLogicalDevice(VulkanContext& context);
void createSurface(VulkanContext& context);
bool checkDeviceExtensionSupport(VkPhysicalDevice device);
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice* device, VkSurfaceKHR surface);
void createSwapChain(VulkanContext& context, SwapChainInfo& swapChainInfo);
void createImageViews(VulkanContext& context, SwapChainInfo& swapChainInfo);
void createRenderPass(VulkanContext& context, SwapChainInfo& swapChainInfo, PipelineInfo& pipelineInfo);
void createDescriptorSetLayout(VulkanContext& context, PipelineInfo& pipelineInfo);
void createGraphicsPipeline(VulkanContext& context, PipelineInfo& pipelineInfo);
VkShaderModule createShaderModule(const std::vector<char>& code, VulkanContext& context);
void createFramebuffers(VulkanContext& context, SwapChainInfo& swapChainInfo, PipelineInfo& pipelineInfo, DepthInfo& depthInfo, PixelInfo& pixelInfo);
void createCommandPool(VulkanContext& context, CommandInfo& commandInfo);
void createColorResources(VulkanContext& context, SwapChainInfo& swapChainInfo, PixelInfo& pixelInfo);
void createDepthResources(VulkanContext& context, SwapChainInfo& swapChainInfo, DepthInfo& depthInfo);
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VulkanContext& context);
VkFormat findDepthFormat(VulkanContext& context);
bool hasStencilComponent(VkFormat format);
void createTextureImage(VulkanContext& context, CommandInfo& commandInfo, TextureData& textureData);
void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, VulkanContext& context, CommandInfo& commandInfo);
void createTextureImageView(VulkanContext& context, TextureData& textureData);
VkImageView createImageView(VulkanContext& context, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
void createTextureSampler(VulkanContext& context, TextureData& textureData);
void createImage(VulkanContext& context, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkSampleCountFlagBits numSamples,
    VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t mipLevels);
void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, VulkanContext& context, CommandInfo& commandInfo);
void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VulkanContext& context, CommandInfo& commandInfo);
void createVertexBuffer(VulkanContext& context, PipelineInfo& pipelineInfo, CommandInfo& commandInfo, VertexData& vertexData);
void createIndexBuffer(VulkanContext& context, PipelineInfo& pipelineInfo, CommandInfo& commandInfo, VertexData& vertexData);
void createUniformBuffers(VulkanContext& context, UniformData& uniformData);
void createDesciptorPool(VulkanContext& context, UniformData& uniformData);
void endSingleTimeCommands(VkCommandBuffer commandBuffer, VulkanContext& context, CommandInfo& commandInfo);
void createDescriptorSets(VulkanContext& context, UniformData& uniformData, PipelineInfo& pipelineInfo, TextureData& textureData);
void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VulkanContext& context);
uint32_t findMemoryType(VulkanContext& context, uint32_t typeFilter, VkMemoryPropertyFlags properties);
VkCommandBuffer beginSingleTimeCommands(VulkanContext& context, CommandInfo& commandInfo);
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VulkanContext& context, CommandInfo& commandInfo);
void createCommandBuffers(VulkanContext& context, CommandInfo& commandInfo);
void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, PipelineInfo& pipelineInfo, CommandInfo& commandInfo, SwapChainInfo& swapChainInfo, UniformData& uniformData, SyncObjects& syncObjects, VertexData& vertexData);
void createSyncObjects(VulkanContext& context, SyncObjects& syncObjects);

void cleanupSwapChain(VulkanContext& context, SwapChainInfo& swapChainInfo, DepthInfo& depthInfo, PixelInfo& pixelInfo);
void cleanupVkObjects(VulkanSetup& setup);

