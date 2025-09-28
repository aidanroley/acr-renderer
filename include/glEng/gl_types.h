#pragma once

struct GLImage {

	GLuint id;
	int width;
	int height;
	GLenum format;

	GLuint linearID;
	GLuint sRGBID;
};

struct GLSampler {

    GLuint id;
    GLenum minFilter;
    GLenum magFilter;
    GLenum wrapS;
    GLenum wrapT;
};

struct GLTexture {

    GLImage image;
    GLSampler sampler;
};

struct Vertex {

	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec4 tangent;
};

struct Texture {

	GLuint texID;
	GLuint samplerID;
	std::string type;
	std::string path;
};

