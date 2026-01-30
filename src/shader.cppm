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

	auto compileFromSrc(std::string_view source, GLenum type) -> u32 {
		const char* src = source.data();
		const GLint len = static_cast<GLint>(source.size());
		std::cerr << "length is: " << len << "\n";

		u32 shader = glCreateShader(type);
		glShaderSource(shader, 1, &src, &len);
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
};