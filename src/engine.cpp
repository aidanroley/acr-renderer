#include "../precompile/pch.h"
#include "../include/engine.h"
#include "../include/file_funcs.h"
#include "../include/window_utils.h"
#include "../include/graphics_setup.h"

std::vector<std::string> SHADER_FILE_PATHS_TO_COMPILE = { 

    "shaders/Sun-Temple-Vert.vert", "shaders/Sun-Temple-Frag.frag"
};

int main() {

    compileShader(SHADER_FILE_PATHS_TO_COMPILE);

    // Initialize structs of GraphicsSetup instance
    Camera camera;
    UniformBufferObject ubo = {};
    ModelFlags modelFlags = {};
    VertexData vertexData = {};

    // Set structs to GraphicsSetup and VulkanSetup
    GraphicsSetup graphics(&ubo, &camera, &vertexData, &modelFlags);
    VkEngine engine(camera);

    initApp(engine, graphics);

    // Render loop
    mainLoop(graphics, engine);
	engine.cleanupVkObjects();
}

// This returns a copy of the struct but it's fine because it only contains references
void initApp(VkEngine& engine, GraphicsSetup& graphics) {

    initWindow(engine, graphics);
    populateVertexBuffer(graphics);

    try {

        engine.initVulkan(*graphics.vertexData);
    }
    catch (const std::exception& e) {

        std::cerr << e.what() << std::endl;
    }

    // Vk must set up uniform buffer mapping before this is called
    initGraphics(graphics, engine);
}

void mainLoop(GraphicsSetup& graphics, VkEngine& engine) {

    while (!glfwWindowShouldClose(engine.window)) {

        glfwPollEvents();
        engine.drawFrame(graphics);

        updateFPS(engine.window);
        updateSceneSpecificInfo(graphics);
    }

    vkDeviceWaitIdle(engine.device); // Wait for logical device to finish before exiting the loop
}

// Wait for previous frame to finish -> Acquire an image from the swap chain -> Record a command buffer which draws the scene onto that image -> Submit the reocrded command buffer -> Present the swap chain image
// Semaphores are for GPU synchronization, Fences are for CPU
void VkEngine::drawFrame(GraphicsSetup& graphics) {

    // Make CPU wait until the GPU is done.
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;

    // This tells the imageAvailableSemaphore to be signaled when done.
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {

        recreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {

        throw std::runtime_error("failed to get swap chain image");
    }
    
    updateUniformBuffers(graphics, *this, currentFrame);
   
    // Reset fence to unsignaled state after we know the swapChain doesn't need to be recreated
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    // Record command buffer then submit info to it
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex, *graphics.vertexData);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] }; // Wait on this before execution begins
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // This tells in which stage of pipeline to wait
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    // These two tell which command buffers to submit for execution
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    // So it knows which semaphores to signal once command buffers are done
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Last parameter is what signals the fence when command buffers finish
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {

        throw std::runtime_error("failed to submit draw command buffer");
    }

    // Submit result back to the swap chain and show it on the screen finally
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // Wait on this semaphore before presentation can happen (renderFinishedSemaphore)

    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {

        framebufferResized = false;
        recreateSwapChain();
    }
    else if (result != VK_SUCCESS) {

        throw std::runtime_error("failed to present swap chain images");
    }

    currentFrame = (currentFrame + 1) & (MAX_FRAMES_IN_FLIGHT);
}

// Note: May want to add functionality to cleanup and create another render pass as well (ie: moving window to a different monitor)
/*

*/

void updateSceneSpecificInfo(GraphicsSetup& graphics) {


}
