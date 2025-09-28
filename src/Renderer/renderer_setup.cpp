#include "pch.h"
#include "Renderer/Camera/camera_manager.h"
#include "Core/window.h"
#include "Renderer/renderer_setup.h"

#ifdef USE_VULKAN
#include "vkEng/gltf_loader.h"
#include "vkEng/vk_engine_setup.h"
#endif

void Renderer::init(IRenderEngine* eng) {

    engine = eng;
    cameraManager.init(eng);
}

void Renderer::setupFrameResources(Window& window) {

    cameraManager.setupCameraUBO(window);
}

// look into push constants at some point
void Renderer::updateFrameResources() {

    cameraManager.perFrameUpdate();
}