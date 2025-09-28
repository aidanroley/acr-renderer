#pragma once

class Renderer;
class Window;

class IRenderEngine {
public:
	virtual ~IRenderEngine() = default;

	virtual void init(Renderer* renderer) = 0;
	virtual void setupEngine() = 0;
	virtual void drawFrame() = 0;
	virtual void setWindow(GLFWwindow* w) = 0;
};