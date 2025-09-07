#pragma once
#include "Renderer/renderer_setup.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

class Camera;

// Func decs
void initApp(VkEngine& engine, Renderer& renderer, Window& window);
void mainLoop(Renderer& renderer, VkEngine& engine, Window& window);

class MainApp {

    DescriptorManager descriptorManager;
    Renderer renderer;
    VkEngine engine;
    Window window;

public:
    void init();
};
