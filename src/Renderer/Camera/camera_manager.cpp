#include "pch.h"
#include "Engine/vk_setup.h"
#include "Renderer/Camera/camera.h"
#include "Renderer/Camera/camera_manager.h"

void CameraManager::init(VkEngine* eng) {

    _engine = eng;
}

void CameraManager::initUBO(uint32_t currentImage) {

    glm::vec3 cameraDirection = camera.getCameraDirection();
    glm::vec3 cameraPosition = camera.getCameraPosition();
    float fov = camera.getCameraFov();

    ubo.model = glm::mat4(1.0f); // Identity matrix, no rotation

    // view (camera position, target position, up)
    ubo.view = glm::lookAt(cameraPosition, cameraPosition + cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f));

    // (fovy, aspect, near, far)
    ubo.proj = glm::perspective(glm::radians(fov), _engine->swapChainExtent.width / (float)_engine->swapChainExtent.height, 0.1f, 1000.0f);
    ubo.proj[1][1] *= -1; // Y flipped in vulkan

    memcpy(_engine->uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

// look into push constants at some point
void CameraManager::updateUniformBuffers(uint32_t currentImage) {

    bool viewNeedsUpdate = camera.directionChanged || camera.posChanged;
    bool projNeedsUpdate = camera.zoomChanged;

    if (viewNeedsUpdate) {

        glm::vec3 cameraDirection = camera.getCameraDirection();
        glm::vec3 cameraPosition = camera.getCameraPosition();
        ubo.view = glm::lookAt(cameraPosition, cameraPosition + cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f));

        camera.directionChanged = false;
        camera.posChanged = false;
    }

    if (projNeedsUpdate) {

        float fov = camera.getCameraFov();
        ubo.proj = glm::perspective(glm::radians(fov), _engine->swapChainExtent.width / (float)_engine->swapChainExtent.height, 0.1f, 1000.0f);
        ubo.proj[1][1] *= -1;

        camera.zoomChanged = false;
    }
    if (viewNeedsUpdate || projNeedsUpdate) {

        memcpy(_engine->uniformBuffersMapped[currentImage], &ubo, sizeof(CameraUBO));
    }
}