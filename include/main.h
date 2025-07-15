#pragma once
#include "Renderer/renderer_setup.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

class Camera;

// Func decs
void initApp(VkEngine& engine, Renderer& renderer);
void mainLoop(Renderer& renderer, VkEngine& engine);

class MainApp {

    DescriptorManager descriptorManager;
    Renderer renderer;
    VkEngine engine;

public:
    void init();
};
