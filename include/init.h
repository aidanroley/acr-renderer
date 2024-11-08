#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>

#include <iostream>
#include <cstring>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>
#include "..\Vulkan-Hpp\Vulkan-Hpp-1.3.295\vulkan\vulkan.hpp"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2; // double buffering

// These first 5 structs are just for storing vulkan structures/variables needed for the renderer
struct VulkanContext {

    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice;
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

    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkPipeline graphicsPipeline;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
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

// Change as needed; self explanatory
const std::vector<const char*> validationLayers = {

    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {

    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// struct for the main app
struct VulkanSetup {

    VulkanContext* context;
    SwapChainInfo* swapChainInfo;
    PipelineInfo* pipelineInfo;
    CommandInfo* commandInfo;
    SyncObjects* syncObjects;

    VulkanSetup(VulkanContext* ctx, SwapChainInfo* sci, PipelineInfo* pi, CommandInfo* ci, SyncObjects* so)
        : context(ctx), swapChainInfo(sci), pipelineInfo(pi), commandInfo(ci), syncObjects(so) {}
};

struct Vertex {

    glm::vec2 pos;
    glm::vec3 color;

    // tells vulkan how to pass the data into the shader
    static VkVertexInputBindingDescription getBindingDescription() {

        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex); // space between each entry
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }
    // We need 2: one for color and position (each attribute)
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {

        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);
        return attributeDescriptions;
    }
};

const std::vector<Vertex> vertices = {

    {{0.0f, -0.5f}, {1.0f,  1.0f, 1.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};

// forward function decs for initialization
VulkanSetup initApp(VulkanContext& context, SwapChainInfo& swapChainInfo, PipelineInfo& pipelineInfo, CommandInfo& commandInfo, SyncObjects& syncObjects);
void initWindow(VulkanContext& context, SwapChainInfo& swapChainInfo);
void frameBufferResizeCallback(GLFWwindow* window, int width, int height);
void initVulkan(VulkanContext& context, SwapChainInfo& swapChainInfo, PipelineInfo& pipelineInfo, CommandInfo& commandInfo, SyncObjects& syncObjects);
void createInstance(VulkanContext& context);
//void mainLoop(VulkanContext& context, SwapChainInfo& swapChainInfo, PipelineInfo& pipelineInfo, CommandInfo& commandInfo, SyncObjects& syncObjects);
bool checkValidationLayerSupport();
std::vector<const char*> getRequiredExtensions();
void setupDebugMessenger(VulkanContext& context);
void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* createInfo);
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
void pickPhysicalDevice(VulkanContext& context);
bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
void createLogicalDevice(VulkanContext& context);
void createSurface(VulkanContext& context);
bool checkDeviceExtensionSupport(VkPhysicalDevice device);
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice* device, VkSurfaceKHR surface);
void createSwapChain(VulkanContext& context, SwapChainInfo& swapChainInfo);
void createImageViews(VulkanContext& context, SwapChainInfo& swapChainInfo);
void createRenderPass(VulkanContext& context, SwapChainInfo& swapChainInfo, PipelineInfo& pipelineInfo);
void createGraphicsPipeline(VulkanContext& context, PipelineInfo& pipelineInfo);
VkShaderModule createShaderModule(const std::vector<char>& code, VulkanContext& context);
void createFramebuffers(VulkanContext& context, SwapChainInfo& swapChainInfo, PipelineInfo& pipelineInfo);
void createCommandPool(VulkanContext& context, CommandInfo& commandInfo);
void createVertexBuffer(VulkanContext& context, PipelineInfo& pipelineInfo);
uint32_t findMemoryType(VulkanContext& context, uint32_t typeFilter, VkMemoryPropertyFlags properties);
void createCommandBuffers(VulkanContext& context, CommandInfo& commandInfo);
void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, PipelineInfo& pipelineInfo, CommandInfo& commandInfo, SwapChainInfo& swapChainInfo);
void createSyncObjects(VulkanContext& context, SyncObjects& syncObjects);

void cleanupSwapChain(VulkanContext& context, SwapChainInfo& swapChainInfo);
void cleanupVkObjects(VulkanContext& context, SwapChainInfo& swapChainInfo, PipelineInfo& pipelineInfo, CommandInfo& commandInfo, SyncObjects& syncObjects);

