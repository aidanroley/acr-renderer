#pragma once
#include "../precompile/pch.h"

#include "../include/window_utils.h"
#include "../include/camera.h"
#include "../include/graphics_setup.h"
#include "../include/vk_setup.h"

// Window managment/GUI things are in this file

auto lastTime = std::chrono::high_resolution_clock::now();
int frameCount = 0;
float fps = 0.0f;

void initWindow(VkEngine& engine, GraphicsSetup& graphics) {

	UserPointerObjects* userPointerObjects = new UserPointerObjects{ &engine.framebufferResized, graphics.ubo, &engine.camera };

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	engine.window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetInputMode(engine.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(engine.window, userPointerObjects);
	glfwSetFramebufferSizeCallback(engine.window, frameBufferResizeCallback); // For resizing window
	glfwSetCursorPosCallback(engine.window, setCursorPositionCallback);
	glfwSetScrollCallback(engine.window, setScrollCallback);
	glfwSetKeyCallback(engine.window, keyCallback);
}

void setCursorPositionCallback(GLFWwindow* window, double xPosition, double yPosition) {

	UserPointerObjects* userPointerObjects = static_cast<UserPointerObjects*>(glfwGetWindowUserPointer(window));
	if (userPointerObjects && userPointerObjects->camera) {

		userPointerObjects->camera->processMouseInput(static_cast<float>(xPosition), static_cast<float>(yPosition));
	}
}

void setScrollCallback(GLFWwindow* window, double xOffset, double yOffset) {

	UserPointerObjects* userPointerObjects = static_cast<UserPointerObjects*>(glfwGetWindowUserPointer(window));
	if (userPointerObjects && userPointerObjects->camera) {

		userPointerObjects->camera->processMouseScroll(static_cast<float>(yOffset));
	}
}

void frameBufferResizeCallback(GLFWwindow* window, int width, int height) {

	auto userPointerObjects = reinterpret_cast<UserPointerObjects*>(glfwGetWindowUserPointer(window));
	*userPointerObjects->framebufferResized = true;
	
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {

		glfwSetWindowShouldClose(window, true);
	}

	// Arrow keys for camera position
	if ((key == GLFW_KEY_W || key == GLFW_KEY_S || key == GLFW_KEY_A || key == GLFW_KEY_D) 
		&& (action == GLFW_PRESS || action == GLFW_REPEAT)) {

		auto userPointerObjects = reinterpret_cast<UserPointerObjects*>(glfwGetWindowUserPointer(window));
		if (userPointerObjects && userPointerObjects->camera) {

			userPointerObjects->camera->processArrowMovement(key);
		}
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
