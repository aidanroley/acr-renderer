#pragma once
#include "Renderer/renderer_setup.h"
#include "Descriptor/vk_descriptor.h"

#include "GLTF/gltf_loader.h"
#include "vk_types.h"
#include "Texture/texture_utils.h"
class VkEngine {
public:

    //Camera& camera;

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

    // Camera UBO for vulkan (storing this here and not in camera manager since vulkan is directly looking at this)
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


    void initVulkan();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    GPUMeshBuffers uploadMesh(std::vector<uint32_t> indices, std::vector<Vertex> vertices);

    void drawFrame(Renderer& renderer);
    void recreateSwapChain();
    void cleanupVkObjects();

    // gltf funcs
    // image loading
    VkImageCreateInfo createImageInfo(ImageCreateInfoProperties& properties);
    VkImageViewCreateInfo createImageViewInfo(ImageViewCreateInfoProperties& properties);

    AllocatedImage createImage(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmapped);
    AllocatedImage createImage(void* data, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmapped);

    gltfData loadedGltf;
    DescriptorManager* descriptorManager;
    Renderer* renderer;
    PBRMaterialSystem pbrSystem;

    void init(Renderer* rd, DescriptorManager* dm) {

        renderer = rd;
        descriptorManager = dm;
        pbrSystem.setDescriptorManager(descriptorManager);
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

    void cleanupSwapChain();
    void loadGltfFile();

};
