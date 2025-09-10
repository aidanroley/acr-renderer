#include "pch.h"
#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

    Utils::File::compileShader(SHADER_FILE_PATHS_TO_COMPILE);

    initApp(engine, renderer, window); // set up window, set up engine (vulkan things), setupCameraUBO of camera manager
    mainLoop(renderer, engine, window);
}

// This returns a copy of the struct but it's fine because it only contains references
void MainApp::initApp(VkEngine & engine, Renderer & renderer, Window & window) {

    try {

        engine.initEngine();
    }
    catch (const std::exception& e) {

        std::cerr << e.what() << std::endl;
    }
    renderer.setupFrameResources();
}

void MainApp::mainLoop(Renderer& renderer, VkEngine& engine, Window& window) {

    while (!glfwWindowShouldClose(engine.window)) {
        
        input.beginFrame();
        glfwPollEvents();

        routeActions(timer.frameDeltaTime()); // dt for cam

        renderer.updateFrameResources(); // update per frame gpu data
        editor.Update();
        engine.drawFrame(renderer);

    }
    vkDeviceWaitIdle(engine.device); // Wait for logical device to finish before exiting the loop
}

void MainApp::routeActions(float dt) {

    // do like switch(mode) case InputMode::FreeCam later
    CameraActions ca = actionMap.buildFreeCam(); // "free cam mode" inputs (gui not toggled)
    renderer.cameraManager.updateCameraData(ca, dt);
}