#include "pch.h"
#include "glEng/gl_engine.h"
#include "glEng/RenderPass/transmission.h"
#include "glEng/gltf_loader.h"

TransmissionPass::TransmissionPass(glEngine* engine) : _engine(engine) {

	_prog = &_engine->_gltfData.prog;
}

void TransmissionPass::drawTransmission() {

	//allocPrevDepth(Window::getResWidth(), Window::getResHeight(),);

	int K = 4;
	_gPeel.layerColor.resize(K);

	renderOpaqueToSceneFBO();

	for (int i = 0; i < K; ++i) renderPeelLayer(i);

	compositePeelLayers(K);
}

void TransmissionPass::initPrevDepthFromScene() {

	glBindFramebuffer(GL_READ_FRAMEBUFFER, _gPeel.sceneFBO);
	glBindTexture(GL_TEXTURE_2D, _gPeel.prevDepthTex);

	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 0, 0, Window::getResWidth(), Window::getResHeight(), 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void TransmissionPass::renderOpaqueToSceneFBO() {

	glBindFramebuffer(GL_FRAMEBUFFER, _gPeel.sceneFBO);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glViewport(0, 0, Window::getResWidth(), Window::getResHeight());
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LESS);

	_prog->useProg();

	// make this into a method (below)
	for (const auto& submesh : _engine->_gltfData.ctx.opaqueSubmeshes) _engine->drawGltfMesh(submesh);

	// generate mips for sceneColor
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, _gPeel.sceneColor);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	initPrevDepthFromScene();

	// maybe move this line to after opaques are drawn?
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);
	_engine->_cubeMap.Draw();  // Draws to currently bound FBO (sceneFBO)
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
}

void TransmissionPass::renderPeelLayer(int i) {

	glBindFramebuffer(GL_FRAMEBUFFER, _gPeel.peelFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _gPeel.layerColor[i], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _gPeel.peelDepthTex, 0);

	GLenum bufs[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, bufs);

	glViewport(0, 0, Window::getResWidth(), Window::getResHeight());
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);

	_prog->useProg();

	GLint uPrevDepthLoc = _prog->getUniformAddress("uPrevDepth");
	GLint uSceneColorLoc = _prog->getUniformAddress("uSceneColor");
	//GLint uPrevDepthLoc = glGetUniformLocation(_engine->_gltfData.prog.getID(), "uPrevDepth");
	//GLint uSceneColorLoc = glGetUniformLocation(_engine->_gltfData.prog.getID(), "uSceneColor");

	// bind depth texture
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, _gPeel.prevDepthTex);
	glUniform1i(uPrevDepthLoc, 7);

	// bind scene color texture
	// first pass, opaque, after look at prev one
	glActiveTexture(GL_TEXTURE6);
	if (i == 0) {

		glBindTexture(GL_TEXTURE_2D, _gPeel.sceneColor);
	}
	else {

		glBindTexture(GL_TEXTURE_2D, _gPeel.layerColor[i - 1]);
	}
	glGenerateMipmap(GL_TEXTURE_2D);

	glUniform1i(uSceneColorLoc, 6); // set the int value of the uniform at location uSceneColorLoc to 6

	GLuint query = beginOcclusionQuery();

	for (const auto& submesh : _engine->_gltfData.ctx.transmissionSubmeshes) _engine->drawGltfMesh(submesh);

	GLuint any = getOcclusionQueryResults(query);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (any) {

		glCopyImageSubData(
			_gPeel.peelDepthTex, GL_TEXTURE_2D, 0, 0, 0, 0,
			_gPeel.prevDepthTex, GL_TEXTURE_2D, 0, 0, 0, 0,
			Window::getResWidth(), Window::getResHeight(), 1
		);
	}
	
}

GLuint TransmissionPass::beginOcclusionQuery() {

	GLuint query = 0;
	glGenQueries(1, &query);
	glBeginQuery(GL_ANY_SAMPLES_PASSED, query);
	return query;
}

GLuint TransmissionPass::getOcclusionQueryResults(GLuint query) {

	GLuint any = 0;
	glGetQueryObjectuiv(query, GL_QUERY_RESULT, &any);
	glDeleteQueries(1, &query);
	return any;
}

void TransmissionPass::compositePeelLayers(int k) {

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_FRAMEBUFFER_SRGB);

	blitTextureToBackbuffer(_gPeel.sceneColor, _gPeel.sceneFBO);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	for (int i = k - 1; i >= 0; --i) {

		drawFullscreen(_gPeel.layerColor[i]);
	}
	glDisable(GL_BLEND);

}

static int mipCount(int w, int h) {

	int m = 1; while ((w | h) >> m) ++m; return m; // ceil(log2(max(w,h)))+1
}

// K layers of color textures
void TransmissionPass::createTex2D(GLuint& id, GLenum internal, int w, int h, GLenum format, GLenum type, bool mipmapped) {

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	const int levels = mipmapped ? mipCount(w, h) : 1;

	// Allocate *each* mip level with the SAME formats
	for (int lvl = 0; lvl < levels; ++lvl) {
		int lw = std::max(1, w >> lvl);
		int lh = std::max(1, h >> lvl);
		glTexImage2D(GL_TEXTURE_2D, lvl, internal, lw, lh, 0, format, type, nullptr);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		mipmapped ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, levels - 1);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void TransmissionPass::createTransmissionTargets(int width, int height, int K) {

	// sampler for reading each peeled layer
	GLuint samp6; glGenSamplers(1, &samp6);
	glSamplerParameteri(samp6, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glSamplerParameteri(samp6, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glSamplerParameteri(samp6, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(samp6, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindSampler(6, samp6);

	// scene FBO
	glGenFramebuffers(1, &_gPeel.sceneFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, _gPeel.sceneFBO);

	createTex2D(_gPeel.sceneColor, GL_SRGB8_ALPHA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, true);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _gPeel.sceneColor, 0);

	glGenRenderbuffers(1, &_gPeel.sceneDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, _gPeel.sceneDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _gPeel.sceneDepth);

	GLenum bufs0[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, bufs0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) std::cout << "framebuffer not complete" << std::endl;

	// peel FBO
	glGenFramebuffers(1, &_gPeel.peelFBO);
	createTex2D(_gPeel.prevDepthTex, GL_DEPTH_COMPONENT24, width, height, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, false);
	setDepthSampleParams(_gPeel.prevDepthTex);
	createTex2D(_gPeel.peelDepthTex, GL_DEPTH_COMPONENT24, width, height, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, false);

	// n layer color textures
	_gPeel.layerColor.resize(K);
	for (int i = 0; i < K; i++) {

		createTex2D(_gPeel.layerColor[i], GL_RGBA16F, width, height, GL_RGBA, GL_HALF_FLOAT, true);
	}

	// composite texture
	glGenFramebuffers(1, &_gPeel.compFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, _gPeel.compFBO);
	createTex2D(_gPeel.compColor, GL_SRGB8_ALPHA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, false);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _gPeel.compColor, 0);
	glDrawBuffers(1, bufs0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) std::cout << "framebuffer not complete" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}


// if upsire down swap Y corrds in glBlitFramebuffer
void blitTextureToBackbuffer(GLuint srcColorTex, GLuint srcFramebuffer) {

	GLuint tempFBO = 0;
	if (srcFramebuffer) {

		glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFramebuffer);
		// Make sure we read from color attachment 0 (common case)
		glReadBuffer(GL_COLOR_ATTACHMENT0);
	}
	else {

		glGenFramebuffers(1, &tempFBO);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, tempFBO);
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, srcColorTex, 0);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		assert(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	const GLbitfield mask = GL_COLOR_BUFFER_BIT;
	glBlitFramebuffer(
		0, 0, Window::getResWidth(), Window::getResHeight(),
		0, 0, Window::getResWidth(), Window::getResHeight(),
		mask,
		GL_NEAREST // use GL_LINEAR if want filtered copy
	);

	if (tempFBO) {

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &tempFBO);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void drawFullscreen(GLuint texture) {

	static GLuint sVAO = 0, sVBO = 0, sEBO = 0, sProg = 0;
	static GLint  sLocTex = -1;

	if (sProg == 0) {

		const char* vsSrc = R"(#version 330 core
            layout (location = 0) in vec2 aPos;      
            layout (location = 1) in vec2 aUV;
            out vec2 vUV;
            void main() {
                vUV = aUV;
                gl_Position = vec4(aPos, 0.0, 1.0);
            }
        )";

		const char* fsSrc = R"(#version 330 core
            in vec2 vUV;
            out vec4 FragColor;
            uniform sampler2D uTex;
            void main() {
                FragColor = textureLod(uTex, vUV, 0.0);
            }
        )";

		auto compile = [](GLenum type, const char* src)->GLuint {

			GLuint s = glCreateShader(type);
			glShaderSource(s, 1, &src, nullptr);
			glCompileShader(s);
			GLint ok = GL_FALSE; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
			if (!ok) { char log[2048]; GLsizei n = 0; glGetShaderInfoLog(s, 2048, &n, log); fprintf(stderr, "%s\n", log); }
			return s;
			};

		GLuint vs = compile(GL_VERTEX_SHADER, vsSrc);
		GLuint fs = compile(GL_FRAGMENT_SHADER, fsSrc);

		sProg = glCreateProgram();
		glAttachShader(sProg, vs);
		glAttachShader(sProg, fs);
		glLinkProgram(sProg);
		glDeleteShader(vs);
		glDeleteShader(fs);

		sLocTex = glGetUniformLocation(sProg, "uTex");

		// verts in screen/NDC order: BL, BR, TR, TL
		const float quad[] = {
			-1.f,-1.f,  0.f,0.f,  // 0
			 1.f,-1.f,  1.f,0.f,  // 1
			 1.f, 1.f,  1.f,1.f,  // 2
			-1.f, 1.f,  0.f,1.f   // 3
		};
		const unsigned int idx[] = { 0,1,2,  0,2,3 };  // both CCW

		glGenVertexArrays(1, &sVAO);
		glGenBuffers(1, &sVBO);
		glGenBuffers(1, &sEBO);

		glBindVertexArray(sVAO);

		glBindBuffer(GL_ARRAY_BUFFER, sVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	GLboolean cullOn = glIsEnabled(GL_CULL_FACE);
	if (cullOn) glDisable(GL_CULL_FACE);

	glUseProgram(sProg);

	static GLuint sNoMip = 0;
	if (!sNoMip) {
		glGenSamplers(1, &sNoMip);
		glSamplerParameteri(sNoMip, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // no mip requirement
		glSamplerParameteri(sNoMip, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glSamplerParameteri(sNoMip, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(sNoMip, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindSampler(0, sNoMip);//bufvfxi
	glUniform1i(sLocTex, 0);

	glBindVertexArray(sVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);

	if (cullOn) glEnable(GL_CULL_FACE);

}

static void setDepthSampleParams(GLuint tex) {

	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glBindTexture(GL_TEXTURE_2D, 0);
}
