#include "pch.h"
#include "Core/window.h"
#include "Renderer/Camera/camera.h"
#include "Renderer/renderer_setup.h"
#include "Core/Input/input.h"
#ifdef USE_VULKAN
#include "vkEng/vk_engine_setup.h"
#endif

// Window managment/GUI things are in this file

// --- Global (internal linkage...) things ---
namespace {

	inline UserPointerObjects* getUserPtr(GLFWwindow* w) {

		return static_cast<UserPointerObjects*>(glfwGetWindowUserPointer(w));
	}
}

Window::Window(const char* title) {

	glfwInit();
	if (gGraphicsAPI == GraphicsAPI::Vulkan) {

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		_window = glfwCreateWindow(getResWidth(), getResHeight(), title, nullptr, nullptr);
	}
	else if (gGraphicsAPI == GraphicsAPI::OpenGL) {

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_DEPTH_BITS, 24);  
		glfwWindowHint(GLFW_STENCIL_BITS, 8);
		glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
		//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		_window = glfwCreateWindow(getResWidth(), getResHeight(), title, nullptr, nullptr);
		glfwMakeContextCurrent(_window);
	}

}

void Window::init(IRenderEngine* engine, Renderer& renderer) {

	engine->setWindow(_window);

#ifdef USE_VULKAN
	if (auto* vk = dynamic_cast<VkEngine*>(engine)) {

		UserPointerObjects* userPtr = new UserPointerObjects{ &vk->framebufferResized, &renderer.cameraManager.camera };
		setVkWindowConfig(userPtr);
	}
#elif USE_OPENGL
	if (auto* gl = dynamic_cast<glEngine*>(engine)) {

		UserPointerObjects* userPtr = new UserPointerObjects{ 0, &renderer.cameraManager.camera };
		setVkWindowConfig(userPtr);
	}
#endif
}

void Window::update() {

	if (gGraphicsAPI == GraphicsAPI::OpenGL) glfwSwapBuffers(_window);
	glfwPollEvents();
}

void Window::setVkWindowConfig(UserPointerObjects* userPtr) {

	glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(_window, userPtr);

	glfwSetFramebufferSizeCallback(_window, frameBufferResizeCallback); // For resizing window
	glfwSetCursorPosCallback(_window, cursorPositionCallback);
	glfwSetScrollCallback(_window, scrollCallback);
	glfwSetKeyCallback(_window, keyCallback);
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

void Window::frameBufferResizeCallback(GLFWwindow* window, int width, int height) {

#ifdef USE_VULKAN
	if (auto* up = getUserPtr(window); up && up->camera) {

		*up->framebufferResized = true;
	}
#elif USE_OPENGL
	glViewport(0, 0, width, height);
#endif
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

GLFWwindow* Window::getWindow() {

	return _window;
}

Window::~Window() {

	if (_window) {

		glfwDestroyWindow(_window);
		glfwTerminate();
	}
}
