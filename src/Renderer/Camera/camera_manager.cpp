#include "pch.h"

#include "vkEng/engine_setup.h"
#include "Renderer/Camera/camera.h"
#include "vkEng/Descriptor/vk_descriptor.h"
#include "Renderer/Camera/camera_manager.h"

void CameraManager::init(VkEngine* eng) {

    _engine = eng;
}

void CameraManager::perFrameUpdate() {

    updateCameraUBO();
}

void CameraManager::updateCameraData(CameraActions ca, float dt) {

    camera.processArrowMovement(ca.moveX, ca.moveY, dt);
    camera.processMouseLook(ca.lookX, ca.lookY);
    camera.processMouseScroll(ca.scroll);
}

void CameraManager::setupCameraUBO() {

    glm::vec3 camDir = camera.getCameraDirection();
    glm::vec3 camPos = camera.getCameraPosition();
    float fov = camera.getCameraFov();

    ubo.model = glm::mat4(1.0f); // Identity matrix, no rotation

    // view (camera position, target position, up)
    ubo.view = glm::lookAt(camPos, camPos + camDir, glm::vec3(0.0f, 1.0f, 0.0f));

    // (fovy, aspect, near, far)
    ubo.proj = glm::perspective(glm::radians(fov), _engine->swapChainExtent.width / (float)_engine->swapChainExtent.height, 0.1f, 1000.0f);
    ubo.proj[1][1] *= -1; // Y flipped in vulkan

    std::memcpy(_engine->uniformBuffersMapped[0], &ubo, sizeof(ubo)); // this will always be 0 the first time around
}

// look into push constants at some point
void CameraManager::updateCameraUBO() {

    bool viewNeedsUpdate = camera.directionChanged || camera.posChanged;
    bool projNeedsUpdate = camera.zoomChanged;

    if (viewNeedsUpdate) {

        glm::vec3 camDir = camera.getCameraDirection();
        glm::vec3 camPos = camera.getCameraPosition();
        ubo.view = glm::lookAt(camPos, camPos + camDir, glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.viewPos = camPos;

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

        memcpy(_engine->uniformBuffersMapped[_engine->currentFrame], &ubo, sizeof(FrameUBO));
    }
}