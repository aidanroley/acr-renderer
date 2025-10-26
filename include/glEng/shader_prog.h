#pragma once

class ShaderProgram {
public:

	GLuint makeShaderProgram(const char* vsPath, const char* fsPath);
	const void setTexture(const std::string& uniformName, GLuint texture, GLint sampler = -1, GLuint texFormat = GL_TEXTURE_2D);
	GLuint getID() { return _programID; }
	GLuint getUniformAddress(const std::string& uniformName);
	void useProg();

	void setInt(const std::string& name, int value) const;
	void setFloat(const std::string& name, float value) const;
	void setVec3(const std::string& name, const glm::vec3& value) const;
	void setMat4(const std::string& name, const glm::mat4& mat) const;

	const char* _vsPath;
	const char* _fsPath;

private:

	GLuint _programID;
	GLuint compileShader(GLenum type, const std::string& src);
	GLuint linkProgram(GLuint vs, GLuint fs);

	int _nextTexUnit = 0;
	std::unordered_map<std::string, int> _textureUnits;
};
std::string loadFile(const char* path);