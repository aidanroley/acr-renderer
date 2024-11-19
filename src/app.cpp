#include "../include/init.h"
#include "../include/app.h"
#include "../include/file_funcs.h"

std::vector<std::string> SHADER_FILE_PATHS_TO_COMPILE = { "shaders/vertex.vert", "shaders/fragment.frag" };

int main() {

    compileShader(SHADER_FILE_PATHS_TO_COMPILE);

    // I'm initializing these structs here since I use references (not ptrs) for these structs and they cannot be destroyed until the program is quit
    VulkanContext context = {};
    SwapChainInfo swapChainInfo = { &context.swapChain };
    PipelineInfo pipelineInfo = {};
    CommandInfo commandInfo = {};
    SyncObjects syncObjects = {};
    UniformData uniformData = {};
    TextureData textureData = {};
    DepthInfo depthInfo = {};

    VulkanSetup setup(&context, &swapChainInfo, &pipelineInfo, &commandInfo, &syncObjects, &uniformData, &textureData, &depthInfo);

	initApp(setup);
    mainLoop(setup);
	cleanupVkObjects(setup);
}

void mainLoop(VulkanSetup& setup) {

    while (!glfwWindowShouldClose(setup.context->window)) {

        glfwPollEvents();
        drawFrame(setup);
    }

    vkDeviceWaitIdle(setup.context->device); // Wait for logical device to finish before exiting the loop
}

// Wait for previous frame to finish -> Acquire an image from the swap chain -> Record a command buffer which draws the scene onto that image -> Submit the reocrded command buffer -> Present the swap chain image
// Semaphores are for GPU synchronization, Fences are for CPU
void drawFrame(VulkanSetup& setup) {

    // Make CPU wait until the GPU is done.
    vkWaitForFences(setup.context->device, 1, &setup.syncObjects->inFlightFences[setup.syncObjects->currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;

    // This tells the imageAvailableSemaphore to be signaled when done.
    VkResult result = vkAcquireNextImageKHR(setup.context->device, *setup.swapChainInfo->swapChain, UINT64_MAX, setup.syncObjects->imageAvailableSemaphores[setup.syncObjects->currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {

        recreateSwapChain(setup);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {

        throw std::runtime_error("failed to get swap chain image");
    }

    updateUniformBuffer(setup.syncObjects->currentFrame, *setup.swapChainInfo, *setup.uniformData);

    // Reset fence to unsignaled state after we know the swapChain doesn't need to be recreated
    vkResetFences(setup.context->device, 1, &setup.syncObjects->inFlightFences[setup.syncObjects->currentFrame]);

    // Record command buffer then submit info to it
    vkResetCommandBuffer(setup.commandInfo->commandBuffers[setup.syncObjects->currentFrame], 0);
    recordCommandBuffer(setup.commandInfo->commandBuffers[setup.syncObjects->currentFrame], imageIndex, *setup.pipelineInfo, *setup.commandInfo, *setup.swapChainInfo, *setup.uniformData, *setup.syncObjects);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { setup.syncObjects->imageAvailableSemaphores[setup.syncObjects->currentFrame] }; // Wait on this before execution begins
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // This tells in which stage of pipeline to wait
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    // These two tell which command buffers to submit for execution
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &setup.commandInfo->commandBuffers[setup.syncObjects->currentFrame];

    // So it knows which semaphores to signal once command buffers are done
    VkSemaphore signalSemaphores[] = { setup.syncObjects->renderFinishedSemaphores[setup.syncObjects->currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Last parameter is what signals the fence when command buffers finish
    if (vkQueueSubmit(setup.context->graphicsQueue, 1, &submitInfo, setup.syncObjects->inFlightFences[setup.syncObjects->currentFrame]) != VK_SUCCESS) {

        throw std::runtime_error("failed to submit draw command buffer");
    }

    // Submit result back to the swap chain and show it on the screen finally
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // Wait on this semaphore before presentation can happen (renderFinishedSemaphore)

    VkSwapchainKHR swapChains[] = { *setup.swapChainInfo->swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(setup.context->presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || setup.swapChainInfo->framebufferResized) {

        setup.swapChainInfo->framebufferResized = false;
        recreateSwapChain(setup);
    }
    else if (result != VK_SUCCESS) {

        throw std::runtime_error("failed to present swap chain images");
    }

    setup.syncObjects->currentFrame = (setup.syncObjects->currentFrame + 1) & (MAX_FRAMES_IN_FLIGHT);
}

// look into push constants later for more efficient
void updateUniformBuffer(uint32_t currentImage, SwapChainInfo& swapChainInfo, UniformData& uniformData) {

    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    // view (camera position, target position, up)
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    // (fovy, aspect, near, far)
    ubo.proj = glm::perspective(glm::radians(40.0f), swapChainInfo.swapChainExtent.width / (float)swapChainInfo.swapChainExtent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1; // Y flipped in vulkan

    memcpy(uniformData.uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

// Note: May want to add functionality to cleanup and create another render pass as well (ie: moving window to a different monitor)
void recreateSwapChain(VulkanSetup& setup) {

    // Handle minimization
    int width, height = 0;
    glfwGetFramebufferSize(setup.context->window, &width, &height);
    while (width == 0 || height == 0) {

        glfwGetFramebufferSize(setup.context->window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(setup.context->device);

    vkDeviceWaitIdle(setup.context->device);
    cleanupSwapChain(*setup.context, *setup.swapChainInfo, *setup.depthInfo);

    createSwapChain(*setup.context, *setup.swapChainInfo);
    createImageViews(*setup.context, *setup.swapChainInfo);
    createDepthResources(*setup.context, *setup.swapChainInfo, *setup.depthInfo);
    createFramebuffers(*setup.context, *setup.swapChainInfo, *setup.pipelineInfo, *setup.depthInfo);
}
