#include "pch.h"
#include "glEng/gl_engine.h"


namespace {

	const char* vertexShaderSource = "#version 330 core\n"
		"layout (location = 0) in vec3 aPos;\n"
		"void main()\n"
		"{\n"
		"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
		"}\0";

	const char* fragmentShaderSource = "#version 330 core\n"
		"out vec4 FragColor;\n"
		"void main()\n"
		"{\n"
		"   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
		"}\0";
}


void glEngine::setupEngine() {

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) throw std::runtime_error("failed to create window");
	glViewport(0, 0, 1440, 810);

	glEnable(GL_FRAMEBUFFER_SRGB);

	bindCameraUBO();

	_defaultSamplerLinear = createDefaultLinearSampler();
	_whiteImage = createDefaultWhiteTexture();

	_prog = makeShaderProgram("shaders/gl/vertex.glsl", "shaders/gl/frag.glsl");

	_modelLoc = glGetUniformLocation(_prog, "model");


	glEnable(GL_DEPTH_TEST);

	// Debug...
	auto [sphereVerts, sphereIndices] = generateDebugSphere();
	_lightSphere.mesh = createDebugMesh(sphereVerts, sphereIndices);
	_lightSphere.prog = makeShaderProgram("shaders/gl/light_debug.vert", "shaders/gl/light_debug.frag");
	_lightSphere.modelLoc = glGetUniformLocation(_lightSphere.prog, "model");
	_lightSphere.colorLoc = glGetUniformLocation(_lightSphere.prog, "lightColor");
	// debug End...

	std::shared_ptr<gltfData> scene = gltfData::Load(this, "assets/Chess.glb");
	scene->drawNodes(_ctx);
}

void glEngine::bindCameraUBO() {

	glGenBuffers(1, &_cameraUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, _cameraUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraUBO), nullptr, GL_DYNAMIC_DRAW);

	// bind the buffer to binding point 0 (for example)
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, _cameraUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

std::string loadFile(const char* path) {

	std::ifstream file(path, std::ios::in | std::ios::binary);
	if (!file) throw std::runtime_error(std::string("Failed to open file: ") + path);

	std::ostringstream contents;
	contents << file.rdbuf();
	return contents.str();
}

GLuint glEngine::compileShader(GLenum type, const std::string& src) {

	GLuint shader = glCreateShader(type);
	const char* csrc = src.c_str();
	glShaderSource(shader, 1, &csrc, nullptr);
	glCompileShader(shader);

	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {

		GLint logLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
		std::string log(logLen, '\0');
		glGetShaderInfoLog(shader, logLen, nullptr, log.data());
		std::cerr << "Shader compilation failed: " << log << std::endl;
		glDeleteShader(shader);
		throw std::runtime_error("Shader compile error");
	}
	return shader;
}

GLuint glEngine::linkProgram(GLuint vs, GLuint fs) {

	GLuint program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);

	GLint success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {

		GLint logLen = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
		std::string log(logLen, '\0');
		glGetProgramInfoLog(program, logLen, nullptr, log.data());
		std::cerr << "Program linking failed: " << log << std::endl;
		glDeleteProgram(program);
		throw std::runtime_error("Shader link error");
	}
	return program;
}

GLuint glEngine::makeShaderProgram(const char* vsPath, const char* fsPath) {

	std::string vsSource = loadFile(vsPath);
	std::string fsSource = loadFile(fsPath);

	GLuint vs = compileShader(GL_VERTEX_SHADER, vsSource);
	GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSource);
	GLuint prog = linkProgram(vs, fs);

	glDeleteShader(vs);
	glDeleteShader(fs);
	return prog;
}

void glEngine::drawFrame() {

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(_prog);

	for (const auto& obj : _ctx.submeshes) {

		glm::mat4 model = obj.transform;

		glUniformMatrix4fv(_modelLoc, 1, GL_FALSE, glm::value_ptr(model));

		auto& mat = obj.material;

		GLint albedoLoc = glGetUniformLocation(_prog, "albedoTex");
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mat->data.resources.albedo.image.id);
		glBindSampler(0, mat->data.resources.albedo.sampler.id);
		glUniform1i(albedoLoc, 0);
		
		// Metal/roughness to texture unit 1
		GLint mrLoc = glGetUniformLocation(_prog, "metalRoughTex");
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, mat->data.resources.metalRough.image.id);
		glBindSampler(1, mat->data.resources.metalRough.sampler.id);
		glUniform1i(mrLoc, 1);

		// Occlusion map to texture unit 2
		GLint occLoc = glGetUniformLocation(_prog, "occlusionTex");
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, mat->data.resources.occlusion.image.id);
		glBindSampler(2, mat->data.resources.occlusion.sampler.id);
		glUniform1i(occLoc, 2);

		// Normal map to texture unit 3
		GLint normLoc = glGetUniformLocation(_prog, "normalTex");
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, mat->data.resources.normalMap.image.id);
		glBindSampler(3, mat->data.resources.normalMap.sampler.id);
		glUniform1i(normLoc, 3);


		GLint trLoc = glGetUniformLocation(_prog, "transmissionTex");
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, mat->data.resources.transmission.image.id);
		glBindSampler(4, mat->data.resources.transmission.sampler.id);
		glUniform1i(trLoc, 4);


		GLint thLoc = glGetUniformLocation(_prog, "thicknessTex");
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, mat->data.resources.volumeThickness.image.id);
		glBindSampler(5, mat->data.resources.volumeThickness.sampler.id);
		glUniform1i(thLoc, 5);

		glBindBufferRange(GL_UNIFORM_BUFFER, 6, mat->data.resources.dataBuffer, mat->data.resources.dataBufferOffset, sizeof(PBRSystem::MaterialPBRConstants));
		

		glBindVertexArray(obj.meshBuffers.vao);
		glDrawElements(
			GL_TRIANGLES,
			obj.numIndices,
			GL_UNSIGNED_INT,
			(void*)(sizeof(uint32_t) * obj.idxStart)
		);
	}

	// --- Draw debug light sphere ---
	glUseProgram(_lightSphere.prog);

	// Example: place the sphere at world-space (2.0, 0.4, 0.5)
	glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.6f, 0.4f, 0.5f));
	lightModel = glm::scale(lightModel, glm::vec3(0.1f)); // make it small

	glUniformMatrix4fv(_lightSphere.modelLoc, 1, GL_FALSE, glm::value_ptr(lightModel));

	glm::vec3 debugColor = glm::vec3(1.0f, 1.0f, 0.5f); // yellowish sphere
	glUniform3fv(_lightSphere.colorLoc, 1, glm::value_ptr(debugColor));

	glBindVertexArray(_lightSphere.mesh.vao);
	glDrawElements(GL_TRIANGLES, _lightSphere.mesh.indexCount, GL_UNSIGNED_INT, 0);
}

GLImage glEngine::createDefaultWhiteTexture() {

	GLImage img{};
	img.width = 1;
	img.height = 1;
	img.format = GL_RGBA8;

	GLuint texID;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	// 1x1 white pixel
	uint8_t whitePixel[4] = { 255, 255, 255, 255 };
	glTexImage2D(GL_TEXTURE_2D,
		0,
		GL_RGBA8,
		1, 1,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		whitePixel);

	glBindTexture(GL_TEXTURE_2D, 0);

	img.id = texID;
	return img;
}

GLSampler glEngine::createDefaultLinearSampler() {

	GLSampler sampler{};

	GLuint samplerID;
	glGenSamplers(1, &samplerID);

	// linear filtering
	glSamplerParameteri(samplerID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glSamplerParameteri(samplerID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// wrap mode repeat
	glSamplerParameteri(samplerID, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glSamplerParameteri(samplerID, GL_TEXTURE_WRAP_T, GL_REPEAT);

	sampler.id = samplerID;
	sampler.minFilter = GL_LINEAR_MIPMAP_LINEAR;
	sampler.magFilter = GL_LINEAR;
	sampler.wrapS = GL_REPEAT;
	sampler.wrapT = GL_REPEAT;

	return sampler;
}

void glEngine::passCameraData(glm::mat4 view, glm::mat4 proj, glm::vec3 viewPos) {

	CameraUBO camData;
	camData.view = view;
	camData.proj = proj;
	camData.viewPos = viewPos;

	glBindBuffer(GL_UNIFORM_BUFFER, _cameraUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraUBO), &camData);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

