#pragma once
#include "Core/IRenderEngine.h"
#include "glEng/gl_types.h"
#include "glEng/gltf_loader.h"
#include "glEng/Debug/debug_light.h"

class glEngine : public IRenderEngine {
public:

	void init(Renderer* r) override {

		_renderer = r;
	}

	struct alignas(16) CameraUBO {

		glm::mat4 view;
		glm::mat4 proj;
		glm::vec3 viewPos;
	};

	void setupEngine() override;
	void drawFrame() override;
	void setWindow(GLFWwindow* w) override { _window = w; }
	GLuint getProgram() { return _prog; }

	void passCameraData(glm::mat4 view, glm::mat4 proj, glm::vec3 viewPos);

	GLFWwindow* _window;
	Renderer* _renderer;

	GLImage _whiteImage;
	GLSampler _defaultSamplerLinear;

	GLuint _cameraUBO;
	GLint _modelLoc;

private:

	static constexpr float vertices[18] = {
		0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  
		-0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  
		 0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f
	};

	static constexpr uint32_t indices[6] = {  
	0, 1, 3,   
	1, 2, 3    
	};

	GLuint _VBO, _VAO, _EBO;
	GLuint _prog;

	DrawContext _ctx;

	GLSampler createDefaultLinearSampler();
	GLImage createDefaultWhiteTexture();
	GLuint makeShaderProgram(const char* vsPath, const char* fsPath);
	GLuint compileShader(GLenum type, const std::string& src);
	GLuint linkProgram(GLuint vs, GLuint fs);
	void bindCameraUBO();


	struct DebugSphere {

		GLuint prog;
		GLint modelLoc;
		GLint colorLoc;
		DebugMesh mesh;
	};

	DebugSphere _lightSphere;
};

