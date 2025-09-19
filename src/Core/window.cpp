#include "pch.h"
#include "Core/window.h"
#include "Renderer/Camera/camera.h"
#include "Renderer/renderer_setup.h"
#include "vkEng/engine_setup.h"
#include "Core/Input/input.h"

// Window managment/GUI things are in this file

// --- Global (internal linkage...) things ---
namespace {

	inline UserPointerObjects* getUserPtr(GLFWwindow* w) {

		return static_cast<UserPointerObjects*>(glfwGetWindowUserPointer(w));
	}
}

Window::Window(int width, int height, const char* title) {

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
}

void Window::init(VkEngine& engine, Renderer& renderer) {

	// fix this below...(make get functions)
	UserPointerObjects* userPtr = new UserPointerObjects{ &engine.framebufferResized, &renderer.cameraManager.camera };

	engine.setWindow(_window);

	glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(_window, userPtr);

	glfwSetFramebufferSizeCallback(_window, frameBufferResizeCallback); // For resizing window
	glfwSetCursorPosCallback	  (_window, cursorPositionCallback);
	glfwSetScrollCallback		  (_window, scrollCallback);
	glfwSetKeyCallback			  (_window, keyCallback);
}

// -- Callbacks --
void Window::cursorPositionCallback(GLFWwindow* window, double x, double y) {

	auto& in = InputDevice::Get();
	in.setMousePos(static_cast<float>(x), static_cast<float>(y));
}

void Window::scrollCallback(GLFWwindow* window, double /*xOffset*/, double yOffset) {

	auto& in = InputDevice::Get();
	in.setScrollDelta(static_cast<float>(yOffset));
}

void Window::frameBufferResizeCallback(GLFWwindow* window, int /*width*/, int /*height*/) {

	if (auto* up = getUserPtr(window); up && up->camera) {

		*up->framebufferResized = true;
	}
}

void Window::keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

	auto& in = InputDevice::Get();
	if (action == GLFW_PRESS || action == GLFW_REPEAT || action == GLFW_RELEASE) {

		bool pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
		if (action == GLFW_RELEASE) pressed = false;
		in.setKey(key, pressed);
	}
}

Window::~Window() {

	if (_window) {

		glfwDestroyWindow(_window);
		glfwTerminate();
	}
}
