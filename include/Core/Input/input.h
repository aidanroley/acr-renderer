#pragma once
struct InputDevice {

	static constexpr int maxKeys = 512;
	static constexpr int maxMouse = 8;

	std::array<bool, maxKeys> currKeys{}, prevKeys{};
	std::array<bool, maxMouse> currMB{}, prevMB{};
	
	float mouseX = 0, pMouseX = 0; 
	float mouseY = 0, pMouseY = 0;

	float scrollYDelta = 0.0f;

	// singleton
	static InputDevice& Get() {

		static InputDevice instance;
		return instance;
	}

	InputDevice(const InputDevice&) = delete;
	InputDevice& operator=(const InputDevice&) = delete;
	
	void beginFrame();

	bool isDown(int key) const;
	bool wentDown(int key) const;
	bool wentUp(int key) const;
	float getMouseX() const;
	float getMouseY() const;
	float getScrollDelta() const;

	void setKey(int key, bool pressed);
	void setMousePos(float x, float y);
	void setScrollDelta(float yOffset);
	
private:

	InputDevice() = default;
};