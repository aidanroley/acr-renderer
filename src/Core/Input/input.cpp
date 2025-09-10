#include "pch.h"
#include "Core/Input/input.h"

void InputDevice::beginFrame() {

	prevKeys = currKeys;
	prevMB = currMB;

	pMouseX = mouseX;
	pMouseY = mouseY;

	scrollYDelta = 0.0f;
}

// these 4 r just for keyboard keys not mouse buttonsdont 
bool InputDevice::isDown(int key) const {

	return currKeys[key];
}

bool InputDevice::wentDown(int key) const {

	return (currKeys[key] && !prevKeys[key]);
}

bool InputDevice::wentUp(int key) const {

	return (!currKeys[key] && prevKeys[key]);
}

void InputDevice::setKey(int key, bool pressed) {

	currKeys[key] = pressed;
}

void InputDevice::setMousePos(float x, float y) {

	mouseX = x;
	mouseY = y;
}

float InputDevice::getMouseX() const {

	return mouseX;
}

float InputDevice::getMouseY() const {

	return mouseY;
}

void InputDevice::setScrollDelta(float yOffset) {

	scrollYDelta += yOffset;
}

float InputDevice::getScrollDelta() const {

	return scrollYDelta;
}