#pragma once
#include "Core/IRenderEngine.h"
#include "glEng/gl_types.h"
#include "glEng/gltf_loader.h"
#include "glEng/Debug/debug_light.h"
#include "glEng/RenderPass/transmission.h"
#include "glEng/shader_prog.h"
#include "glEng/RenderPass/cubemap.h"

class glEngine : public IRenderEngine {
public:

	glEngine();

	void init(Renderer* r) override {

		_renderer = r;
	}

	struct alignas(16) CameraUBO {

		glm::mat4 view;
		glm::mat4 proj;
		glm::vec4 viewPos;
	};

	struct GLTFRenderData {

		ShaderProgram prog;
		GLuint VBO, VAO, EBO;
		GLint modelLoc;
		GltfDrawContext ctx;
	} _gltfData;

	// fbo = framebuffer object
	

	GLFWwindow* _window;
	Renderer* _renderer;
	GLImage _whiteImage;
	GLSampler _defaultSamplerLinear;
	GLuint _cameraUBO;
	void setupEngine() override;
	void drawFrame() override;
	void setWindow(GLFWwindow* w) override { _window = w; }
	void passCameraData(glm::mat4 view, glm::mat4 proj, glm::vec4 viewPos);

	void drawGltfMesh(const RenderObject& submesh); // publkic for transmission

	Cubemap _cubeMap;


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


	void setDefaultValues();
	void setDebugDefaults();
	GLSampler createDefaultLinearSampler();
	GLImage createDefaultWhiteTexture();

	void bindCameraUBO();
	void drawDebugMesh();
	void drawGltf();
	void drawNoExtensions();

	struct DebugSphere {

		ShaderProgram prog;
		GLint modelLoc;
		GLint colorLoc;
		DebugMesh mesh;
	};

	DebugSphere _lightSphere;
	TransmissionPass _transmissionPass;

};



