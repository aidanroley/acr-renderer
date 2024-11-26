#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Euler based camera system (pitch and yaw, no roll)
class Camera {

	const float CAMERA_YAW = -90.0f;
	const float CAMERA_PITCH = 0.0f;

public:

	glm::vec3 cameraPos;
	glm::vec3 cameraTarget;
	glm::vec3 cameraDirection;

	float yaw;
	float pitch;
	
	Camera(glm::vec3 startPosition, glm::vec3 startTarget) : cameraPos(startPosition), cameraTarget(startTarget), cameraDirection(glm::vec3(0.0f, 0.0f, -1.0f)), 
		   yaw(CAMERA_YAW), pitch(CAMERA_PITCH) {

		updateDirection();
	}
	
	glm::mat4 getViewMatrix() const {

		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		return glm::lookAt(cameraPos, cameraTarget, up);
	}

	void processMouseInput() {

	}

private:

	void updateDirection() {

		cameraDirection.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		cameraDirection.y = sin(glm::radians(pitch));
		cameraDirection.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	}

};

#endif