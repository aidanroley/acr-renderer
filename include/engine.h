#pragma once

#include "../include/graphics_setup.h"


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

class Camera;

// Func decs
void initApp(VkEngine& engine, GraphicsSetup& graphics);
void mainLoop(GraphicsSetup& graphics, VkEngine& engine);
void updateSceneSpecificInfo(GraphicsSetup& graphics);