#pragma once

#include "Renderer/Camera/camera.h"
#include "Renderer/renderer_setup.h"
#ifdef USE_OPENGL
#include "glEng/gl_engine.h"
#endif

struct VulkanContext;
struct SwapChainInfo;
class Camera;
struct FrameUBO;
struct CameraHelper;

class VkEngine;

struct UserPointerObjects {

	bool* framebufferResized;
	Camera* camera;
};


class Window {

public:

	Window(int width = 1440, int height = 810, const char* title = "awesome engine");
	~Window();

	void init(IRenderEngine* engine, Renderer& renderer);
	void update();
	GLFWwindow* getWindow();

private:

	GLFWwindow* _window;
	static void cursorPositionCallback(GLFWwindow* window, double xPosition, double yPosition);
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);
	static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);
	void setVkWindowConfig(UserPointerObjects* userPtr);
};


