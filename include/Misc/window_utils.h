#pragma once

#include "Renderer/Camera/camera.h"
#include "Renderer/renderer_setup.h"

struct VulkanContext;
struct SwapChainInfo;
class Camera;
struct CameraUBO;
struct CameraHelper;

struct UserPointerObjects {

	bool* framebufferResized;
	CameraUBO* ubo;
	Camera* camera;
};

void updateWindowTitle(GLFWwindow* window);
void updateFPS(GLFWwindow* window);
void initWindow(VkEngine& engine, Renderer& renderer);
void setCursorPositionCallback(GLFWwindow* window, double xPosition, double yPosition);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void frameBufferResizeCallback(GLFWwindow* window, int width, int height);
void setScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

