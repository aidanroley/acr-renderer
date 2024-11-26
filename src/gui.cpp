#include "../include/gui.h"

// Window managment/GUI things are in this file

auto lastTime = std::chrono::high_resolution_clock::now();
int frameCount = 0;
float fps = 0.0f;

void initWindow(VulkanContext& context, SwapChainInfo& swapChainInfo) {

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	context.window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetInputMode(context.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(context.window, &swapChainInfo);
	glfwSetFramebufferSizeCallback(context.window, frameBufferResizeCallback); // For resizing window
	//glfwSetCursorPosCallback(context.window, Camera::processMouseInput());
}

void frameBufferResizeCallback(GLFWwindow* window, int width, int height) {

	auto appState = reinterpret_cast<SwapChainInfo*>(glfwGetWindowUserPointer(window));
	appState->framebufferResized = true;
}

void updateFPS(GLFWwindow* window) {

	auto currentTime = std::chrono::high_resolution_clock::now();
	float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
	frameCount++;

	if (deltaTime >= 1.0f) {

		fps = frameCount / deltaTime;
		frameCount = 0;
		lastTime = currentTime;
	}
	updateWindowTitle(window);
}

void updateWindowTitle(GLFWwindow* window) {

	std::ostringstream title;
	title << "FPS: " << std::fixed << std::setprecision(1) << fps;

	glfwSetWindowTitle(window, title.str().c_str());
}
