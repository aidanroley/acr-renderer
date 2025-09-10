#pragma once

#include "Renderer/Camera/camera.h"
#include "Renderer/renderer_setup.h"

struct VulkanContext;
struct SwapChainInfo;
class Camera;
struct FrameUBO;
struct CameraHelper;

class Window {

public:

	Window(int width = 1440, int height = 810, const char* title = "Vulkan");
	~Window();

	void init(VkEngine& engine, Renderer& renderer);

private:

	GLFWwindow* _window;
	static void cursorPositionCallback(GLFWwindow* window, double xPosition, double yPosition);
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);
	static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);

};
struct UserPointerObjects {

	bool* framebufferResized;
	Camera* camera;
};


