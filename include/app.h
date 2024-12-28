#pragma once

#include "../include/graphics_setup.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

class Camera;

// Func decs
void initApp(VulkanSetup& setup, GraphicsSetup& graphics);
void mainLoop(VulkanSetup& setup, GraphicsSetup& graphics);
void drawFrame(VulkanSetup& setup, Camera& camera, UniformBufferObject& ubo, GraphicsSetup& graphics);
void recreateSwapChain(VulkanSetup& setup);
void updateSceneSpecificInfo(GraphicsSetup& graphics);