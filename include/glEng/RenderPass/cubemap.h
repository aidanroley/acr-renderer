#pragma once
#include "glEng/shader_prog.h"

class Cubemap {
public:

	void init(const std::filesystem::path filepath);
    void updateCam(const glm::mat4 view, const glm::mat4 proj);
    GLuint getIRTex() { return _irMapTex; }
    void Draw();

private:

	void loadHDRTexture(const std::filesystem::path filepath);
    void setupCubeVertices();
    void setupShaderProg();
    void generateCubemap();
    void generateIDRTexture();
	ShaderProgram* _equiToCubeProg;
    ShaderProgram* _skyboxProg;
    ShaderProgram* _idrProg;
	uint32_t _hdrTexture;
    glm::mat4 _view;
    glm::mat4 _proj;

    GLuint _cubeVAO, _cubeVBO;
    GLuint _capFBO, _capRBO;
    GLuint _cubemapTex;
    GLuint _irMapTex;
};
