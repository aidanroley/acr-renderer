#pragma once

#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <GLFW/glfw3.h>

#include "../include/init.h"
#include "../include/camera.h"

void updateWindowTitle(GLFWwindow* window);
void updateFPS(GLFWwindow* window);
void initWindow(VulkanContext& context, SwapChainInfo& swapChainInfo);
void frameBufferResizeCallback(GLFWwindow* window, int width, int height);

