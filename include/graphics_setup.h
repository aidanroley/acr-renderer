#pragma once

#include "../include/camera.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

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
/*
struct CameraHelper {

    Camera camera;

    CameraHelper(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
        : camera(position, target, up) {}
};
*/

struct VertexData {

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

struct ModelFlags {

    bool SunTempleFlag = true;
};

struct GraphicsSetup {

    UniformBufferObject* ubo;
    Camera* camera;
    VertexData* vertexData;
    ModelFlags* modelFlags;

    GraphicsSetup(UniformBufferObject* uboT, Camera* ca, VertexData* vd, ModelFlags* mf)
        : ubo(uboT), camera(ca), vertexData(vd), modelFlags(mf) {}
};

void initGraphics(GraphicsSetup& graphics, VkEngine& engine);
void initUBO_Camera(GraphicsSetup& graphics, VkEngine& engine, uint32_t currentImage);
void populateVertexBuffer(GraphicsSetup& graphics);
void loadModel(GraphicsSetup& graphics);
void updateUniformBuffers(GraphicsSetup& graphics, VkEngine& engine, uint32_t currentImage);
