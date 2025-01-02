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

struct UniformBufferObject {

    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;

    alignas(16) glm::vec3 lightPos;
    alignas(16) glm::vec3 lightColor;
    alignas(16) glm::vec3 viewPos;
};

struct CameraHelper {

    Camera camera;

    CameraHelper(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
        : camera(position, target, up) {}
};

struct VertexData {

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

struct ModelFlags {

    bool CornellBoxFlag = false;
    bool SunTempleFlag = true;
};

struct GraphicsSetup {

    UniformBufferObject* ubo;
    CameraHelper* cameraHelper;
    VertexData* vertexData;
    ModelFlags* modelFlags;

    GraphicsSetup(UniformBufferObject* uboT, CameraHelper* ch, VertexData* vd, ModelFlags* mf)
        : ubo(uboT), cameraHelper(ch), vertexData(vd), modelFlags(mf) {}
};

void initGraphics(GraphicsSetup& graphics, VulkanSetup& setup);
void initUBO_Camera(GraphicsSetup& graphics, SwapChainInfo& swapChainInfo, UniformData& uniformData, uint32_t currentImage);
void populateVertexBuffer(GraphicsSetup& graphics);
void loadModel(GraphicsSetup& graphics);
void updateUniformBuffers(GraphicsSetup& graphics, SwapChainInfo& swapChainInfo, UniformData& uniformData, uint32_t currentImage);
void loadModel_CornellBox(VertexData& vertexData);
