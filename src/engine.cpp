#pragma once
#include "../precompile/pch.h"
#include "../include/engine.h"
#include "../include/file_funcs.h"
#include "../include/window_utils.h"
#include "../include/graphics_setup.h"
#include "../include/vk_setup.h"
#include "../include/vk_helper_funcs.h"


//#include <vma/vk_mem_alloc.h>


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

gltfMaterial VkEngine::writeMaterial(MaterialPass pass, const GLTFMetallicRoughness::MaterialResources& resources) {

    gltfMaterial materialData;
    MaterialPipeline matPipeline;
    if (pass == MaterialPass::Transparent) {

        matPipeline.pipeline = &graphicsPipeline;
        matPipeline.layout = &pipelineLayout;
        materialData.pipeline = &matPipeline; // make this trnapsnare/opaque later
    }

    //allocate descriptor set (abstract into descriptormanager later)
    //materialData.materialSet = descriptorManager.allocateSet(*materialData.pipeline->layout);
    return materialData;
}

GPUMeshBuffers VkEngine::uploadMesh(std::vector<uint32_t> indices, std::vector<Vertex> vertices) {

    size_t vbSize = vertices.size() * sizeof(Vertex);
    size_t idxSize = indices.size() * sizeof(uint32_t);

    GPUMeshBuffers newSurface;

    newSurface.vertexBuffer = createBufferVMA(vbSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY, _allocator);

    VkBufferDeviceAddressInfo deviceAddressInfo{};
    deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    deviceAddressInfo.buffer = newSurface.vertexBuffer.buffer;

    newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(device, &deviceAddressInfo);
    newSurface.indexBuffer = createBufferVMA(idxSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, _allocator);

    // create staging buffer and populate it with vertex/index data 
    AllocatedBuffer staging = createBufferVMA(vbSize + idxSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, _allocator);
    void* data = staging.allocation->GetMappedData();

    memcpy(data, vertices.data(), vbSize);
    memcpy((char*)data + vbSize, indices.data(), idxSize);

    // transfer buffers to GPU memory using command buffers
    if (vkResetFences(device, 1, &immFence) != VK_SUCCESS) {

        throw std::runtime_error("failed to reset fence imm");
    }
    if ((vkResetCommandBuffer(immCommandBuffer, 0)) != VK_SUCCESS) {

        throw std::runtime_error("failed to reset command buffer imm");
    }

    // command buffer create info
    VkCommandBuffer cmd = immCommandBuffer;
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.pInheritanceInfo = nullptr;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {

        throw std::runtime_error("failed to begin command buffer imm");
    }

    // ready to copy buffers to GPU (index buffer is right after vertex buffer)
    VkBufferCopy vertexCopy{ 0 };
    vertexCopy.dstOffset = 0;
    vertexCopy.srcOffset = 0;
    vertexCopy.size = vbSize;
    vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy); // copies raw bytes from staging buffer (after submit queues of course)

    VkBufferCopy indexCopy{ 0 };
    indexCopy.dstOffset = 0;
    indexCopy.srcOffset = vbSize;
    indexCopy.size = idxSize;
    vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {

        throw std::runtime_error("failed to end command buffer imm");
    }

    // submit command buffer to queue for GPU
    VkCommandBufferSubmitInfo cmdSubmitInfo = {};
    cmdSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmdSubmitInfo.pNext = nullptr;
    cmdSubmitInfo.commandBuffer = cmd;
    cmdSubmitInfo.deviceMask = 0;

    VkSubmitInfo2 cmdSubmitInfo2 = {};
    cmdSubmitInfo2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    cmdSubmitInfo2.pNext = nullptr;
    cmdSubmitInfo2.waitSemaphoreInfoCount = 0;
    cmdSubmitInfo2.pWaitSemaphoreInfos = nullptr;
    cmdSubmitInfo2.signalSemaphoreInfoCount = 0;
    cmdSubmitInfo2.pSignalSemaphoreInfos = nullptr;

    cmdSubmitInfo2.commandBufferInfoCount = 1;
    cmdSubmitInfo2.pCommandBufferInfos = &cmdSubmitInfo;

    if (vkQueueSubmit2(graphicsQueue, 1, &cmdSubmitInfo2, immFence) != VK_SUCCESS) {

        throw std::runtime_error("failed to queue submit2 imm");
    }

    if (vkWaitForFences(device, 1, &immFence, true, 999999) != VK_SUCCESS) {

        throw std::runtime_error("error waiting for fences imm");
    }
    
    // destroy staging add functionality.

    return newSurface;
}

void updateSceneSpecificInfo(GraphicsSetup& graphics) {


}
