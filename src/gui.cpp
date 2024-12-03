#include "../include/gui.h"
#include "../include/camera.h"

// Window managment/GUI things are in this file

auto lastTime = std::chrono::high_resolution_clock::now();
int frameCount = 0;
float fps = 0.0f;

void initWindow(VulkanContext& context, SwapChainInfo& swapChainInfo, Camera& camera) {

	UserPointerObjects* userPointerObjects = new UserPointerObjects{ &swapChainInfo, &camera };

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	context.window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetInputMode(context.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(context.window, userPointerObjects);
	glfwSetFramebufferSizeCallback(context.window, frameBufferResizeCallback); // For resizing window
	glfwSetCursorPosCallback(context.window, setCursorPositionCallback);
	glfwSetKeyCallback(context.window, keyCallback);
}

void setCursorPositionCallback(GLFWwindow* window, double xPosition, double yPosition) {

	UserPointerObjects* userPointerObjects = static_cast<UserPointerObjects*>(glfwGetWindowUserPointer(window));
	if (userPointerObjects && userPointerObjects->camera) {

		userPointerObjects->camera->processMouseInput(static_cast<float>(xPosition), static_cast<float>(yPosition));
	}
}

void frameBufferResizeCallback(GLFWwindow* window, int width, int height) {

	auto userPointerObjects = reinterpret_cast<UserPointerObjects*>(glfwGetWindowUserPointer(window));

	if (userPointerObjects && userPointerObjects->swapChainInfo) {

		userPointerObjects->swapChainInfo->framebufferResized = true;
	}
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {

		glfwSetWindowShouldClose(window, true);
	}
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
