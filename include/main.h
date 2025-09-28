#pragma once
#ifdef USE_VULKAN
#include "vkEng/file_funcs.h"
#include "vkEng/vk_engine_setup.h"
#include "vkEng/vk_helper_funcs.h"
#include "vkEng/vk_engine.h"
#elif USE_OPENGL
#include "glEng/gl_engine.h"
#endif

#include "Core/window.h"
#include "Renderer/renderer_setup.h"
#include "Core/Input/action_map.h"
#include "Core/Input/input.h"
#include "Core/Utils/timer.h"
#include "Editor/editor.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

class Camera;

class MainApp {

    Renderer renderer;
    std::unique_ptr<IRenderEngine> engine;
    Editor editor;
    Window window;

public:

    void init();

private:

    void initApp();
    void mainLoop();
    void routeActions(float dt);

    ActionMap actionMap;
    InputDevice& input = InputDevice::Get();
    Utils::Timer::Timer& timer = Utils::Timer::get();
};
