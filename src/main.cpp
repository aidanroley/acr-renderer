#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <cstring>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>
#include "..\Vulkan-Hpp\Vulkan-Hpp-1.3.295\vulkan\vulkan.hpp"

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
};

struct CommandInfo {

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
};

struct SyncObjects {
    
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
};

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

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {

    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {

    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

void initWindow(VulkanContext& context);
void initVulkan(VulkanContext& context, SwapChainInfo& swapChainInfo, PipelineInfo& pipelineInfo, CommandInfo& commandInfo, SyncObjects& syncObjects);
void createInstance(VulkanContext& context);
void mainLoop(VulkanContext& context, SwapChainInfo& swapChainInfo, PipelineInfo& pipelineInfo, CommandInfo& commandInfo, SyncObjects& syncObjects);
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
std::vector<char> readFile(const std::string& filename);
void createFramebuffers(VulkanContext& context, SwapChainInfo& swapChainInfo, PipelineInfo& pipelineInfo);
void createCommandPool(VulkanContext& context, CommandInfo& commandInfo);
void createCommandBuffer(VulkanContext& context, CommandInfo& commandInfo);
void createSyncObjects(VulkanContext& context, SyncObjects& syncObjects);
void drawFrame(VulkanContext& context, SwapChainInfo& swapChainInfo, CommandInfo& commandInfo, SyncObjects& syncObjects, PipelineInfo& pipelineInfo);

void cleanup(VulkanContext& context, SwapChainInfo& swapChainInfo, PipelineInfo& pipelineInfo, CommandInfo& commandInfo, SyncObjects& syncObjects);

#ifndef NDEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = true;
#endif

int main() {

    VulkanContext context = {};
    SwapChainInfo swapChainInfo = {&context.swapChain};
    PipelineInfo pipelineInfo = {};
    CommandInfo commandInfo = {};
    SyncObjects syncObjects = {};

    try {

        initWindow(context);
        initVulkan(context, swapChainInfo, pipelineInfo, commandInfo, syncObjects);
        mainLoop(context, swapChainInfo, pipelineInfo, commandInfo, syncObjects);
        cleanup(context, swapChainInfo, pipelineInfo, commandInfo, syncObjects);
    }
    catch (const std::exception& e) {

        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void initWindow(VulkanContext& context) {

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    context.window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void initVulkan(VulkanContext& context, SwapChainInfo& swapChainInfo, PipelineInfo& pipelineInfo, CommandInfo& commandInfo, SyncObjects& syncObjects) {

    createInstance(context);
    setupDebugMessenger(context);
    createSurface(context);
    pickPhysicalDevice(context);
    createLogicalDevice(context);
    createSwapChain(context, swapChainInfo);
    createImageViews(context, swapChainInfo);
    createRenderPass(context, swapChainInfo, pipelineInfo);
    createGraphicsPipeline(context, pipelineInfo);
    createFramebuffers(context, swapChainInfo, pipelineInfo);
    createCommandPool(context, commandInfo);
    createCommandBuffer(context, commandInfo);
    createSyncObjects(context, syncObjects);
}

void createInstance(VulkanContext& context) {

    if (enableValidationLayers && !checkValidationLayerSupport()) {

        throw std::runtime_error("Validation layers request but not available");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {

        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(&debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {

        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &context.instance) != VK_SUCCESS) {

        throw std::runtime_error("failed to create instance!");
    }
}

std::vector<const char*> getRequiredExtensions() {

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {

        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool checkValidationLayerSupport() {

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());


    for (const char* layerName : validationLayers) {

        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {

            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {

            return false;
        }
    }
    return true;
}

void setupDebugMessenger(VulkanContext& context) {

    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(&createInfo);

    if (CreateDebugUtilsMessengerEXT(context.instance, &createInfo, nullptr, &context.debugMessenger) != VK_SUCCESS) {

        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* createInfo) {

    *createInfo = {};
    createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo->pfnUserCallback = debugCallback;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");  
    if (func != nullptr) {

        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {

        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {

    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {

        func(instance, debugMessenger, pAllocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(

    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

void createSurface(VulkanContext& context) {

    if (glfwCreateWindowSurface(context.instance, context.window, nullptr, &context.surface) != VK_SUCCESS) {

        throw std::runtime_error("failed to create window surface");
    }
}

void pickPhysicalDevice(VulkanContext& context) {

    context.physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(context.instance, &deviceCount, nullptr);
    if (deviceCount == 0) {

        throw std::runtime_error("failed to find GPUs w/ VK support");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(context.instance, &deviceCount, devices.data());

    for (const auto& device : devices) {

        // This is only checking compatibility, so there is no need to pass in a pointer
        if (isDeviceSuitable(device, context.surface)) {

            context.physicalDevice = device;
            break;
        }
    }
    if (context.physicalDevice == VK_NULL_HANDLE) {

        throw std::runtime_error("failed to find suitable GPU!");
    }
    
}

// Change this to prefer different GPUs later.
bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {

    QueueFamilyIndices indices = findQueueFamilies(device, surface);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    
    bool swapChainAdequate = false;
    if (extensionsSupported) {

        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(&device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }
    

    return indices.isComplete() && extensionsSupported;// && swapChainAdequate;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {

        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {

    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {

            indices.graphicsFamily = i;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport) {

            indices.presentFamily = i;
        }

        if (indices.isComplete()) {

            break;
        }

        i++;
    }
    return indices;
}

void createLogicalDevice(VulkanContext& context) {

    QueueFamilyIndices indices = findQueueFamilies(context.physicalDevice, context.surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {

        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {

        createInfo.enabledLayerCount = 0;
    }
    if (vkCreateDevice(context.physicalDevice, &createInfo, nullptr, &context.device) != VK_SUCCESS) {

        throw std::runtime_error("failed to create logical device");
    }
    vkGetDeviceQueue(context.device, indices.graphicsFamily.value(), 0, &context.graphicsQueue);
    vkGetDeviceQueue(context.device, indices.presentFamily.value(), 0, &context.presentQueue);
}

// Make sure the swap chain is available *AND* sufficient
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice* device, VkSurfaceKHR surface) {

    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(*device, surface, &formatCount, details.formats.data());

    if (formatCount != 0) {

        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(*device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(*device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {

        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(*device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {

    for (const auto& availableFormat : availableFormats) {

        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {

            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {

    for (const auto& availablePresentMode : availablePresentModes) {

        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {

            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(VulkanContext& context, const VkSurfaceCapabilitiesKHR& capabilities) {

    VkExtent2D actualExtent;

    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {

        return capabilities.currentExtent;
    }
    else {

        int width, height;
        glfwGetFramebufferSize(context.window, &width, &height);

        actualExtent = {

            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void createSwapChain(VulkanContext& context, SwapChainInfo& swapChainInfo) {

    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(&context.physicalDevice, context.surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(context, swapChainSupport.capabilities);

    // + 1 because you may have to wait on drive to fetch next image to render to for the minimum
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {

        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = context.surface;
    
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // If using post processing, change this to VK_IMAGE_USAGE_TRANSFER_DST_BIT perhaps

    QueueFamilyIndices indices = findQueueFamilies(context.physicalDevice, context.surface);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {

        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {

        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(context.device, &createInfo, nullptr, &context.swapChain) != VK_SUCCESS) {

        throw std::runtime_error("failed to create swap chain");
    }

    vkGetSwapchainImagesKHR(context.device, context.swapChain, &imageCount, nullptr);
    swapChainInfo.swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(context.device, context.swapChain, &imageCount, swapChainInfo.swapChainImages.data());

    swapChainInfo.swapChainImageFormat = surfaceFormat.format;
    swapChainInfo.swapChainExtent = extent;
}

void createImageViews(VulkanContext& context, SwapChainInfo& swapChainInfo) {

    swapChainInfo.swapChainImageViews.resize(swapChainInfo.swapChainImages.size());
    for (size_t i = 0; i < swapChainInfo.swapChainImages.size(); i++) {

        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainInfo.swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainInfo.swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(context.device, &createInfo, nullptr, &swapChainInfo.swapChainImageViews[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create image views");
        }
    }
}

void createRenderPass(VulkanContext& context, SwapChainInfo& swapChainInfo, PipelineInfo& pipelineInfo) {

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainInfo.swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // Change for multisampling
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Change this when I need stencil buffer
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Change stuff below this for subpasses/post-processing :)
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Wait for swap chain to finish reading from the image
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(context.device, &renderPassInfo, nullptr, &pipelineInfo.renderPass) != VK_SUCCESS) {

        throw std::runtime_error("failed to create render pass");
    }

}

void createGraphicsPipeline(VulkanContext& context, PipelineInfo& pipelineInfo) {

    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, context);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, context);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // Change this for data being per-vertex/per-instance
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    // This describes what kind of geometry will be drawn from vertices and if primitive restart should be enabled
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{}; 
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;
    
    // Depth/stencil testing here l8r

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // Viewport/Scissor will be dynamic and set in command buffer
    std::vector<VkDynamicState> dynamicStates = {

        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // This is for uniforms later as well
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(context.device, &pipelineLayoutInfo, nullptr, &pipelineInfo.pipelineLayout) != VK_SUCCESS) {

        throw std::runtime_error("failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo VkPipelineInfo{};
    VkPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    VkPipelineInfo.stageCount = 2;
    VkPipelineInfo.pStages = shaderStages;
    VkPipelineInfo.pVertexInputState = &vertexInputInfo;
    VkPipelineInfo.pInputAssemblyState = &inputAssembly;
    VkPipelineInfo.pViewportState = &viewportState;
    VkPipelineInfo.pRasterizationState = &rasterizer;
    VkPipelineInfo.pMultisampleState = &multisampling;
    VkPipelineInfo.pDepthStencilState = nullptr;
    VkPipelineInfo.pColorBlendState = &colorBlending;
    VkPipelineInfo.pDynamicState = &dynamicState;
    VkPipelineInfo.layout = pipelineInfo.pipelineLayout;
    VkPipelineInfo.renderPass = pipelineInfo.renderPass;
    VkPipelineInfo.subpass = 0;
    VkPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    VkPipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &VkPipelineInfo, nullptr, &pipelineInfo.graphicsPipeline) != VK_SUCCESS) {

        throw std::runtime_error("failed to create graphics pipeline");
    }

    vkDestroyShaderModule(context.device, fragShaderModule, nullptr);
    vkDestroyShaderModule(context.device, vertShaderModule, nullptr);
}

VkShaderModule createShaderModule(const std::vector<char>& code, VulkanContext& context) {

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(context.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {

        throw std::runtime_error("failed to create shader module");
    }

    return shaderModule;
}

std::vector<char> readFile(const std::string& filename) {
    std::cout << "Attempting to open file: " << filename << std::endl;

    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {

        throw std::runtime_error("failed to open file");
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

void createFramebuffers(VulkanContext& context, SwapChainInfo& swapChainInfo, PipelineInfo& pipelineInfo) {

    swapChainInfo.swapChainFramebuffers.resize(swapChainInfo.swapChainImageViews.size());

    // Must make a framebuffer for each image view
    for (size_t i = 0; i < swapChainInfo.swapChainImageViews.size(); i++) {

        VkImageView attachments[] = {

            swapChainInfo.swapChainImageViews[i]
        };
        
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = pipelineInfo.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainInfo.swapChainExtent.width;
        framebufferInfo.height = swapChainInfo.swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(context.device, &framebufferInfo, nullptr, &swapChainInfo.swapChainFramebuffers[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create frame buffer");
        }
    }
}

void createCommandPool(VulkanContext& context, CommandInfo& commandInfo) {

    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(context.physicalDevice, context.surface);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(context.device, &poolInfo, nullptr, &commandInfo.commandPool) != VK_SUCCESS) {

        throw std::runtime_error("failed to create command pool");
    }
}

void createCommandBuffer(VulkanContext& context, CommandInfo& commandInfo) {

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandInfo.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(context.device, &allocInfo, &commandInfo.commandBuffer) != VK_SUCCESS) {

        throw std::runtime_error("failed to allocate command buffers");
    }
}

void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, PipelineInfo& pipelineInfo, CommandInfo& commandInfo, SwapChainInfo& swapChainInfo) {

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {

        throw std::runtime_error("failed to begin recording command buffer");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = pipelineInfo.renderPass;
    renderPassInfo.framebuffer = swapChainInfo.swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0,0 };
    renderPassInfo.renderArea.extent = swapChainInfo.swapChainExtent;

    VkClearValue clearColor = { { {0.0f, 0.0f, 0.0f, 1.0f} } };
    renderPassInfo.clearValueCount = 3;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInfo.graphicsPipeline);
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainInfo.swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainInfo.swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainInfo.swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {

        throw std::runtime_error("failed to record command buffer");
    }
}


void createSyncObjects(VulkanContext& context, SyncObjects& syncObjects) {

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // This creates it unsignaled so the first waitForFences isn't infinitely long

    if (vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &syncObjects.imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &syncObjects.renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(context.device, &fenceInfo, nullptr, &syncObjects.inFlightFence) != VK_SUCCESS) {

        throw std::runtime_error("failed to create semaphores/fence");
    }
}

// Wait for previous frame to finish -> Acquire an image from the swap chain -> Record a command buffer which draws the scene onto that image -> Submit the reocrded command buffer -> Present the swap chain image
// Semaphores are for GPU synchronization, Fences are for CPU
void drawFrame(VulkanContext& context, SwapChainInfo& swapChainInfo, CommandInfo& commandInfo, SyncObjects& syncObjects, PipelineInfo& pipelineInfo) {

    // Make CPU wait until the GPU is done.
    vkWaitForFences(context.device, 1, &syncObjects.inFlightFence, VK_TRUE, UINT64_MAX);
    // Reset fence to unsignaled state
    vkResetFences(context.device, 1, &syncObjects.inFlightFence);

    uint32_t imageIndex;
    // This tells the imageAvailableSemaphore to be signaled when done.
    vkAcquireNextImageKHR(context.device, *swapChainInfo.swapChain, UINT64_MAX, syncObjects.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    // Record command buffer then submit info to it
    vkResetCommandBuffer(commandInfo.commandBuffer, 0);
    recordCommandBuffer(commandInfo.commandBuffer, imageIndex, pipelineInfo, commandInfo, swapChainInfo);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { syncObjects.imageAvailableSemaphore }; // Wait on this before execution begins
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // This tells in which stage of pipeline to wait
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    // These two tell which command buffers to submit for execution
    submitInfo.commandBufferCount = 1; 
    submitInfo.pCommandBuffers = &commandInfo.commandBuffer;

    // So it knows which semaphores to signal once command buffers are done
    VkSemaphore signalSemaphores[] = { syncObjects.renderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Last parameter is what signals the fence when command buffers finish
    if (vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, syncObjects.inFlightFence) != VK_SUCCESS) {

        throw std::runtime_error("failed to submit draw command buffer");
    }

    // Submit result back to the swap chain and show it on the screen finally
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // Wait on this semaphore before presentation can happen (renderFinishedSemaphore)

    VkSwapchainKHR swapChains[] = { *swapChainInfo.swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    vkQueuePresentKHR(context.presentQueue, &presentInfo);

}

void mainLoop(VulkanContext& context, SwapChainInfo& swapChainInfo, PipelineInfo& pipelineInfo, CommandInfo& commandInfo, SyncObjects& syncObjects) {

    while (!glfwWindowShouldClose(context.window)) {

        glfwPollEvents();
        drawFrame(context, swapChainInfo, commandInfo, syncObjects, pipelineInfo);
    }

    vkDeviceWaitIdle(context.device); // Wait for logical device to finish before exiting the loop
}

void cleanup(VulkanContext& context, SwapChainInfo& swapChainInfo, PipelineInfo& pipelineInfo, CommandInfo& commandInfo, SyncObjects& syncObjects) {

    vkDestroySemaphore(context.device, syncObjects.imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(context.device, syncObjects.renderFinishedSemaphore, nullptr);
    vkDestroyFence(context.device, syncObjects.inFlightFence, nullptr);

    vkDestroyCommandPool(context.device, commandInfo.commandPool, nullptr);

    for (auto framebuffer : swapChainInfo.swapChainFramebuffers) {

        vkDestroyFramebuffer(context.device, framebuffer, nullptr);
    }

    vkDestroyPipeline(context.device, pipelineInfo.graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(context.device, pipelineInfo.pipelineLayout, nullptr);
    vkDestroyRenderPass(context.device, pipelineInfo.renderPass, nullptr);

    for (auto imageView : swapChainInfo.swapChainImageViews) {

        vkDestroyImageView(context.device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(context.device, context.swapChain, nullptr);

    vkDestroyDevice(context.device, nullptr);

    if (enableValidationLayers) {

        DestroyDebugUtilsMessengerEXT(context.instance, context.debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
    vkDestroyInstance(context.instance, nullptr);

    glfwDestroyWindow(context.window);

    glfwTerminate();
}