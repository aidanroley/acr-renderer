#pragma once
#include "Graphics/graphics_setup.h"
#include "Descriptor/vk_descriptor.h"

#include "GLTF/gltf_loader.h"
#include "vk_types.h"
#include "Texture/texture_utils.h"


class VkEngine {
public:

    Camera& camera;

    // Vulkan context-related variables
    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_4_BIT;
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

    void initVulkan();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    //MaterialInstance writeMaterial(MaterialPass pass, const GLTFMetallicRoughness::MaterialResources& resources, DescriptorManager& descriptorManager);
    GPUMeshBuffers uploadMesh(std::vector<uint32_t> indices, std::vector<Vertex> vertices);

    void drawFrame(GraphicsSetup& graphics);
    void recreateSwapChain();
    void cleanupVkObjects();

    // gltf funcs
    // image loading
    VkImageCreateInfo createImageInfo(ImageCreateInfoProperties& properties);
    VkImageViewCreateInfo createImageViewInfo(ImageViewCreateInfoProperties& properties);

    AllocatedImage createImage(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmapped);
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
    GLTFMetallicRoughness metalRoughMaterial;

    VkEngine(Camera& cameraPass) : camera(cameraPass), descriptorManager(this) {
    }

    DrawContext ctx;

    struct Pipelines {
        
        VkPipeline opaque;
        VkPipeline transparent;
        VkPipelineLayout layout;
    } pipelines;

    // Pipeline-related variables
    VkRenderPass renderPass;


private:

    void bootstrapVk();
    void initDefaultImages();
    void loadGltfFile();

    void initDefaultValues();

    void createSurface();
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
    void createUniformBuffers();
    void createDescriptorPools();
    void createCommandBuffers();
    void createSyncObjects();
    void cleanupSwapChain();

};

// Forward decs from graphics_setup.h
struct GraphicsSetup;
struct VertexData;



