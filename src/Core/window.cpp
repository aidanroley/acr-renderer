#include "pch.h"
#include "Core/window.h"
#include "Renderer/Camera/camera.h"
#include "Renderer/renderer_setup.h"
#include "Engine/engine_setup.h"

// Window managment/GUI things are in this file

// --- Global (internal linkage...) things ---
namespace {

	auto g_lastTime = std::chrono::high_resolution_clock::now();
	int g_frameCount = 0;
	float g_fps = 0.0f;

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

	if (auto* up = getUserPtr(window); up && up->camera) {

		up->camera->processMouseInput(static_cast<float>(x), static_cast<float>(y));
	}
}

void Window::scrollCallback(GLFWwindow* window, double /*xOffset*/, double yOffset) {

	if (auto* up = getUserPtr(window); up && up->camera) {

		up->camera->processMouseScroll(static_cast<float>(yOffset));
	}
}

void Window::frameBufferResizeCallback(GLFWwindow* window, int /*width*/, int /*height*/) {

	if (auto* up = getUserPtr(window); up && up->camera) {

		*up->framebufferResized = true;
	}
}

void Window::keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {

	if (auto* up = getUserPtr(window); up && up->camera) {

		bool isValidKey =
			(key == GLFW_KEY_W) || (key == GLFW_KEY_S) ||
			(key == GLFW_KEY_A) || (key == GLFW_KEY_D);

		if (!isValidKey) return;

		bool pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
		if (action == GLFW_RELEASE) pressed = false;

		up->camera->updateKBState(key, pressed);
	}
}

// -- fps title on the window --
void Window::update() {

	auto currentTime = std::chrono::high_resolution_clock::now();
	float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - g_lastTime).count();

	g_frameCount++;

	if (dt >= 1.0f) {

		g_fps = g_frameCount / dt;
		g_frameCount = 0;
		g_lastTime = currentTime;
	}
	updateWindowTitle();
}

void Window::updateWindowTitle() {

	std::ostringstream title;
	title << "FPS: " << std::fixed << std::setprecision(1) << g_fps;
	glfwSetWindowTitle(_window, title.str().c_str());
}

Window::~Window() {

	if (_window) {

		glfwDestroyWindow(_window);
		glfwTerminate();
	}
}
