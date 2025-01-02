#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Euler based camera system (pitch and yaw, no roll)
class Camera {

	const float CAMERA_YAW = 0.0f;         // Look straight down the positive Z-axis.
	const float CAMERA_PITCH = -15.0f;
	const float MOUSE_X_INIT = 400.0f; // center of screen
	const float MOUSE_Y_INIT = 300.0f;
	const float CAMERA_FOV = 45.0f;
	const float CAMERA_SPEED = 0.2f;

	bool firstMouse = true;

public:

	glm::vec3 cameraPos;
	glm::vec3 cameraTarget;
	glm::vec3 cameraDirection;
	glm::vec3 cameraFront;
	glm::vec3 cameraUp;

	float yaw;
	float pitch;
	float fov;
	
	double currentMouseX;
	double currentMouseY;

	bool zoomChanged = true;
	bool posChanged = true;
	bool directionChanged = true;
	
	Camera(glm::vec3 startPosition, glm::vec3 startTarget, glm::vec3 defaultUp) : cameraPos(startPosition), cameraTarget(startTarget), cameraUp(defaultUp),
		cameraDirection(glm::vec3(0.0f, 0.0f, 0.0f)), yaw(CAMERA_YAW), pitch(CAMERA_PITCH), fov(CAMERA_FOV), currentMouseX(MOUSE_X_INIT), currentMouseY(MOUSE_Y_INIT) {

		updateDirection();
	}
	
	glm::mat4 getViewMatrix() const {

		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		return glm::lookAt(cameraPos, cameraTarget, up);
	}

	glm::vec3 getCameraDirection() const {

		return cameraFront;
	}

	glm::vec3 getCameraPosition() const {

		return cameraPos;
	}

	float getCameraFov() const {

		return fov;
	}

	void processMouseInput(double xpos, double ypos) {

		if (firstMouse) {

			currentMouseX = xpos;
			currentMouseY = ypos;
			firstMouse = false;
		}

		float xOffset = static_cast<float>(xpos) - currentMouseX;
		float yOffset = currentMouseY - static_cast<float>(ypos);
		//float yOffset = static_cast<float>(ypos) - currentMouseY;

		const float sensitivity = 0.1f;
		xOffset *= sensitivity;
		yOffset *= sensitivity;

		pitch += yOffset;
		yaw += xOffset;

		if (pitch > 89.0f) pitch = 89.0f;
		if (pitch < -89.0f) pitch = -89.0f;

		updateDirection();
		directionChanged = true;

		currentMouseX = xpos;
		currentMouseY = ypos;
	}

	void processMouseScroll(float yOffset) {

		fov -= yOffset;
		if (fov < 1.0f) fov = 1.0f;
		if (fov > 45.0f) fov = 45.0f;

		zoomChanged = true;
	}

	void processArrowMovement(int key) {

		if (key == 265) cameraPos += CAMERA_SPEED * cameraFront;
		else if (key == 264) cameraPos -= CAMERA_SPEED * cameraFront;
		else if (key == 263) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * CAMERA_SPEED;
		else if (key == 262) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * CAMERA_SPEED;

		posChanged = true;

	}

private:

	void updateDirection() {

		cameraDirection.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		cameraDirection.y = sin(glm::radians(pitch));
		cameraDirection.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		cameraFront = glm::normalize(cameraDirection);
	}

};

#endif