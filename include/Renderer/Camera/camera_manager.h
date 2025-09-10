#pragma once
#include "Renderer/Camera/camera.h"
#include "Engine/Descriptor/vk_descriptor.h"
#include "Core/Input/action_map.h"


struct FrameUBO {

    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 viewPos;
};


class CameraManager {

public:

    VkEngine* _engine;
    DescriptorManager* _descriptorManager;

    FrameUBO ubo;
    Camera camera;

    void init(VkEngine* eng);
    void setupCameraUBO();
    void updateCameraUBO();
    void perFrameUpdate();
    void updateCameraData(CameraActions ca, float dt);

private:

    int MAX_FRAMES_IN_FLIGHT = 2;
};


