#include "../include/init.h"
#include "../include/app.h"

// I'm initializing them in this file since I use references (not ptrs) for these structs many times and they have to have root file scope
VulkanContext context = {};
SwapChainInfo swapChainInfo = { &context.swapChain };
PipelineInfo pipelineInfo = {};
CommandInfo commandInfo = {};
SyncObjects syncObjects = {};

int main() {

	VulkanSetup setup = initApp(context, swapChainInfo, pipelineInfo, commandInfo, syncObjects);
	mainLoop(*setup.context, *setup.swapChainInfo, *setup.pipelineInfo, *setup.commandInfo, *setup.syncObjects);
	cleanupVkObjects(*setup.context, *setup.swapChainInfo, *setup.pipelineInfo, *setup.commandInfo, *setup.syncObjects);
}

void mainLoop(VulkanContext& context, SwapChainInfo& swapChainInfo, PipelineInfo& pipelineInfo, CommandInfo& commandInfo, SyncObjects& syncObjects) {

    while (!glfwWindowShouldClose(context.window)) {

        glfwPollEvents();
        drawFrame(context, swapChainInfo, commandInfo, syncObjects, pipelineInfo);
    }

    vkDeviceWaitIdle(context.device); // Wait for logical device to finish before exiting the loop
}

// Wait for previous frame to finish -> Acquire an image from the swap chain -> Record a command buffer which draws the scene onto that image -> Submit the reocrded command buffer -> Present the swap chain image
// Semaphores are for GPU synchronization, Fences are for CPU
void drawFrame(VulkanContext& context, SwapChainInfo& swapChainInfo, CommandInfo& commandInfo, SyncObjects& syncObjects, PipelineInfo& pipelineInfo) {

    // Make CPU wait until the GPU is done.
    vkWaitForFences(context.device, 1, &syncObjects.inFlightFences[syncObjects.currentFrame], VK_TRUE, UINT64_MAX);
    // Reset fence to unsignaled state
    vkResetFences(context.device, 1, &syncObjects.inFlightFences[syncObjects.currentFrame]);

    uint32_t imageIndex;
    // This tells the imageAvailableSemaphore to be signaled when done.
    vkAcquireNextImageKHR(context.device, *swapChainInfo.swapChain, UINT64_MAX, syncObjects.imageAvailableSemaphores[syncObjects.currentFrame], VK_NULL_HANDLE, &imageIndex);

    // Record command buffer then submit info to it
    vkResetCommandBuffer(commandInfo.commandBuffers[syncObjects.currentFrame], 0);
    recordCommandBuffer(commandInfo.commandBuffers[syncObjects.currentFrame], imageIndex, pipelineInfo, commandInfo, swapChainInfo);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { syncObjects.imageAvailableSemaphores[syncObjects.currentFrame] }; // Wait on this before execution begins
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // This tells in which stage of pipeline to wait
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    // These two tell which command buffers to submit for execution
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandInfo.commandBuffers[syncObjects.currentFrame];

    // So it knows which semaphores to signal once command buffers are done
    VkSemaphore signalSemaphores[] = { syncObjects.renderFinishedSemaphores[syncObjects.currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Last parameter is what signals the fence when command buffers finish
    if (vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, syncObjects.inFlightFences[syncObjects.currentFrame]) != VK_SUCCESS) {

        throw std::runtime_error("failed to submit draw command buffer");
    }

    // Submit result back to the swap chain and show it on the screen finally
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // Wait on this semaphore before presentation can happen (renderFinishedSemaphore)

    VkSwapchainKHR swapChains[] = { *swapChainInfo.swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    vkQueuePresentKHR(context.presentQueue, &presentInfo);
    syncObjects.currentFrame = (syncObjects.currentFrame + 1) & (MAX_FRAMES_IN_FLIGHT);
}