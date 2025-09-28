#pragma once
#include "Renderer/Camera/camera.h"
#include "Renderer/Camera/camera_manager.h"
#include "Core/IRenderEngine.h"

class Window;

class Renderer {

public:

    IRenderEngine* engine = nullptr;
    CameraManager cameraManager;
    void init(IRenderEngine* eng);

    void updateFrameResources();
    void setupFrameResources(Window& window);
    void handleInputs();
};