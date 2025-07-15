#include "pch.h"
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#include "Engine/vk_setup.h"
#include "Engine/engine.h"




// Wait for previous frame to finish -> Acquire an image from the swap chain -> Record a command buffer which draws the scene onto that image -> Submit the reocrded command buffer -> Present the swap chain image
// Semaphores are for GPU synchronization, Fences are for CPU
void VkEngine::drawFrame(Renderer& renderer) {

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
    
    //updateUniformBuffers(renderer, *this, currentFrame);
    renderer.updateUniformBuffers(currentFrame);
   
    // Reset fence to unsignaled state after we know the swapChain doesn't need to be recreated
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    // Record command buffer then submit info to it
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

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

GPUMeshBuffers VkEngine::uploadMesh(std::vector<uint32_t> indices, std::vector<Vertex> vertices) {

    size_t vbSize = vertices.size() * sizeof(Vertex);
    size_t idxSize = indices.size() * sizeof(uint32_t);

    GPUMeshBuffers newSurface{};

    newSurface.vertexBuffer = createBufferVMA(vbSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY, _allocator);

    VkBufferDeviceAddressInfo deviceAddressCreateInfo{};
    deviceAddressCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    deviceAddressCreateInfo.buffer = newSurface.vertexBuffer.buffer;

    assert(device != VK_NULL_HANDLE);
    assert(deviceAddressCreateInfo.buffer != VK_NULL_HANDLE);
    //std::cout << "Buffer handle: " << deviceAddressCreateInfo.buffer << std::endl;
    newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(device, &deviceAddressCreateInfo);
    std::cout << "UPLOADMESH vertexBufferAddress = 0x"
        << std::hex << static_cast<uint64_t>(newSurface.vertexBufferAddress)
        << std::dec << '\n';

    newSurface.indexBuffer = createBufferVMA(idxSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, _allocator);

    // create staging buffer and populate it with vertex/index data 
    AllocatedBuffer staging = createBufferVMA(vbSize + idxSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, _allocator);

    void* ptr = staging.allocation->GetMappedData();
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

    if (vkWaitForFences(device, 1, &immFence, true, 99999999) != VK_SUCCESS) {

        throw std::runtime_error("error waiting for fences imm");
    }
    
    // destroy staging add functionality.

    return newSurface;
}

// gets info from the MeshAsset within MeshNode
RenderObject MeshNode::createRenderObject(const GeoSurface& surface) {

    // next make it so 
    RenderObject obj;
    obj.materialSet.resize(MAX_FRAMES_IN_FLIGHT);
    obj.idxStart = surface.startIndex;
    //obj.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;
    obj.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
    obj.numIndices = surface.count;
    obj.vertexBuffer = mesh->meshBuffers.vertexBuffer.buffer;
    obj.transform = mesh->transform;
    obj.material = surface.material;
    obj.materialSet = surface.material->data.materialSet;
    return obj;
}

void MeshNode::Draw(DrawContext& ctx) {

    for (auto& surface : mesh->surfaces) {

        ctx.surfaces.push_back(createRenderObject(surface));
    }
    Node::Draw(ctx); // draws the children too....

}