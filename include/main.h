#pragma once
#include "vkEng/file_funcs.h"
#include "Core/window.h"
#include "Renderer/renderer_setup.h"
#include "vkEng/engine_setup.h"
#include "vkEng/vk_helper_funcs.h"
#include "vkEng/engine.h"
#include "Core/Input/action_map.h"
#include "Core/Input/input.h"
#include "Core/Utils/timer.h"
#include "Renderer/renderer_setup.h"
#include "Editor/editor.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

class Camera;

class MainApp {

    DescriptorManager descriptorManager;
    Renderer renderer;
    VkEngine engine;
    Editor editor;
    Window window;

public:

    void init();

private:

    void initApp(VkEngine& engine, Renderer& renderer, Window& window);
    void mainLoop(Renderer& renderer, VkEngine& engine, Window& window);
    void routeActions(float dt);

    ActionMap actionMap;
    InputDevice& input = InputDevice::Get();
    Utils::Timer::Timer& timer = Utils::Timer::get();
};
