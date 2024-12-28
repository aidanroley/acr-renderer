#include "../precompile/pch.h"

#include "../include/init.h"
#include "../include/file_funcs.h"
#include "../include/graphics_setup.h"


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" 

// fix this later
#ifndef NDEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = true;
#endif

void initVulkan(VulkanSetup& setup, GraphicsSetup& graphics) {

    createInstance(*setup.context);
    setupDebugMessenger(*setup.context);
    createSurface(*setup.context);
    pickPhysicalDevice(*setup.context);
    createLogicalDevice(*setup.context);
    createSwapChain(*setup.context, *setup.swapChainInfo);
    createImageViews(*setup.context, *setup.swapChainInfo);
    createRenderPass(*setup.context, *setup.swapChainInfo, *setup.pipelineInfo);
    createDescriptorSetLayout(*setup.context, *setup.pipelineInfo);
    createGraphicsPipeline(*setup.context, *setup.pipelineInfo);
    createCommandPool(*setup.context, *setup.commandInfo);
    createColorResources(*setup.context, *setup.swapChainInfo, *setup.pixelInfo);
    createDepthResources(*setup.context, *setup.swapChainInfo, *setup.depthInfo);
    createFramebuffers(*setup.context, *setup.swapChainInfo, *setup.pipelineInfo, *setup.depthInfo, *setup.pixelInfo);
    createTextureImage(*setup.context, *setup.commandInfo, *setup.textureData);
    createTextureImageView(*setup.context, *setup.textureData);
    createTextureSampler(*setup.context, *setup.textureData);
    createVertexBuffer(*setup.context, *setup.pipelineInfo, *setup.commandInfo, *graphics.vertexData);
    createIndexBuffer(*setup.context, *setup.pipelineInfo, *setup.commandInfo, *graphics.vertexData);
    createUniformBuffers(*setup.context, *setup.uniformData);
    createDesciptorPool(*setup.context, *setup.uniformData);
    createDescriptorSets(*setup.context, *setup.uniformData, *setup.pipelineInfo, *setup.textureData);
    createCommandBuffers(*setup.context, *setup.commandInfo);
    createSyncObjects(*setup.context, *setup.syncObjects);
}

void createInstance(VulkanContext& context) {

    if (enableValidationLayers && !checkValidationLayerSupport()) {

        throw std::runtime_error("Validation layers request but not available");
    }

    // I'm not using C++20 so I can't do something like VKApplicationInfo = { .sType = ...};
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
            context.msaaSamples = getMaxUsableSampleCount(context);
            break;
        }
    }
    if (context.physicalDevice == VK_NULL_HANDLE) {

        throw std::runtime_error("failed to find suitable GPU!");
    }
}

VkSampleCountFlagBits getMaxUsableSampleCount(VulkanContext& context) {

    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(context.physicalDevice, &physicalDeviceProperties);
    
    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
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
    
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
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
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.sampleRateShading = VK_TRUE;

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

        if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {//VK_PRESENT_MODE_MAILBOX_KHR) { // disable vsync

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

    // create swap chain
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
    colorAttachment.samples = context.msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Change this when I need stencil buffer
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Change stuff below this for subpasses/post-processing :)
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat(context);
    depthAttachment.samples = context.msaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = swapChainInfo.swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; // Wait for swap chain to finish reading from the image
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    
    std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(context.device, &renderPassInfo, nullptr, &pipelineInfo.renderPass) != VK_SUCCESS) {

        throw std::runtime_error("failed to create render pass");
    }
}

void createDescriptorSetLayout(VulkanContext& context, PipelineInfo& pipelineInfo) {

    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;


    // so shaders can access image through sampler object
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    
    if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &pipelineInfo.descriptorSetLayout) != VK_SUCCESS) {

        throw std::runtime_error("failed to create descriptor set layout");
    }
}

void createGraphicsPipeline(VulkanContext& context, PipelineInfo& pipelineInfo) {

    auto vertShaderCode = readFile("shaders/shaderCompilation/Cornell-Box-Phong.spv");
    auto fragShaderCode = readFile("shaders/shaderCompilation/Cornell-Box-Diffuse.spv");

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

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = context.msaaSamples;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;
    
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS; // lower depth is closer
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;
    multisampling.sampleShadingEnable = VK_TRUE; 
    multisampling.minSampleShading = .2f;
    depthStencil.front = {}; 
    depthStencil.back = {};

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
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &pipelineInfo.descriptorSetLayout;
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
    VkPipelineInfo.pDepthStencilState = &depthStencil;
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

void createFramebuffers(VulkanContext& context, SwapChainInfo& swapChainInfo, PipelineInfo& pipelineInfo, DepthInfo& depthInfo, PixelInfo& pixelInfo) {

    swapChainInfo.swapChainFramebuffers.resize(swapChainInfo.swapChainImageViews.size());

    // Must make a framebuffer for each image view
    for (size_t i = 0; i < swapChainInfo.swapChainImageViews.size(); i++) {

        std::array<VkImageView, 3> attachments = {

            pixelInfo.colorImageView,
            depthInfo.depthImageView,
            swapChainInfo.swapChainImageViews[i]
        };
            
            
        
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = pipelineInfo.renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
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

void createColorResources(VulkanContext& context, SwapChainInfo& swapChainInfo, PixelInfo& pixelInfo) {

    VkFormat colorFormat = swapChainInfo.swapChainImageFormat;

    createImage(context, swapChainInfo.swapChainExtent.width, swapChainInfo.swapChainExtent.height, colorFormat, VK_IMAGE_TILING_OPTIMAL, context.msaaSamples,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pixelInfo.colorImage, pixelInfo.colorImageMemory, 1);
    pixelInfo.colorImageView = createImageView(context, pixelInfo.colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void createDepthResources(VulkanContext& context, SwapChainInfo& swapChainInfo, DepthInfo& depthInfo) {

    VkFormat depthFormat = findDepthFormat(context);

    createImage(context, swapChainInfo.swapChainExtent.width, swapChainInfo.swapChainExtent.height, depthFormat,
        VK_IMAGE_TILING_OPTIMAL, context.msaaSamples, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        depthInfo.depthImage, depthInfo.depthImageMemory, 1);
    depthInfo.depthImageView = createImageView(context, depthInfo.depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VulkanContext& context) {

    for (VkFormat format : candidates) {

        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(context.physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {

            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {

            return format;
        }
    } 

    throw std::runtime_error("failed to find supported format");
}

VkFormat findDepthFormat(VulkanContext& context) {

    return findSupportedFormat(

        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
        context
    );
}

bool hasStencilComponent(VkFormat format) {

    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void createTextureImage(VulkanContext& context, CommandInfo& commandInfo, TextureData& textureData) {

    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4; // RGBA (size of image in bytes)
    textureData.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    if (!pixels) {

        throw std::runtime_error("failed to load texture image");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, context);

    void* data;
    vkMapMemory(context.device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(context.device, stagingBufferMemory);

    stbi_image_free(pixels);

    // must be src and dst since blit requires the texture to be a source in a transfer operation
    createImage(context, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureData.textureImage, textureData.textureImageMemory, textureData.mipLevels);
    
    transitionImageLayout(textureData.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, textureData.mipLevels, context, commandInfo);
    copyBufferToImage(stagingBuffer, textureData.textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), context, commandInfo);

    vkDestroyBuffer(context.device, stagingBuffer, nullptr);
    vkFreeMemory(context.device, stagingBufferMemory, nullptr);

    generateMipmaps(textureData.textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, textureData.mipLevels, context, commandInfo);
}

void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, VulkanContext& context, CommandInfo& commandInfo) {

    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(context.physicalDevice, imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {

        throw std::runtime_error("texture image format does not support linear blitting");
    }

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(context, commandInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {

        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);
        
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // transition it to read only since that was the last mipmap created
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    endSingleTimeCommands(commandBuffer, context, commandInfo);
}

void createTextureImageView(VulkanContext& context, TextureData& textureData) {

    textureData.textureImageView = createImageView(context, textureData.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, textureData.mipLevels);
}

VkImageView createImageView(VulkanContext& context, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(context.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {

        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void createTextureSampler(VulkanContext& context, TextureData& textureData) {

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(context.physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(textureData.mipLevels);

    if (vkCreateSampler(context.device, &samplerInfo, nullptr, &textureData.textureSampler) != VK_SUCCESS) {

        throw std::runtime_error("failed to create texture sampler");
    }

}

void createImage(VulkanContext& context, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkSampleCountFlagBits numSamples,
    VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t mipLevels) {

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(context.device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(context.device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(context, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(context.device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(context.device, image, imageMemory, 0);
}

void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, VulkanContext& context, CommandInfo& commandInfo) {

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(context, commandInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = 0;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    // transfer writes that dont need to wait on anything
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {

        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    // 
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {

        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {

        throw std::invalid_argument("unsupported layout transition");
    }

    vkCmdPipelineBarrier(

        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer, context, commandInfo);
}

void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VulkanContext& context, CommandInfo& commandInfo) {

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(context, commandInfo);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {

        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(

        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    ); 

    endSingleTimeCommands(commandBuffer, context, commandInfo);
}

void createVertexBuffer(VulkanContext& context, PipelineInfo& pipelineInfo, CommandInfo& commandInfo, VertexData& vertexData) {

    VkDeviceSize bufferSize = sizeof(vertexData.vertices[0]) * vertexData.vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, context);

    // Copy vertex data to the staging buffer
    void* data;
    vkMapMemory(context.device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertexData.vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(context.device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pipelineInfo.vertexBuffer, pipelineInfo.vertexBufferMemory, context);

    copyBuffer(stagingBuffer, pipelineInfo.vertexBuffer, bufferSize, context, commandInfo);

    vkDestroyBuffer(context.device, stagingBuffer, nullptr);
    vkFreeMemory(context.device, stagingBufferMemory, nullptr);
}

void createIndexBuffer(VulkanContext& context, PipelineInfo& pipelineInfo, CommandInfo& commandInfo, VertexData& vertexData) {

    VkDeviceSize bufferSize = sizeof(vertexData.indices[0]) * vertexData.indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, context);

    void* data;
    vkMapMemory(context.device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertexData.indices.data(), (size_t)bufferSize);
    vkUnmapMemory(context.device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pipelineInfo.indexBuffer, pipelineInfo.indexBufferMemory, context);

    copyBuffer(stagingBuffer, pipelineInfo.indexBuffer, bufferSize, context, commandInfo);

    vkDestroyBuffer(context.device, stagingBuffer, nullptr);
    vkFreeMemory(context.device, stagingBufferMemory, nullptr);
}

void createUniformBuffers(VulkanContext& context, UniformData& uniformData) {

    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformData.uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformData.uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformData.uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        uniformData.uniformBuffers[i], uniformData.uniformBuffersMemory[i], context); 

        vkMapMemory(context.device, uniformData.uniformBuffersMemory[i], 0, bufferSize, 0, &uniformData.uniformBuffersMapped[i]);
    }
}

void createDesciptorPool(VulkanContext& context, UniformData& uniformData) {

    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(context.device, &poolInfo, nullptr, &uniformData.descriptorPool) != VK_SUCCESS) {

        throw std::runtime_error("failed to create descriptor pool");
    }
}

void createDescriptorSets(VulkanContext& context, UniformData& uniformData, PipelineInfo& pipelineInfo, TextureData& textureData) {

    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, pipelineInfo.descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = uniformData.descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    uniformData.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(context.device, &allocInfo, uniformData.descriptorSets.data()) != VK_SUCCESS) {
        
        throw std::runtime_error("failed to allocate descriptor sets");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformData.uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureData.textureImageView;
        imageInfo.sampler = textureData.textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = uniformData.descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = uniformData.descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

}

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VulkanContext& context) {

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(context.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {

        throw std::runtime_error("failed to create buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(context.device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(context, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(context.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(context.device, buffer, bufferMemory, 0);
}

uint32_t findMemoryType(VulkanContext& context, uint32_t typeFilter, VkMemoryPropertyFlags properties) {

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(context.physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {

        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {

            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type");
}

VkCommandBuffer beginSingleTimeCommands(VulkanContext& context, CommandInfo& commandInfo) {

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandInfo.commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(context.device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(VkCommandBuffer commandBuffer, VulkanContext& context, CommandInfo& commandInfo) {

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(context.graphicsQueue);

    vkFreeCommandBuffers(context.device, commandInfo.commandPool, 1, &commandBuffer);
}

void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VulkanContext& context, CommandInfo& commandInfo) {

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(context, commandInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer, context, commandInfo);
}

void createCommandBuffers(VulkanContext& context, CommandInfo& commandInfo) {

    commandInfo.commandBuffers.resize(MAX_FRAMES_IN_FLIGHT); // 2 for double buffering

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandInfo.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandInfo.commandBuffers.size();

    if (vkAllocateCommandBuffers(context.device, &allocInfo, commandInfo.commandBuffers.data()) != VK_SUCCESS) {

        throw std::runtime_error("failed to allocate command buffers");
    }
}

void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, PipelineInfo& pipelineInfo, CommandInfo& commandInfo, SwapChainInfo& swapChainInfo, UniformData& uniformData, SyncObjects& syncObjects, VertexData& vertexData) {

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

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 }; // Max depth value
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

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

        VkBuffer vertexBuffers[] = { pipelineInfo.vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, pipelineInfo.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInfo.pipelineLayout, 0, 1, &uniformData.descriptorSets[syncObjects.currentFrame], 0, nullptr);

        //vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(vertexData.indices.size()), 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {

        throw std::runtime_error("failed to record command buffer");
    }
}

void createSyncObjects(VulkanContext& context, SyncObjects& syncObjects) {

    syncObjects.imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    syncObjects.renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    syncObjects.inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // This creates it unsignaled so the first waitForFences isn't infinitely long

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        if (vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &syncObjects.imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &syncObjects.renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(context.device, &fenceInfo, nullptr, &syncObjects.inFlightFences[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create semaphores/fence");
        }
    }
}

void cleanupSwapChain(VulkanContext& context, SwapChainInfo& swapChainInfo, DepthInfo& depthInfo, PixelInfo& pixelInfo) {

    vkDestroyImageView(context.device, pixelInfo.colorImageView, nullptr);
    vkDestroyImage(context.device, pixelInfo.colorImage, nullptr);
    vkFreeMemory(context.device, pixelInfo.colorImageMemory, nullptr);

    vkDestroyImageView(context.device, depthInfo.depthImageView, nullptr);
    vkDestroyImage(context.device, depthInfo.depthImage, nullptr);
    vkFreeMemory(context.device, depthInfo.depthImageMemory, nullptr);

    for (size_t i = 0; i < swapChainInfo.swapChainFramebuffers.size(); i++) {

        vkDestroyFramebuffer(context.device, swapChainInfo.swapChainFramebuffers[i], nullptr);
    }

    for (size_t i = 0; i < swapChainInfo.swapChainImageViews.size(); i++) {

        vkDestroyImageView(context.device, swapChainInfo.swapChainImageViews[i], nullptr);
    }
    
    vkDestroySwapchainKHR(context.device, *swapChainInfo.swapChain, nullptr);
}

void cleanupVkObjects(VulkanSetup& setup) {

    cleanupSwapChain(*setup.context, *setup.swapChainInfo, *setup.depthInfo, *setup.pixelInfo);

    vkDestroySampler(setup.context->device, setup.textureData->textureSampler, nullptr);
    vkDestroyImageView(setup.context->device, setup.textureData->textureImageView, nullptr);

    vkDestroyImage(setup.context->device, setup.textureData->textureImage, nullptr);
    vkFreeMemory(setup.context->device, setup.textureData->textureImageMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        vkDestroyBuffer(setup.context->device, setup.uniformData->uniformBuffers[i], nullptr);
        vkFreeMemory(setup.context->device, setup.uniformData->uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(setup.context->device, setup.uniformData->descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(setup.context->device, setup.pipelineInfo->descriptorSetLayout, nullptr);

    vkDestroyBuffer(setup.context->device, setup.pipelineInfo->indexBuffer, nullptr);
    vkFreeMemory(setup.context->device, setup.pipelineInfo->indexBufferMemory, nullptr);

    vkDestroyBuffer(setup.context->device, setup.pipelineInfo->vertexBuffer, nullptr); 
    vkFreeMemory(setup.context->device, setup.pipelineInfo->vertexBufferMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        vkDestroySemaphore(setup.context->device, setup.syncObjects->renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(setup.context->device, setup.syncObjects->imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(setup.context->device, setup.syncObjects->inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(setup.context->device, setup.commandInfo->commandPool, nullptr);

    vkDestroyPipeline(setup.context->device, setup.pipelineInfo->graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(setup.context->device, setup.pipelineInfo->pipelineLayout, nullptr);
    vkDestroyRenderPass(setup.context->device, setup.pipelineInfo->renderPass, nullptr);

    vkDestroyDevice(setup.context->device, nullptr);

    if (enableValidationLayers) {

        DestroyDebugUtilsMessengerEXT(setup.context->instance, setup.context->debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(setup.context->instance, setup.context->surface, nullptr);
    vkDestroyInstance(setup.context->instance, nullptr);

    glfwDestroyWindow(setup.context->window);

    glfwTerminate();
}