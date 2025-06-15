#pragma once
#include "vk_types.h"

// Forward decs for helpers
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice* device, VkSurfaceKHR surface);
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
VkExtent2D chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities);
VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice& device);
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice& physicalDevice);
VkFormat findDepthFormat(VkPhysicalDevice& physicalDevice);
bool hasStencilComponent(VkFormat format);
VkCommandBuffer beginSingleTimeCommands(VkDevice& device, VkCommandPool& commandPool);
void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkDevice& device, VkCommandPool& commandPool, VkQueue& graphicsQueue);
void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels,
    VkPhysicalDevice& physicalDevice, VkDevice& device, VkCommandPool& commandPool, VkQueue& graphicsQueue);
VkImageView createImageView(VkDevice& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels,
    VkDevice& device, VkCommandPool& commandPool, VkQueue& graphicsQueue);
void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkDevice& device, VkCommandPool& commandPool, VkQueue& graphicsQueue);
void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDevice& device, VkPhysicalDevice& physicalDevice);
uint32_t findMemoryType(VkPhysicalDevice& physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDevice& device, VkPhysicalDevice& physicalDevice, VkQueue& graphicsQueue, VkCommandPool& commandPool);
AllocatedBuffer createBufferVMA(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocator& allocator);
void createImageNonVMA(VkPhysicalDevice& physicalDevice, VkDevice& device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkSampleCountFlagBits numSamples,
    VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t mipLevels);