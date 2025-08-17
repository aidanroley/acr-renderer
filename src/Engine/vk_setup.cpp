#include "pch.h"
#include "Engine/engine.h"
#include "Misc/file_funcs.h"
#include "Renderer/renderer_setup.h"
#include "Engine/vk_helper_funcs.h"
#include "Texture/texture_utils.h"
#include "VkBootstrap.h"
#include "Engine/vk_setup.h"
#include "Misc/logger.h"


bool PBR_ENABLED = true;

namespace VulkanSetup {

    void bootstrapVk(VkEngine* engine) {

        vkb::InstanceBuilder builder;

        auto instance_ret = builder.set_app_name("Vulkan App")
            .request_validation_layers(true)
            .use_default_debug_messenger()
            .require_api_version(1, 3, 0)
            .build();

        if (!instance_ret) {

            throw std::runtime_error("boostrap failed");
        }

        vkb::Instance vkb_inst = instance_ret.value();

        engine->instance = vkb_inst.instance;
        engine->debugMessenger = vkb_inst.debug_messenger;

        //set up surface
        createSurface(engine);

        // set up device
        VkPhysicalDeviceVulkan13Features features{};
        features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        features.dynamicRendering = true;
        features.synchronization2 = true;


        VkPhysicalDeviceVulkan12Features features12{};
        features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        features12.bufferDeviceAddress = true;
        features12.descriptorIndexing = true;

        VkPhysicalDeviceFeatures legacyFeatures{};
        legacyFeatures.samplerAnisotropy = VK_TRUE;
        legacyFeatures.sampleRateShading = VK_TRUE;

        vkb::PhysicalDeviceSelector selector{ vkb_inst };
        vkb::PhysicalDevice physicalDeviceVkb = selector
            .set_minimum_version(1, 3)
            .set_required_features_13(features)
            .set_required_features_12(features12)
            .set_required_features(legacyFeatures)
            .set_surface(engine->surface)
            .select()
            .value();

        vkb::DeviceBuilder deviceBuilder{ physicalDeviceVkb };
        vkb::Device vkbDevice = deviceBuilder.build().value();

        engine->device = vkbDevice.device;
        engine->physicalDevice = physicalDeviceVkb.physical_device;

        engine->graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        engine->presentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();
    }

    void initAllocator(VkEngine* engine) {

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = engine->physicalDevice;
        allocatorInfo.device = engine->device;
        allocatorInfo.instance = engine->instance;
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        vmaCreateAllocator(&allocatorInfo, &engine->_allocator);
    }

    void initDefaultValues(VkEngine* engine) {

        // default samplers
        VkSamplerCreateInfo sampler = {};
        sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler.magFilter = VK_FILTER_NEAREST;
        sampler.minFilter = VK_FILTER_NEAREST;
        vkCreateSampler(engine->device, &sampler, nullptr, &engine->_defaultSamplerNearest);

        sampler.magFilter = VK_FILTER_LINEAR;
        sampler.minFilter = VK_FILTER_LINEAR;
        vkCreateSampler(engine->device, &sampler, nullptr, &engine->_defaultSamplerLinear);
    }

    void initDefaultImages(VkEngine* engine) {

        uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
        engine->_whiteImage = engine->createImage((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

        uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
        engine->_blackImage = engine->createImage((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

        //checkerboard image
        uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
        std::array<uint32_t, 16 * 16 > pixels; //for 16x16 checkerboard texture
        for (int x = 0; x < 16; x++) {

            for (int y = 0; y < 16; y++) {

                pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
            }
        }

        engine->_errorImage = engine->createImage(pixels.data(), VkExtent3D{ 16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
    }

    void createSurface(VkEngine* engine) {

        Logger::vkCheck(glfwCreateWindowSurface(engine->instance, engine->window, nullptr, &engine->surface), "failed to create window surface");
    }

    void createSwapChain(VkEngine* engine) {

        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(&engine->physicalDevice, engine->surface);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(engine->window, swapChainSupport.capabilities);

        // + 1 because you may have to wait on drive to fetch next image to render to for the minimum
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {

            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        // create swap chain
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = engine->surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // If using post processing, change this to VK_IMAGE_USAGE_TRANSFER_DST_BIT perhaps

        QueueFamilyIndices indices = findQueueFamilies(engine->physicalDevice, engine->surface);
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

        Logger::vkCheck(vkCreateSwapchainKHR(engine->device, &createInfo, nullptr, &engine->swapChain), "failed to create swap chain");

        std::cout << "Swapchain handle: " << engine->swapChain << std::endl;

        vkGetSwapchainImagesKHR(engine->device, engine->swapChain, &imageCount, nullptr);
        engine->swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(engine->device, engine->swapChain, &imageCount, engine->swapChainImages.data());

        engine->swapChainImageFormat = surfaceFormat.format;
        engine->swapChainExtent = extent;
    }

    void createImageViews(VkEngine* engine) {

        engine->swapChainImageViews.resize(engine->swapChainImages.size());
        for (size_t i = 0; i < engine->swapChainImages.size(); i++) {

            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = engine->swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = engine->swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            Logger::vkCheck(vkCreateImageView(engine->device, &createInfo, nullptr, &engine->swapChainImageViews[i]), "failed to create image views");
        }
    }

    void createRenderPass(VkEngine* engine) {

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = engine->swapChainImageFormat;
        colorAttachment.samples = engine->msaaSamples;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat(engine->physicalDevice);
        depthAttachment.samples = engine->msaaSamples;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription resolveAttachment{};
        resolveAttachment.format = engine->swapChainImageFormat;
        resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        std::array<VkAttachmentDescription, 3> attachments = {
            colorAttachment,
            depthAttachment,
            resolveAttachment
        };

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthRef{};
        depthRef.attachment = 1;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference resolveRef{};
        resolveRef.attachment = 2;
        resolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pResolveAttachments = &resolveRef;
        subpass.pDepthStencilAttachment = &depthRef;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        Logger::vkCheck(vkCreateRenderPass(engine->device, &renderPassInfo, nullptr, &engine->renderPass), "failed to create render pass");
    }

    void initCameraDescriptorSetLayout(VkEngine* engine) {

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        VkDescriptorSetLayoutBinding cameraBinding = engine->descriptorManager->createLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0);
        bindings.push_back(cameraBinding);
        engine->descriptorManager->createDescriptorLayout(bindings, engine->descriptorManager->_descriptorSetLayoutCamera);
    }

    void createDescriptorSetLayouts(VkEngine* engine) {

        if (PBR_ENABLED) {

            engine->pbrSystem.initDescriptorSetLayouts();
        }
        initCameraDescriptorSetLayout(engine);
    }

    void createGraphicsPipeline(VkEngine* engine) {

        auto vertShaderCode = readFile("shaders/shaderCompilation/Shader-Vert.spv");
        auto fragShaderCode = readFile("shaders/shaderCompilation/Shader-Frag.spv");
        deleteAllExceptCompileBat("shaders/shaderCompilation/Sun-Temple-Vert.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, engine->device);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, engine->device);

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
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //LINE
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // BACK_BIT
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //I MPORTANT
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = engine->msaaSamples;
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

        VkDescriptorSetLayout setLayouts[] = { engine->descriptorManager->_descriptorSetLayoutCamera, engine->pbrSystem._descriptorSetLayoutMat };

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(glm::mat4);

        // This is for uniforms later as well
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 2;
        pipelineLayoutInfo.pSetLayouts = setLayouts;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        Logger::vkCheck(vkCreatePipelineLayout(engine->device, &pipelineLayoutInfo, nullptr, &engine->pipelines.layout), "failed to create pipeline layout");

        VkGraphicsPipelineCreateInfo opaqueInfo{};
        opaqueInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        opaqueInfo.stageCount = 2;
        opaqueInfo.pStages = shaderStages;
        opaqueInfo.pVertexInputState = &vertexInputInfo;
        opaqueInfo.pInputAssemblyState = &inputAssembly;
        opaqueInfo.pViewportState = &viewportState;
        opaqueInfo.pRasterizationState = &rasterizer;
        opaqueInfo.pMultisampleState = &multisampling;
        opaqueInfo.pDepthStencilState = &depthStencil;
        opaqueInfo.pColorBlendState = &colorBlending;
        opaqueInfo.pDynamicState = &dynamicState;
        opaqueInfo.layout = engine->pipelines.layout;
        opaqueInfo.renderPass = engine->renderPass;
        opaqueInfo.subpass = 0;
        opaqueInfo.basePipelineHandle = VK_NULL_HANDLE;
        opaqueInfo.basePipelineIndex = -1;

        Logger::vkCheck(vkCreateGraphicsPipelines(engine->device, VK_NULL_HANDLE, 1, &opaqueInfo, nullptr, &engine->pipelines.opaque), "failed to create graphics pipeline");

        VkGraphicsPipelineCreateInfo trInfo = opaqueInfo; // copy it and modify the ones for transparent.
        VkPipelineColorBlendAttachmentState trBlend = *opaqueInfo.pColorBlendState->pAttachments;
        VkPipelineDepthStencilStateCreateInfo trDS = *opaqueInfo.pDepthStencilState;

        trBlend.blendEnable = VK_TRUE;
        trBlend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        trBlend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        trBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        trBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        trDS.depthWriteEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo trCB = *opaqueInfo.pColorBlendState;
        trCB.pAttachments = &trBlend;

        trInfo.pColorBlendState = &trCB;
        trInfo.pDepthStencilState = &trDS;

        vkCreateGraphicsPipelines(engine->device, VK_NULL_HANDLE, 1, &trInfo, nullptr, &engine->pipelines.transparent);

        vkDestroyShaderModule(engine->device, fragShaderModule, nullptr);
        vkDestroyShaderModule(engine->device, vertShaderModule, nullptr);
    }

    void createCommandPool(VkEngine* engine) {

        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(engine->physicalDevice, engine->surface);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        Logger::vkCheck(vkCreateCommandPool(engine->device, &poolInfo, nullptr, &engine->commandPool), "failed to create command pool");
    }

    // chang ethese Lol
    void createColorResources(VkEngine* engine) {

        VkFormat colorFormat = engine->swapChainImageFormat;

        createImageNonVMA(engine->physicalDevice, engine->device, engine->swapChainExtent.width, engine->swapChainExtent.height, colorFormat, VK_IMAGE_TILING_OPTIMAL, engine->msaaSamples,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, engine->colorImage, engine->colorImageMemory, 1);
        engine->colorImageView = createImageView(engine->device, engine->colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    void createDepthResources(VkEngine* engine) {

        VkFormat depthFormat = findDepthFormat(engine->physicalDevice);

        createImageNonVMA(engine->physicalDevice, engine->device, engine->swapChainExtent.width, engine->swapChainExtent.height, depthFormat,
            VK_IMAGE_TILING_OPTIMAL, engine->msaaSamples, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            engine->depthImage, engine->depthImageMemory, 1);

        //depthImage = createImage(swapChainExtent, depthFormat, 1);
        engine->depthImageView = createImageView(engine->device, engine->depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }



    void createFramebuffers(VkEngine* engine) {

        engine->swapChainFramebuffers.resize(engine->swapChainImageViews.size());

        // Must make a framebuffer for each image view
        for (size_t i = 0; i < engine->swapChainImageViews.size(); i++) {

            std::array<VkImageView, 3> attachments = {

                engine->colorImageView,
                engine->depthImageView,
                engine->swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = engine->renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = engine->swapChainExtent.width;
            framebufferInfo.height = engine->swapChainExtent.height;
            framebufferInfo.layers = 1;

            Logger::vkCheck(vkCreateFramebuffer(engine->device, &framebufferInfo, nullptr, &engine->swapChainFramebuffers[i]), "failed to create frame buffer");
        }
    }

    void createUniformBuffers(VkEngine* engine) {

        VkDeviceSize bufferSize = sizeof(FrameUBO);

        engine->uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        engine->uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        engine->uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                engine->uniformBuffers[i], engine->uniformBuffersMemory[i], engine->device, engine->physicalDevice);

            vkMapMemory(engine->device, engine->uniformBuffersMemory[i], 0, bufferSize, 0, &engine->uniformBuffersMapped[i]);
        }
    }

    //rename this func
    void createDescriptorPools(VkEngine* engine) {

        engine->descriptorManager->initDescriptorPool();
        engine->descriptorManager->initDescriptorSets();
        engine->descriptorManager->initCameraDescriptor();
        //descriptorManager.writeSamplerDescriptor();
    }

    void createCommandBuffers(VkEngine* engine) {

        engine->commandBuffers.resize(MAX_FRAMES_IN_FLIGHT); // 2 for double buffering

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = engine->commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)engine->commandBuffers.size();

        Logger::vkCheck(vkAllocateCommandBuffers(engine->device, &allocInfo, engine->commandBuffers.data()), "failed to allocate command buffers");

        //abstract this pool stuff later
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(engine->physicalDevice, engine->surface);
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        vkCreateCommandPool(engine->device, &poolInfo, nullptr, &engine->immCommandPool);

        // imm command buff alloc
        VkCommandBufferAllocateInfo cmdInfo = {};
        cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdInfo.pNext = nullptr;
        cmdInfo.commandPool = engine->immCommandPool;
        cmdInfo.commandBufferCount = 1;
        cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        vkAllocateCommandBuffers(engine->device, &cmdInfo, &engine->immCommandBuffer);
    }

    void createSyncObjects(VkEngine* engine) {

        //start with imm fences 
        VkFenceCreateInfo fenceInfoImm{};
        fenceInfoImm.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfoImm.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(engine->device, &fenceInfoImm, nullptr, &engine->immFence);
        // cleanuup thing here perhaps later.

        engine->imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        engine->renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        engine->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // This creates it unsignaled so the first waitForFences isn't infinitely long

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            if (vkCreateSemaphore(engine->device, &semaphoreInfo, nullptr, &engine->imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(engine->device, &semaphoreInfo, nullptr, &engine->renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(engine->device, &fenceInfo, nullptr, &engine->inFlightFences[i]) != VK_SUCCESS) {

                throw std::runtime_error("failed to create semaphores/fence");
            }
        }
    }

    void init(VkEngine* engine) {

        bootstrapVk(engine);
        initAllocator(engine); // This needs physicalDevice, device, instance to be called
        initDefaultValues(engine);

        createSwapChain(engine);
        createImageViews(engine);
        createRenderPass(engine);
        createDescriptorSetLayouts(engine);
        createGraphicsPipeline(engine);
        createCommandPool(engine);
        createColorResources(engine);
        createDepthResources(engine);
        createFramebuffers(engine);
        createUniformBuffers(engine);
        createDescriptorPools(engine);
        createCommandBuffers(engine);
        createSyncObjects(engine);
        initDefaultImages(engine);
    }

}

void VkEngine::initVulkan() {

    VulkanSetup::init(this);
    loadGltfFile();
}


void VkEngine::loadGltfFile() {
    std::shared_ptr<gltfData> gltf;
    std::shared_ptr<gltfData> scene = gltfData::Load(this, "assets/Chess.glb");
    //pbrSystem.buildPipelines(this);
    scene->drawNodes(ctx);
}