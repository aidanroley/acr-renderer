#pragma once
#include "Engine/engine.h"

namespace VulkanSetup {

    void bootstrapVk(VkEngine* engine);
    void initDefaultImages(VkEngine* engine);
    //void loadGltfFile(VkEngine* engine);

    void initDefaultValues(VkEngine* engine);

    void createSurface(VkEngine* engine);
    void initAllocator(VkEngine* engine); // This needs physicalDevice, device, instance to be called
    void createSwapChain(VkEngine* engine);
    void createImageViews(VkEngine* engine);
    void createRenderPass(VkEngine* engine);
    void createDescriptorSetLayouts(VkEngine* engine);
    void createGraphicsPipeline(VkEngine* engine);
    void createFramebuffers(VkEngine* engine);
    void createCommandPool(VkEngine* engine);
    void createColorResources(VkEngine* engine);
    void createDepthResources(VkEngine* engine);
    void createUniformBuffers(VkEngine* engine);
    void createDescriptorPools(VkEngine* engine);
    void createCommandBuffers(VkEngine* engine);
    void createSyncObjects(VkEngine* engine);
    //void cleanupSwapChain(VkEngine* engine);
    void initCameraDescriptorSetLayout(VkEngine* engine);

    void init(VkEngine* engine);
};

// Forward decs from renderer_setup.h
struct RendererSetup;
struct VertexData;



