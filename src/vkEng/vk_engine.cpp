#include "pch.h"
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#include "vkEng/vk_engine_setup.h"
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "vkEng/vk_engine.h"

// Wait for previous frame to finish -> Acquire an image from the swap chain -> Record a command buffer which draws the scene onto that image -> Submit the reocrded command buffer -> Present the swap chain image
// Semaphores are for GPU synchronization, Fences are for CPU
void VkEngine::drawFrame() {

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
    
    // Reset fence to unsignaled state after we know the swapChain doesn't need to be recreated
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    // Record command buffer then submit info to it
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    // Record draw commands
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    submitFrame(commandBuffers[currentFrame]);

    presentFrame(imageIndex);

    currentFrame = (currentFrame + 1) % (MAX_FRAMES_IN_FLIGHT);
}

void VkEngine::submitFrame(VkCommandBuffer cmd) {

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };

    VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    Logger::vkCheck(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]), "failed to submit queue in submitFrame");
}

void VkEngine::presentFrame(uint32_t imageIndex) {

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    VkSwapchainKHR swapChains[] = { swapChain };

    VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image");
    }
}

// record command buffer for draw
void VkEngine::recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex) {

    VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    Logger::vkCheck((vkBeginCommandBuffer(cmd, &beginInfo)), "failed to begin command buffer");

    // start scene render pass
    VkRenderPassBeginInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0,0 };
    renderPassInfo.renderArea.extent = swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 }; // Max depth value
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    recordScene(cmd);

    vkCmdEndRenderPass(cmd);

    drawGUI(cmd, swapChainImageViews[imageIndex]);

    Logger::vkCheck(vkEndCommandBuffer(cmd), "failed to record command buffer");
}

void VkEngine::bindDraw(RenderObject& obj, VkCommandBuffer cmd) {

    VkDescriptorSet sets[] = {

            descriptorManager._descriptorSets[currentFrame],
            obj.materialSet[currentFrame]
    };

    vkCmdBindDescriptorSets(cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelines.layout,
        0, 2, sets,
        0, nullptr);

    // push constants
    vkCmdPushConstants(
        cmd, pipelines.layout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0, sizeof(glm::mat4),
        &obj.transform);

    // vertex/index buffers
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &obj.vertexBuffer, &offset);
    vkCmdBindIndexBuffer(cmd, obj.indexBuffer,
        obj.idxStart * sizeof(uint32_t),
        VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, obj.numIndices, 1, 0, 0, 0);
}

void VkEngine::recordScene(VkCommandBuffer cmd) {

    // add a check to see if its the same pipeline as last one later.
    // also look into sorting opaque materials for performacne later
    //vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.transparent);

    VkViewport viewport{ 0, 0,
        (float)swapChainExtent.width, (float)swapChainExtent.height, 0.0f, 1.0f };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{ {0,0}, swapChainExtent };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    for (auto& obj : ctx.surfaces) {

        //vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, obj.material->data.matPipeline.pipeline);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.opaque);
        bindDraw(obj, cmd);
    }
}

void VkEngine::drawGUI(VkCommandBuffer cb, VkImageView imageView) {

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.pNext = nullptr;

    colorAttachment.imageView = imageView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.pNext = nullptr;

    renderInfo.renderArea = VkRect2D{ VkOffset2D { 0, 0 }, swapChainExtent };
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachment;
    renderInfo.pDepthAttachment = nullptr;
    renderInfo.pStencilAttachment = nullptr;

    vkCmdBeginRendering(cb, &renderInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb);
    vkCmdEndRendering(cb);
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
    std::cerr << surface.material->data.type << std::endl;
    return obj;
}

void MeshNode::Draw(DrawContext& ctx) {

    for (auto& surface : mesh->surfaces) {

        ctx.surfaces.push_back(createRenderObject(surface));
    }
    Node::Draw(ctx); // draws the children too....

}

void VkEngine::recreateSwapChain() {

    // Handle minimization
    int width, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {

        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    editorContext.setWindowRes(width, height);

    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    // fix this...
    VulkanSetup::createSwapChain(this);
    VulkanSetup::createImageViews(this);
    VulkanSetup::createColorResources(this);
    VulkanSetup::createDepthResources(this);
    VulkanSetup::createFramebuffers(this);
}

// Helper for cleanup
void VkEngine::cleanupSwapChain() {

    vkDestroyImageView(device, colorImageView, nullptr);
    vkDestroyImage(device, colorImage, nullptr);
    vkFreeMemory(device, colorImageMemory, nullptr);

    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {

        vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
    }

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {

        vkDestroyImageView(device, swapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void VkEngine::cleanupVkObjects() {

    cleanupSwapChain();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    //vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);


    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyPipeline(device, pipelines.opaque, nullptr);
    vkDestroyPipelineLayout(device, pipelines.layout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {

    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}
