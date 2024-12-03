#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

class Camera;

// Func decs
void mainLoop(VulkanSetup& setup, Camera& camera);
void drawFrame(VulkanSetup& setup, Camera& camera);
void updateUniformBuffer(uint32_t currentImage, SwapChainInfo& swapChainInfo, UniformData& uniformData, Camera& camera);
void recreateSwapChain(VulkanSetup& setup);