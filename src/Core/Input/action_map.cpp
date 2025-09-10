#include "pch.h"
#include "Core/Input/action_map.h"

CameraActions ActionMap::buildFreeCam() const {

	CameraActions a{};
	
	a.moveX = in.isDown(GLFW_KEY_D) - in.isDown(GLFW_KEY_A);
	a.moveY = in.isDown(GLFW_KEY_W) - in.isDown(GLFW_KEY_S);
	a.lookX = in.getMouseX();
	a.lookY = in.getMouseY();
	a.scroll = in.getScrollDelta();
	
	return a;	
}