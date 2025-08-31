#include "pch.h"
#include "Renderer/renderer_setup.h"
#include "Engine/gltf_loader.h"
#include "Engine/vk_setup.h"

void Renderer::init(VkEngine* eng, DescriptorManager* dm) {

    engine = eng;
    descriptorManager = dm;

    cameraManager.init(eng);
}

void Renderer::setUpUniformBuffers(uint32_t currentImage) {

    cameraManager.initUBO(currentImage);
}

// look into push constants at some point
void Renderer::updateUniformBuffers(uint32_t currentImage) {

    cameraManager.updateUniformBuffers(currentImage);
}