#pragma once
#include "Core/Input/input.h"

enum class InputMode { FreeCam, GUI };

struct CameraActions {

	int moveX = 0;
	int moveY = 0;

	float lookX = 0.f;
	float lookY = 0.f;

	float scroll = 0.f;

	bool toggleGui = false;
};

struct ActionMap {

	InputDevice& in = InputDevice::Get();
	CameraActions buildFreeCam() const;
};

