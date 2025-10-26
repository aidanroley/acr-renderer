#include "pch.h"
#include "glEng/shader_prog.h"

GLuint ShaderProgram::makeShaderProgram(const char* vsPath, const char* fsPath) {

	std::string vsSource = loadFile(vsPath);
	std::string fsSource = loadFile(fsPath);

	GLuint vs = compileShader(GL_VERTEX_SHADER, vsSource);
	GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSource);
	GLuint prog = linkProgram(vs, fs);

	glDeleteShader(vs);
	glDeleteShader(fs);

	_vsPath = vsPath;
	_fsPath = fsPath;
	_programID = prog;
	return prog;
}

GLuint ShaderProgram::compileShader(GLenum type, const std::string& src) {

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

GLuint ShaderProgram::linkProgram(GLuint vs, GLuint fs) {

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

std::string loadFile(const char* path) {

	std::ifstream file(path, std::ios::in | std::ios::binary);
	if (!file) throw std::runtime_error(std::string("Failed to open file: ") + path);

	std::ostringstream contents;
	contents << file.rdbuf();
	return contents.str();
}


// gets uniform address from uniformName, then binds a texture to active slot, then binds sampler, then lastly binds the unit to the loc
const void ShaderProgram::setTexture(const std::string& uniformName, GLuint texture, GLint sampler, GLuint texFormat) {

	glUseProgram(this->getID());

	if (_textureUnits.count(uniformName) == 0) {

		_textureUnits[uniformName] = _nextTexUnit++;
	}
	int unit = _textureUnits[uniformName];

	GLint loc = glGetUniformLocation(this->getID(), uniformName.c_str());

	if (loc == -1) { 

		//std::cerr << "Warning:: uniform " << uniformName << "not found in shader :(" << std::endl; 
		return;
	}

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(texFormat, texture);

	if (sampler != -1) { 

		glBindSampler(unit, sampler); 
	}
	else { 

		glBindSampler(unit, 1); 
	}

	glUniform1i(loc, unit);
}

void ShaderProgram::useProg() {

	glUseProgram(_programID);
}

GLuint ShaderProgram::getUniformAddress(const std::string& uniformName) {

	return glGetUniformLocation(_programID, uniformName.c_str());
}

void ShaderProgram::setInt(const std::string& name, int value) const {

	GLint loc = glGetUniformLocation(_programID, name.c_str());
	if (loc == -1) {
		std::cerr << "Warning: uniform '" << name << "' not found in shader.\n";
	}
	glUniform1i(loc, value);
}

void ShaderProgram::setMat4(const std::string& name, const glm::mat4& mat) const {

	GLint loc = glGetUniformLocation(_programID, name.c_str());
	if (loc == -1) {
		std::cerr << "Warning: uniform '" << name << "' not found in shader.\n";
	}
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
}

void ShaderProgram::setFloat(const std::string& name, float value) const {

	GLint loc = glGetUniformLocation(_programID, name.c_str());
	if (loc == -1) {
		std::cerr << "[ShaderProgram] Warning: uniform '" << name << "' not found.\n";
		return;
	}
	glUniform1f(loc, value);
}

void ShaderProgram::setVec3(const std::string& name, const glm::vec3& value) const {

	GLint loc = glGetUniformLocation(_programID, name.c_str());
	if (loc == -1) {
		std::cerr << "[ShaderProgram] Warning: uniform '" << name << "' not found.\n";
		return;
	}
	glUniform3fv(loc, 1, glm::value_ptr(value));
}


