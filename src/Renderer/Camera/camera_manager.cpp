// CameraManager.cpp
#include "pch.h"
#include "Renderer/Camera/camera.h"
#include "Core/window.h"
#include "Renderer/Camera/camera_manager.h"

#ifdef USE_VULKAN
#include "vkEng/vk_engine_setup.h"
#elif defined(USE_OPENGL)
#include "glEng/gl_engine.h"
#endif

void CameraManager::init(IRenderEngine* eng) {

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

static float aspectFromGL(Window& window) {

    int fbW = 1, fbH = 1;
    glfwGetFramebufferSize(window.getWindow(), &fbW, &fbH);
    return fbW > 0 ? (fbW / static_cast<float>(fbH)) : 16.0f / 9.0f;
}

static float aspectFromVK(IRenderEngine* eng) {

#ifdef USE_VULKAN
    if (auto* vk = dynamic_cast<VkEngine*>(eng)) {

        return vk->swapChainExtent.width / (float)vk->swapChainExtent.height;
    }
#endif
    return 16.0f / 9.0f;
}

void CameraManager::setupCameraUBO(Window& window) {

    // cache aspect once; update this on window-resize callback too
#ifdef USE_OPENGL
    _aspect = aspectFromGL(window);
#else
    _aspect = aspectFromVK(_engine);
#endif

    // unused here, but set to identity
    ubo.model = glm::mat4(1.0f);

    // view
    const glm::vec3 camPos = camera.getCameraPosition();
    const glm::vec3 camDir = camera.getCameraDirection();
    ubo.view = glm::lookAt(camPos, camPos + camDir, glm::vec3(0, 1, 0));
    ubo.viewPos = camPos;

    // proj
    const float fov = glm::radians(camera.getCameraFov());
    ubo.proj = glm::perspective(fov, _aspect, 0.1f, 1000.0f);

#ifdef USE_VULKAN
    // only Vulkan needs Y flip
    ubo.proj[1][1] *= -1.0f;
#endif

    // push once now
    passToEngine();
}

void CameraManager::updateCameraUBO() {

    const bool viewNeedsUpdate = camera.directionChanged || camera.posChanged;
    const bool projNeedsUpdate = camera.zoomChanged;

    if (viewNeedsUpdate) {

        const glm::vec3 camPos = camera.getCameraPosition();
        const glm::vec3 camDir = camera.getCameraDirection();
        ubo.view = glm::lookAt(camPos, camPos + camDir, glm::vec3(0, 1, 0));
        ubo.viewPos = camPos;
        camera.directionChanged = false;
        camera.posChanged = false;
    }

    if (projNeedsUpdate) {

        const float fov = glm::radians(camera.getCameraFov());
        ubo.proj = glm::perspective(fov, _aspect, 0.1f, 1000.0f);
#ifdef USE_VULKAN
        ubo.proj[1][1] *= -1.0f;
#endif
        camera.zoomChanged = false;
    }

    if (viewNeedsUpdate || projNeedsUpdate) {

        passToEngine();
    }
}

void CameraManager::passToEngine() {

#ifdef USE_VULKAN
    if (auto* vk = dynamic_cast<VkEngine*>(_engine)) {

        memcpy(vk->uniformBuffersMapped[vk->currentFrame], &ubo, sizeof(FrameUBO));
        return;
    }
#endif
#ifdef USE_OPENGL
    if (auto* gl = dynamic_cast<glEngine*>(_engine)) {

        gl->passCameraData(ubo.view, ubo.proj, ubo.viewPos);
        return;
    }
#endif
}
