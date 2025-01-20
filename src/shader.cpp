#include "shader.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
	// Compile shaders
	uint32_t vertex = compileShader(vertexPath, GL_VERTEX_SHADER);
	uint32_t fragment = compileShader(fragmentPath, GL_FRAGMENT_SHADER);

	ID = glCreateProgram();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);

	int success;
	char infoLog[512];
	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(ID, 512, NULL, infoLog);
		std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	} else {
		std::cout << "Shader program linked successfully." << std::endl;
	}

	// Delete shaders as they are no longer needed after linking
	glDeleteShader(vertex);
	glDeleteShader(fragment);
}

void Shader::use() const {
	glUseProgram(ID);
}

void Shader::setFloat(const std::string& name, float value) const {
	int location = glGetUniformLocation(ID, name.c_str());
	if (location == -1) {
		std::cerr << "ERROR::SHADER::UNIFORM::" << name << " not found in shader." << std::endl;
	}
	glUniform1f(location, value);
}

void Shader::setFloatArray(const std::string& name, const float* values, int size) const {
	int location = glGetUniformLocation(ID, name.c_str());
	if (location == -1) {
		std::cerr << "ERROR::SHADER::UNIFORM::" << name << " not found in shader." << std::endl;
	}
	glUniform1fv(location, size, values);
}

uint32_t Shader::compileShader(const std::string& relativePath, GLenum type) {

	std::filesystem::path exePath = std::filesystem::current_path();
	std::filesystem::path shaderPath = exePath / relativePath;
	// Read shader source from file
	std::ifstream shaderFile(shaderPath);
	if (!shaderFile.is_open()) {
		std::cerr << "ERROR::SHADER::FILE_NOT_FOUND::" << shaderPath << std::endl;
		exit(EXIT_FAILURE);
	}
	std::stringstream shaderStream;
	shaderStream << shaderFile.rdbuf();
	std::string code = shaderStream.str();
	const char* shaderCode = code.c_str();

	// Compile shader
	uint32_t shader = glCreateShader(type);
	glShaderSource(shader, 1, &shaderCode, NULL);
	glCompileShader(shader);

	// Check for compilation errors
	int success;
	char infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cerr << "ERROR::SHADER::COMPILATION_FAILED::" 
				  << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT") 
				  << "\n" << infoLog << std::endl;
	} else {
		std::cout << (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment")
				  << " shader compiled successfully: " << shaderPath << std::endl;
	}

	return shader;
}
