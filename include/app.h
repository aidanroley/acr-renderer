#pragma once

#include "../include/graphics_setup.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

class Camera;

// Func decs
//void mainLoop(VulkanSetup& setup, Camera& camera, UniformBufferObject& ubo);
void mainLoop(VulkanSetup& setup, GraphicsSetup& graphics);
void drawFrame(VulkanSetup& setup, Camera& camera, UniformBufferObject& ubo, GraphicsSetup& graphics);
void updateUniformBuffer(uint32_t currentImage, SwapChainInfo& swapChainInfo, UniformData& uniformData, Camera& camera);
void recreateSwapChain(VulkanSetup& setup);