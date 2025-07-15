#include "pch.h"
#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// #include <vma/vk_mem_alloc.h>
#include "Misc/file_funcs.h"
#include "Misc/window_utils.h"
#include "Renderer/renderer_setup.h"
#include "Engine/vk_setup.h"
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

    compileShader(SHADER_FILE_PATHS_TO_COMPILE);

    initApp(engine, renderer); // set up window, set up engine (vulkan things), initUBO of camera manager
    mainLoop(renderer, engine);
}

// This returns a copy of the struct but it's fine because it only contains references
void initApp(VkEngine& engine, Renderer& renderer) {

    initWindow(engine, renderer);
    try {

        engine.initVulkan();
    }
    catch (const std::exception& e) {

        std::cerr << e.what() << std::endl;
    }
    renderer.setUpUniformBuffers(engine.currentFrame);
}

void mainLoop(Renderer& renderer, VkEngine& engine) {

    while (!glfwWindowShouldClose(engine.window)) {

        glfwPollEvents();
        engine.drawFrame(renderer);

        updateFPS(engine.window);
    }

    vkDeviceWaitIdle(engine.device); // Wait for logical device to finish before exiting the loop
}