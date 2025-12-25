module;

#include <filesystem>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include "./external/vendor/glad.h"

export module shader;
import weretype;

export struct Shader {
	u32 m_ID;

	Shader(const std::string& vertexPath, const std::string& fragmentPath) {
		// Compile shaders
		u32 vertex = compileFromFile(vertexPath, GL_VERTEX_SHADER);
		u32 fragment = compileFromFile(fragmentPath, GL_FRAGMENT_SHADER);

		m_ID = glCreateProgram();
		glAttachShader(m_ID, vertex);
		glAttachShader(m_ID, fragment);
		glLinkProgram(m_ID);

		int success;
		char infoLog[512];
		glGetProgramiv(m_ID, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(m_ID, 512, NULL, infoLog);
			std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		} else {
			std::cout << "Shader program linked successfully." << std::endl;
		}

		// Delete shaders as they are no longer needed after linking
		glDeleteShader(vertex);
		glDeleteShader(fragment);
	}

	Shader(std::string_view vertexSrc, std::string_view fragmentSrc) {
		u32 vertex   = compileFromSrc(vertexSrc, GL_VERTEX_SHADER);
		u32 fragment = compileFromSrc(fragmentSrc, GL_FRAGMENT_SHADER);

		m_ID = glCreateProgram();
		glAttachShader(m_ID, vertex);
		glAttachShader(m_ID, fragment);
		glLinkProgram(m_ID);

		glDeleteShader(vertex);
		glDeleteShader(fragment);
	}

	void use() const {
		glUseProgram(m_ID);
	}

	void setFloat(const std::string& name, float value) const {
		int location = glGetUniformLocation(m_ID, name.c_str());
		if (location == -1) {
			std::cerr << "ERROR::SHADER::UNIFORM::" << name << " not found in shader." << std::endl;
		}
		glUniform1f(location, value);
	}

	void setFloatArray(const std::string& name, const float* values, int size) const {
		int location = glGetUniformLocation(m_ID, name.c_str());
		if (location == -1) {
			std::cerr << "ERROR::SHADER::UNIFORM::" << name << " not found in shader." << std::endl;
		}
		glUniform1fv(location, size, values);
	}

	u32 compileFromSrc(std::string_view source, GLenum type)
	{
		const char* src = source.data();

		u32 shader = glCreateShader(type);
		glShaderSource(shader, 1, &src, nullptr);
		glCompileShader(shader);

		int success;
		char infoLog[512];
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(shader, 512, nullptr, infoLog);
			std::cerr << "ERROR::SHADER::COMPILATION_FAILED::"
								<< (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT")
								<< "\n" << infoLog << std::endl;
		}

		return shader;
	}

	u32 compileFromFile(const std::string& relativePath, GLenum type) {

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
		u32 shader = glCreateShader(type);
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
};