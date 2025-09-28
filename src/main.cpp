#include "pch.h"
#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "main.h"

int main() {

    MainApp app;
    app.init();
}

void MainApp::init() {

#ifdef USE_VULKAN 
    engine = std::make_unique<VkEngine>(); 
#elif USE_OPENGL
    engine = std::make_unique<glEngine>();
#endif

    engine->init(&renderer);
    renderer.init(engine.get());
    window.init(engine.get(), renderer);

    initApp(); // set up window, set up engine (vulkan things), setupCameraUBO of camera manager
    mainLoop();
}

// This returns a copy of the struct but it's fine because it only contains references
void MainApp::initApp() {

    try {

        engine->setupEngine();
    }
    catch (const std::exception& e) {

        std::cerr << e.what() << std::endl;
    }
    renderer.setupFrameResources(window);
}

void MainApp::mainLoop() {

    while (!glfwWindowShouldClose(window.getWindow())) {
        
        input.beginFrame();
        window.update();

        routeActions(timer.frameDeltaTime()); // dt for cam

        renderer.updateFrameResources(); // update per frame gpu data
        // temporary solution until iu get it working with opengl
#ifdef USE_VULKAN
        editor.Update();
#endif

        engine->drawFrame();
    }
}

void MainApp::routeActions(float dt) {

    // do like switch(mode) case InputMode::FreeCam later
    CameraActions ca = actionMap.buildFreeCam(); // "free cam mode" inputs (gui not toggled)
    renderer.cameraManager.updateCameraData(ca, dt);
}