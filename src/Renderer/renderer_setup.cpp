#include "pch.h"
#include "Renderer/Camera/camera_manager.h"
#include "Renderer/renderer_setup.h"
#include "Engine/gltf_loader.h"
#include "Engine/engine_setup.h"

void Renderer::init(VkEngine* eng, DescriptorManager* dm) {

    engine = eng;
    descriptorManager = dm;

    cameraManager.init(eng);
}

void Renderer::setupFrameResources() {

    cameraManager.setupCameraUBO();
}

// look into push constants at some point
void Renderer::updateFrameResources() {

    cameraManager.perFrameUpdate();
}