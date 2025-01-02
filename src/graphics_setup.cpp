#include "../precompile/pch.h"
#include "../include/graphics_setup.h"
#include "../include/init.h"
#include "../include/scene_info/Cornell_Box.h"
#include "../include/scene_info/Sun_Temple.h"

void initGraphics(GraphicsSetup& graphics, VulkanSetup& setup) {

    initUBO_Camera(graphics, *setup.swapChainInfo, *setup.uniformData, setup.syncObjects->currentFrame);

    // Set model specific lighting data
    if (graphics.modelFlags->CornellBoxFlag) {

        setLightData_CornellBox(*graphics.ubo, *graphics.cameraHelper);
    }
}

void initUBO_Camera(GraphicsSetup& graphics, SwapChainInfo& swapChainInfo, UniformData& uniformData, uint32_t currentImage) {

    glm::vec3 cameraDirection = graphics.cameraHelper->camera.getCameraDirection();
    glm::vec3 cameraPosition = graphics.cameraHelper->camera.getCameraPosition();
    float fov = graphics.cameraHelper->camera.getCameraFov();

    graphics.ubo->model = glm::mat4(1.0f); // Identity matrix, no rotation

    // view (camera position, target position, up)
    graphics.ubo->view = glm::lookAt(cameraPosition, cameraPosition + cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f));

    // (fovy, aspect, near, far)
    graphics.ubo->proj = glm::perspective(glm::radians(fov), swapChainInfo.swapChainExtent.width / (float)swapChainInfo.swapChainExtent.height, 0.1f, 1000.0f);
    graphics.ubo->proj[1][1] *= -1; // Y flipped in vulkan

    memcpy(uniformData.uniformBuffersMapped[currentImage], &graphics.ubo, sizeof(graphics.ubo));
}

void populateVertexBuffer(GraphicsSetup& graphics) {

    loadModel(graphics);
}


void loadModel(GraphicsSetup& graphics) {

    if (graphics.modelFlags->CornellBoxFlag) {

        loadModel_CornellBox(*graphics.vertexData);
    }
    if (graphics.modelFlags->SunTempleFlag) {

        loadModel_SunTemple(*graphics.vertexData);
    }
}

// look into push constants at some point
void updateUniformBuffers(GraphicsSetup& graphics, SwapChainInfo& swapChainInfo, UniformData& uniformData, uint32_t currentImage) {

    bool viewNeedsUpdate = graphics.cameraHelper->camera.directionChanged || graphics.cameraHelper->camera.posChanged;
    bool projNeedsUpdate = graphics.cameraHelper->camera.zoomChanged;

    if (viewNeedsUpdate) {

        glm::vec3 cameraDirection = graphics.cameraHelper->camera.getCameraDirection();
        glm::vec3 cameraPosition = graphics.cameraHelper->camera.getCameraPosition();
        graphics.ubo->view = glm::lookAt(cameraPosition, cameraPosition + cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f));

        graphics.cameraHelper->camera.directionChanged = false;
        graphics.cameraHelper->camera.posChanged = false;
    }

    if (projNeedsUpdate) {

        float fov = graphics.cameraHelper->camera.getCameraFov();
        graphics.ubo->proj = glm::perspective(glm::radians(fov), swapChainInfo.swapChainExtent.width / (float)swapChainInfo.swapChainExtent.height, 0.1f, 1000.0f);
        graphics.ubo->proj[1][1] *= -1;

        graphics.cameraHelper->camera.zoomChanged = false;
    }
    if (viewNeedsUpdate || projNeedsUpdate) {
        
        memcpy(uniformData.uniformBuffersMapped[currentImage], graphics.ubo, sizeof(UniformBufferObject));
    }
}