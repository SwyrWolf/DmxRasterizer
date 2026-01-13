module;

#include <iostream>
#include <expected>
#include <array>

#include "../../external/vendor/glad.h"
#include <glfw3.h>
#include <SpoutGL/SpoutSender.h>

export module render;
import weretype;
import appState;

f32 vertices[] = {
	-1.0f,  1.0f,  0.0f, 1.0f, // Top-left
	-1.0f, -1.0f,  0.0f, 0.0f, // Bottom-left
	1.0f, -1.0f,  1.0f, 0.0f, // Bottom-right
	
	-1.0f,  1.0f,  0.0f, 1.0f, // Top-left
	1.0f, -1.0f,  1.0f, 0.0f, // Bottom-right
	1.0f,  1.0f,  1.0f, 1.0f  // Top-right
};

constexpr int RENDER_WIDTH = 1920;
constexpr int RENDER_HEIGHT = 208;

export namespace Render {
	enum class RenderError {
		glfwFailed,
		windowCreationFailed,
		gladFailed,
	};

	GLuint dmxDataTexture;
	
	auto BeginRenderer(std::array<u8, 1560>& array) -> std::expected<void, RenderError> {
		if (!glfwInit()) {
			return std::unexpected(RenderError::glfwFailed);
		}
		
		if(!app::debugMode) {
			glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		}
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		GLFWwindow* window = glfwCreateWindow(RENDER_WIDTH, RENDER_HEIGHT, "DMX Shader Renderer", nullptr, nullptr);

		if (!window) {
			glfwTerminate();
			return std::unexpected(RenderError::windowCreationFailed);
		}
		glfwMakeContextCurrent(window);
		
		if (!gladLoadGLLoader(raw<GLADloadproc>(glfwGetProcAddress))) {
			std::cerr << "Failed to initialize GLAD" << std::endl;
			return std::unexpected(RenderError::gladFailed);
		}

		glViewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);

		glGenTextures(1, &dmxDataTexture);
		glBindTexture(GL_TEXTURE_1D, dmxDataTexture);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, 1560, 0, GL_RED, GL_FLOAT, array.data());
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		return {};
	}
}