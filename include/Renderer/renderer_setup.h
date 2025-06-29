#pragma once
#include "Renderer/Camera/camera.h"
#include "Renderer/Camera/camera_manager.h"

class Renderer {

public:

    VkEngine* engine;
    DescriptorManager* descriptorManager;
    CameraManager cameraManager;
    void init(VkEngine* eng, DescriptorManager* dm);

    void updateUniformBuffers(uint32_t currentImage);
    void setUpUniformBuffers(uint32_t currentImage);
};