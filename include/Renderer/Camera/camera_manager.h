#pragma once
#include "Renderer/Camera/camera.h"
#include "Core/Input/action_map.h"
#include "Core/IRenderEngine.h"

#ifdef USE_VULKAN
#include "vkEng/Descriptor/vk_descriptor.h"
#endif

class DescriptorManager;

class Window;

struct FrameUBO {

    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 viewPos;
};


class CameraManager {

public:

    IRenderEngine* _engine;
    DescriptorManager* _vkDescriptorManager;

    FrameUBO ubo;
    Camera camera;

    void init(IRenderEngine* eng);
    void setupCameraUBO(Window& window);
    void updateCameraUBO();
    void perFrameUpdate();
    void updateCameraData(CameraActions ca, float dt);
    void passToEngine();

private:

    float getAspectRatioGL(Window& window);
    float getAspectRatioVk();
    int MAX_FRAMES_IN_FLIGHT = 2;

    float _aspect = 16.0f / 9.0f;
};


