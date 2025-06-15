#pragma once
#include "Graphics/Camera/camera.h"

// Forward decs for init.h
struct Vertex;
struct VulkanSetup;
struct SwapChainInfo;
struct UniformData;

class VkEngine;
struct UniformBufferObject {

    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;

    alignas(16) glm::vec3 lightPos;
    alignas(16) glm::vec3 lightColor;
    alignas(16) glm::vec3 viewPos;
};

struct GraphicsSetup {

    UniformBufferObject* ubo;
    Camera* camera;


    GraphicsSetup(UniformBufferObject* uboT, Camera* ca)
        : ubo(uboT), camera(ca) {}
};

void initGraphics(GraphicsSetup& graphics, VkEngine& engine);
void initUBO_Camera(GraphicsSetup& graphics, VkEngine& engine, uint32_t currentImage);
void updateUniformBuffers(GraphicsSetup& graphics, VkEngine& engine, uint32_t currentImage);
