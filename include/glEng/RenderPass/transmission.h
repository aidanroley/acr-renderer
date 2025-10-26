#pragma once
#include "Core/window.h"

class glEngine;
class ShaderProgram;

class TransmissionPass {

public:

	explicit TransmissionPass(glEngine* engine);

	void initPrevDepthFromScene();
	void renderOpaqueToSceneFBO();
	void renderPeelLayer(int i);
	void compositePeelLayers(int k);
	static void createTex2D(GLuint& id, GLenum internal, int w, int h, GLenum format, GLenum type, bool mipmapped);
	void createTransmissionTargets(int width, int height, int K);

	void drawTransmission();

private:

	struct TransmissionPeel {

		GLuint sceneFBO = 0; // opaques
		GLuint sceneColor = 0; // opqaque base color
		GLuint sceneDepth = 0;

		GLuint prevDepthTex = 0;
		GLuint peelDepthTex = 0;

		std::vector<GLuint> layerColor;	// GL_RGBA8 ?

		GLuint peelFBO = 0;
		GLuint compFBO = 0;

		GLuint compColor = 0; // final composited color

		GLuint samp6 = 0;
	} _gPeel;

	glEngine* _engine = nullptr;
	ShaderProgram* _prog;

	void allocPrevDepth(int w, int h);
	GLuint beginOcclusionQuery();
	GLuint getOcclusionQueryResults(GLuint query);

};


static void drawFullscreen(GLuint texture);
static void blitTextureToBackbuffer(GLuint srcColorTex, GLuint srcFramebuffer);
static void setDepthSampleParams(GLuint tex);