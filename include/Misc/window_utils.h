#pragma once

#include "Graphics/Camera/camera.h"
#include "Graphics/graphics_setup.h"

struct VulkanContext;
struct SwapChainInfo;
class Camera;
struct UniformBufferObject;
struct CameraHelper;

struct UserPointerObjects {

	bool* framebufferResized;
	UniformBufferObject* ubo;
	Camera* camera;
};

void updateWindowTitle(GLFWwindow* window);
void updateFPS(GLFWwindow* window);
void initWindow(VkEngine& engine, GraphicsSetup& graphics);
void setCursorPositionCallback(GLFWwindow* window, double xPosition, double yPosition);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void frameBufferResizeCallback(GLFWwindow* window, int width, int height);
void setScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

