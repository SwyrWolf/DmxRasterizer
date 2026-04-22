module;

#include <iostream>
#include <string_view>
#include <expected>

#include "glad.h"

export module shader;
import weretype;

export struct Shader {
	u32 m_ID;

	Shader() = default;

	static auto Create(std::string_view vertexSrc, std::string_view fragmentSrc) -> std::expected<Shader, std::string> {
		auto vertex = compileFromSrc(vertexSrc, GL_VERTEX_SHADER);
		if (!vertex) return std::unexpected(vertex.error());

		auto fragment = compileFromSrc(fragmentSrc, GL_FRAGMENT_SHADER);
		if (!fragment) { glDeleteShader(*vertex); return std::unexpected(fragment.error()); }

		Shader s;
		s.m_ID = glCreateProgram();
		glAttachShader(s.m_ID, *vertex);
		glAttachShader(s.m_ID, *fragment);
		glLinkProgram(s.m_ID);

		glDeleteShader(*vertex);
		glDeleteShader(*fragment);

		GLint success;
		glGetProgramiv(s.m_ID, GL_LINK_STATUS, &success);
		if (!success) {
			char log[512];
			glGetProgramInfoLog(s.m_ID, 512, nullptr, log);
			glDeleteProgram(s.m_ID);
			return std::unexpected(std::string("Shader link failed: ") + log);
		}

		return s;
	}

	void use() const {
		glUseProgram(m_ID);
	}

private:
	static auto compileFromSrc(std::string_view source, GLenum type) -> std::expected<u32, std::string> {
		const char* src = source.data();
		const GLint len = as<GLint>(source.size());

		u32 shader = glCreateShader(type);
		glShaderSource(shader, 1, &src, &len);
		glCompileShader(shader);

		int success;
		char log[512];
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(shader, 512, nullptr, log);
			return std::unexpected(
				std::string(type == GL_VERTEX_SHADER ? "Vertex" : "Fragment") 
				+ " shader failed: " + log
			);
		}

		return shader;
	}
};