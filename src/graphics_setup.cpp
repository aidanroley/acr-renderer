#pragma once
#include "../precompile/pch.h"
#include "../include/graphics_setup.h"
#include "../include/scene_info/gltf_loader.h"
#include "../include/vk_setup.h"

void initGraphics(GraphicsSetup& graphics, VkEngine& engine) {

    initUBO_Camera(graphics, engine, engine.currentFrame);
}

void initUBO_Camera(GraphicsSetup& graphics, VkEngine& engine, uint32_t currentImage) {

    glm::vec3 cameraDirection = graphics.camera->getCameraDirection();
    glm::vec3 cameraPosition = graphics.camera->getCameraPosition();
    float fov = graphics.camera->getCameraFov();

    graphics.ubo->model = glm::mat4(1.0f); // Identity matrix, no rotation

    // view (camera position, target position, up)
    graphics.ubo->view = glm::lookAt(cameraPosition, cameraPosition + cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f));

    // (fovy, aspect, near, far)
    graphics.ubo->proj = glm::perspective(glm::radians(fov), engine.swapChainExtent.width / (float)engine.swapChainExtent.height, 0.1f, 1000.0f);
    graphics.ubo->proj[1][1] *= -1; // Y flipped in vulkan

    memcpy(engine.uniformBuffersMapped[currentImage], &graphics.ubo, sizeof(graphics.ubo));
}

void populateVertexBuffer(GraphicsSetup& graphics) {

    loadModel(graphics);
}


void loadModel(GraphicsSetup& graphics) {

    if (graphics.modelFlags->SunTempleFlag) {

        //loadModel_SunTemple(*graphics.vertexData);
    }
}

// look into push constants at some point
void updateUniformBuffers(GraphicsSetup& graphics, VkEngine& engine, uint32_t currentImage) {

    bool viewNeedsUpdate = graphics.camera->directionChanged || graphics.camera->posChanged;
    bool projNeedsUpdate = graphics.camera->zoomChanged;

    if (viewNeedsUpdate) {

        glm::vec3 cameraDirection = graphics.camera->getCameraDirection();
        glm::vec3 cameraPosition = graphics.camera->getCameraPosition();
        graphics.ubo->view = glm::lookAt(cameraPosition, cameraPosition + cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f));

        graphics.camera->directionChanged = false;
        graphics.camera->posChanged = false;
    }

    if (projNeedsUpdate) {

        float fov = graphics.camera->getCameraFov();
        graphics.ubo->proj = glm::perspective(glm::radians(fov), engine.swapChainExtent.width / (float)engine.swapChainExtent.height, 0.1f, 1000.0f);
        graphics.ubo->proj[1][1] *= -1;

        graphics.camera->zoomChanged = false;
    }
    if (viewNeedsUpdate || projNeedsUpdate) {
        
        memcpy(engine.uniformBuffersMapped[currentImage], graphics.ubo, sizeof(UniformBufferObject));
    }
}