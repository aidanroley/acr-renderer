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
#include "../include/vk_descriptor.h"

#include "../include/scene_info/gltf_loader.h"
#include "../include/vk_types.h"
#include "../include/texture_utils.h"

#include <optional>


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

    // Non-render sync variables (immediate GPU submission/copying)
    VkFence immFence;
    VkCommandBuffer immCommandBuffer;
    VkCommandPool immCommandPool;

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

    // VMA
    VmaAllocator _allocator;

    // default images
    AllocatedImage _whiteImage;
    AllocatedImage _blackImage;
    AllocatedImage _errorImage;

    // samplers
    VkSampler _defaultSamplerLinear;
    VkSampler _defaultSamplerNearest;

    //DescriptorManager descriptorManager;
    

    void initVulkan(VertexData& vertexData);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, VertexData& vertexData);

    gltfMaterial writeMaterial(MaterialPass pass, const GLTFMetallicRoughness::MaterialResources& resources);
    GPUMeshBuffers uploadMesh(std::vector<uint32_t> indices, std::vector<Vertex> vertices);

    void drawFrame(GraphicsSetup& graphics);
    void recreateSwapChain();
    void cleanupVkObjects();

    // gltf funcs
    // image loading
    VkImageCreateInfo createImageInfo(ImageCreateInfoProperties& properties);
    VkImageViewCreateInfo createImageViewInfo(ImageViewCreateInfoProperties& properties);

    AllocatedImage createRawImage(ImageCreateInfoProperties& imageProperties, bool mipmapped);
    AllocatedImage createImage(void* data, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmapped);

    // store textures
    struct TextureStorage {

        std::vector<VkDescriptorImageInfo> storage;
        std::unordered_map<std::string, uint32_t> texMap;
        uint32_t addTexture(const VkImageView& image, VkSampler sampler);
    };
    TextureStorage texStorage;

    gltfData loadedGltf;
    DescriptorManager descriptorManager;

    VkEngine(Camera& cameraPass) : camera(cameraPass), descriptorManager(this) {
    }

    DrawContext ctx;

private:

    void bootstrapVk();
    void loadGltfFile();

    void initDefaultValues();
    // vulkan-tutorial functions that will be slowly replaced as I make probably (aside from initAllocator)
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void initAllocator(); // This needs physicalDevice, device, instance to be called
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createColorResources();
    void createDepthResources();
    //void createTextureImage();
    //void createTextureImageView();
    void createTextureSampler();
    void createVertexBuffer(VertexData& vertexData);
    void createIndexBuffer(VertexData& vertexData);
    void createUniformBuffers();
    void createDescriptorPool();
    //void createDescriptorSets();
    void createCommandBuffers();
    void createSyncObjects();
    void cleanupSwapChain();

};

// Forward decs from graphics_setup.h
struct GraphicsSetup;
struct VertexData;



