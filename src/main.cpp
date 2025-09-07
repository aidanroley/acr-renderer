#include "pch.h"
#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// #include <vma/vk_mem_alloc.h>
#include "Core/file_funcs.h"
#include "Core/window.h"
#include "Renderer/renderer_setup.h"
#include "Engine/engine_setup.h"
#include "Engine/vk_helper_funcs.h"
#include "Engine/engine.h"
#include "main.h"

std::vector<std::string> SHADER_FILE_PATHS_TO_COMPILE = {

    "shaders/Shader-Vert.vert", "shaders/Shader-Frag.frag"
};

int main() {

    MainApp app;
    app.init();
}

void MainApp::init() {

    engine.init(&renderer, &descriptorManager);
    descriptorManager.init(&engine);
    renderer.init(&engine, &descriptorManager);
    window.init(engine, renderer);

    compileShader(SHADER_FILE_PATHS_TO_COMPILE);

    initApp(engine, renderer, window); // set up window, set up engine (vulkan things), setupCameraUBO of camera manager
    mainLoop(renderer, engine, window);
}

// This returns a copy of the struct but it's fine because it only contains references
void initApp(VkEngine& engine, Renderer& renderer, Window& window) {

    try {

        engine.initEngine();
    }
    catch (const std::exception& e) {

        std::cerr << e.what() << std::endl;
    }
    renderer.setupFrameResources();
}

void mainLoop(Renderer& renderer, VkEngine& engine, Window& window) {

    while (!glfwWindowShouldClose(engine.window)) {

        glfwPollEvents();
        
        engine.drawFrame(renderer);

        window.update();
    }
    vkDeviceWaitIdle(engine.device); // Wait for logical device to finish before exiting the loop
}