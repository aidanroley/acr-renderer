#include "pch.h"
#include "glEng/gl_engine.h"
#include "Core/window.h"

glEngine::glEngine() : _transmissionPass(this) {}

void glEngine::setupEngine() {

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) throw std::runtime_error("failed to create window");
	glViewport(0, 0, Window::getResWidth(), Window::getResHeight());
	glEnable(GL_FRAMEBUFFER_SRGB);

	bindCameraUBO();
	setDefaultValues();
	_transmissionPass.createTransmissionTargets(Window::getResWidth(), Window::getResHeight(), 4);
	_cubeMap.init("assets/christmas.hdr");

	glEnable(GL_DEPTH_TEST);

	std::shared_ptr<gltfData> scene = gltfData::Load(this, "assets/Chess_Edit.glb");
	//std::shared_ptr<gltfData> scene = gltfData::Load(this, "assets/DragonAttenuation.glb");
	//std::shared_ptr<gltfData> scene = gltfData::Load(this, "assets/Duck.glb");
	scene->drawNodes(_gltfData.ctx);
}

//void setPBRLoc()

void glEngine::setDefaultValues() {

	_defaultSamplerLinear = createDefaultLinearSampler();
	_whiteImage = createDefaultWhiteTexture();

	_gltfData.prog.makeShaderProgram("shaders/gl/pbr_v.glsl", "shaders/gl/pbr_f.glsl");

	_gltfData.modelLoc = glGetUniformLocation(_gltfData.prog.getID(), "model");

	setDebugDefaults();
}

void glEngine::setDebugDefaults() {

	auto [sphereVerts, sphereIndices] = generateDebugSphere();
	_lightSphere.mesh = createDebugMesh(sphereVerts, sphereIndices);
	_lightSphere.prog.makeShaderProgram("shaders/gl/light_debug_v.glsl", "shaders/gl/light_debug_f.glsl");
	_lightSphere.modelLoc = glGetUniformLocation(_lightSphere.prog.getID(), "model");
	_lightSphere.colorLoc = glGetUniformLocation(_lightSphere.prog.getID(), "lightColor");
	
}

void glEngine::bindCameraUBO() {

	glGenBuffers(1, &_cameraUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, _cameraUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraUBO), nullptr, GL_DYNAMIC_DRAW);

	// bind the buffer to binding point 0 (for example)
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, _cameraUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}


void glEngine::drawFrame() {

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	_cubeMap.Draw();
	drawGltf();
	drawDebugMesh();
}

void glEngine::drawGltf() {

	glUseProgram(_gltfData.prog.getID());
	GLuint irTexMap = _cubeMap.getIRTex();
	_gltfData.prog.setTexture("iradianceMap", irTexMap); // try (, 0)


	if (_gltfData.ctx.isTransmissionEnabled) {

		_transmissionPass.drawTransmission();
	}

	else drawNoExtensions(); 

	//drawNoExtensions();

}

void glEngine::drawNoExtensions() {

	//glUseProgram(_gltfData.prog.getID());
	for (const auto& submesh : _gltfData.ctx.opaqueSubmeshes) {

		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
		drawGltfMesh(submesh);
	}
	// same settings as opaque but needs to be drawn after them anyways.
	for (const auto& submesh : _gltfData.ctx.transmissionSubmeshes) {

		drawGltfMesh(submesh);
	}

	for (const auto& submesh : _gltfData.ctx.transparentSubmeshes) {

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
		drawGltfMesh(submesh);
	}
}

void glEngine::drawGltfMesh(const RenderObject& submesh) {

	glm::mat4 model = submesh.transform;

	glUniformMatrix4fv(_gltfData.modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	auto& mat = submesh.material;
	auto& res = mat->data.resources;

	GLuint irTexMap = _cubeMap.getIRTex();
	_gltfData.prog.setTexture("irradianceMap", irTexMap, 0, GL_TEXTURE_CUBE_MAP); // try (, 0)

	_gltfData.prog.setTexture("albedoTex", mat->data.resources.albedo.image.id, mat->data.resources.albedo.sampler.id);
	_gltfData.prog.setTexture("metalRoughTex", mat->data.resources.metalRough.image.id, mat->data.resources.metalRough.sampler.id);
	_gltfData.prog.setTexture("occlusionTex", mat->data.resources.occlusion.image.id, mat->data.resources.occlusion.sampler.id);
	_gltfData.prog.setTexture("normalTex", mat->data.resources.normalMap.image.id, mat->data.resources.normalMap.sampler.id);
	_gltfData.prog.setTexture("transmissionTex", mat->data.resources.transmission.image.id, mat->data.resources.transmission.sampler.id);
	_gltfData.prog.setTexture("thicknessTex", mat->data.resources.volumeThickness.image.id, mat->data.resources.volumeThickness.sampler.id);


	glBindBufferRange(GL_UNIFORM_BUFFER, 8, mat->data.resources.dataBuffer, mat->data.resources.dataBufferOffset, sizeof(PBRSystem::MaterialPBRConstants));
	glBindVertexArray(submesh.meshBuffers.vao);
	glDrawElements(
		GL_TRIANGLES,
		submesh.numIndices,
		GL_UNSIGNED_INT,
		(void*)(sizeof(uint32_t) * submesh.idxStart)
	);
}

void glEngine::drawDebugMesh() {

	glUseProgram(_lightSphere.prog.getID());

	std::vector<glm::vec3> lightPos = { glm::vec3(0.6f, 1.2f, 0.5f), glm::vec3(1.0f, 1.2f, -0.8f) };

	for (int i = 0; i < lightPos.size(); i++) {

		glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), lightPos[i]);

		lightModel = glm::scale(lightModel, glm::vec3(0.1f)); // make it small

		glUniformMatrix4fv(_lightSphere.modelLoc, 1, GL_FALSE, glm::value_ptr(lightModel));

		glm::vec3 debugColor = glm::vec3(1.0f, 1.0f, 0.5f); // yellowish sphere
		glUniform3fv(_lightSphere.colorLoc, 1, glm::value_ptr(debugColor));

		glBindVertexArray(_lightSphere.mesh.vao);
		glDrawElements(GL_TRIANGLES, _lightSphere.mesh.indexCount, GL_UNSIGNED_INT, 0);
	}
}

GLImage glEngine::createDefaultWhiteTexture() {

	GLImage img{};
	img.width = 1;
	img.height = 1;
	img.format = GL_RGBA8;

	GLuint texID;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

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

void glEngine::passCameraData(glm::mat4 view, glm::mat4 proj, glm::vec4 viewPos) {

	CameraUBO camData;
	camData.view = view;
	camData.proj = proj;
	camData.viewPos = viewPos;

	glBindBuffer(GL_UNIFORM_BUFFER, _cameraUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraUBO), &camData);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	_cubeMap.updateCam(view, proj);
}


