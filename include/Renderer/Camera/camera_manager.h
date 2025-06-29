#pragma once
#include "Descriptor/vk_descriptor.h"


struct CameraUBO {

    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};


class CameraManager {

public:

    VkEngine* _engine;
    DescriptorManager* _descriptorManager;

    CameraUBO ubo;
    Camera camera;

    void init(VkEngine* eng);
    void initUBO(uint32_t currentImage);
    void updateUniformBuffers(uint32_t currentImage);

private:

    int MAX_FRAMES_IN_FLIGHT = 2;
};


