// Core Rendering module

module;

#include <iostream>
#include <expected>
#include <span>
#include <ranges>

#include "../../external/vendor/glad.h"
#include "../../external/vendor/imGui/imgui.h"
#include "../../external/vendor/imGui/backends/imgui_impl_glfw.h"
#include "../../external/vendor/imGui/backends/imgui_impl_opengl3.h"
#include <glfw3.h>
#include <SpoutGL/SpoutSender.h>

export module render;
import weretype;
import shader;
import appState;
import artnet;

export namespace Render {

	constexpr char vertex_src[] = {
		#embed "../shaders/vertex.glsl"
	};
	constexpr char frag_src[] = {
		#embed "../shaders/frag.glsl"
	};
	constexpr char frag9_src[] = {
		#embed "../shaders/frag9.glsl"
	};

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
  bool g_imguiInitialized = false;
	GLFWwindow* g_uiWindow = nullptr;

	enum class RenderError {
		glfwFailed,
		windowCreationFailed,
		gladFailed,
	};

	GLuint dmxDataTexture;

	auto CreateGUI() -> std::expected<GLFWwindow*, std::string> {
		if (g_uiWindow) {
			return g_uiWindow;
		}

		glfwDefaultWindowHints();
		glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		GLFWwindow* uiWindow = glfwCreateWindow(1280, 720, "DMX Rasterizer UI", nullptr, nullptr);
		if (!uiWindow) return std::unexpected("glfwCreateWindow(UI) failed");

		glfwMakeContextCurrent(uiWindow);

		if (!gladLoadGLLoader(raw<GLADloadproc>(glfwGetProcAddress))) {
			glfwDestroyWindow(uiWindow);
			return std::unexpected("gladLoadGLLoader failed for UI");
		}

		if (!g_imguiInitialized) {
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGui::StyleColorsDark();
			
			ImGuiStyle& style = ImGui::GetStyle();
			style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
			
			ImGui_ImplGlfw_InitForOpenGL(uiWindow, true);
			ImGui_ImplOpenGL3_Init("#version 330 core");

			g_imguiInitialized = true;
		}


		g_uiWindow = uiWindow;
		return uiWindow;
	}

	auto InitGlad() -> std::expected<void, std::string> {
		if (!gladLoadGLLoader(raw<GLADloadproc>(glfwGetProcAddress))) { 
			return std::unexpected(std::string("Failed to create initialize Glad"));
		}
		return {};
	}
	
	auto BeginRenderer(std::span<u8> dmxspan) -> std::expected<void, RenderError> {

		// initialize glfw
		if (!glfwInit()) {
			return std::unexpected(RenderError::glfwFailed);
		}
		
		// glfw create hidden window(unless in debug), use opengl 3.3
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

		// initialize OpenGL via GLAD
		if (!gladLoadGLLoader(raw<GLADloadproc>(glfwGetProcAddress))) {
			std::cerr << "Failed to initialize GLAD" << std::endl;
			return std::unexpected(RenderError::gladFailed);
		}

		// set opengl viewport & create 1D texture
		glViewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);
		glGenTextures(1, &dmxDataTexture);
		glBindTexture(GL_TEXTURE_1D, dmxDataTexture);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, dmxspan.size(), 0, GL_RED, GL_FLOAT, dmxspan.data());
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		// load glsl source shader code
		std::string_view fragShader = app::nineMode ? frag9_src : frag_src;
		std::string_view vertexShader = vertex_src;
		Shader shader(vertexShader, fragShader);

		// setup opengl shader
		glUseProgram(shader.m_ID);
		glUniform2f(glGetUniformLocation(shader.m_ID, "resolution"), RENDER_WIDTH, RENDER_HEIGHT);

		// Create Vertex Attribute and Buffer Objects
		GLuint VAO, VBO;
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		// Space Cordinates (x,y) // (index, size, type, stride, void*)
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		// Texture Cordinates (u,v)
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		// Create opengl output texture
		GLuint texture, framebuffer;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RENDER_WIDTH, RENDER_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glGenFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
		
		glfwMakeContextCurrent(nullptr);

		// current operating renderloop operates on:
		// window - GLFWwindow* // dmxLogger& // shader& // VAO // dmxDataTexture // sender& // framebuffer // texture
		return {};
	}

	void renderLoop(
		GLFWwindow* window,
		ArtNet::UniverseLogger& logger,
		Shader& shader,
		GLuint VAO,
		GLuint dmxDataTexture,
		SpoutSender& sender,
		GLuint framebuffer,
		GLuint texture
	) noexcept {
		glfwMakeContextCurrent(window);
		
		while (app::running) {
			logger.waitForRender();
			for (auto&& [src, dst] : std::views::zip(logger.dmxData, logger.dmxDataNormalized)) {
				dst = as<f32>(src) / 255.0f;
			}
			
			// Update the DMX data into texture
			glBindTexture(GL_TEXTURE_1D, dmxDataTexture);
			glTexSubImage1D(GL_TEXTURE_1D, 0, 0, logger.Channels, GL_RED, GL_FLOAT, logger.dmxDataNormalized.data());

			// Rendering
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
			glViewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			
			shader.use();
			glBindVertexArray(VAO);
			
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_1D, dmxDataTexture);
			glUniform1i(glGetUniformLocation(shader.m_ID, "dmxDataTexture"), 0);
			
			glDrawArrays(GL_TRIANGLES, 0, 6);
			sender.SendTexture(texture, GL_TEXTURE_2D, RENDER_WIDTH, RENDER_HEIGHT);
			
			if (app::debugMode) {
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glViewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);
				glClear(GL_COLOR_BUFFER_BIT);
			
				shader.use();
				glBindVertexArray(VAO);
				
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_1D, dmxDataTexture);
				glUniform1i(glGetUniformLocation(shader.m_ID, "dmxDataTexture"), 0);
				
				glDrawArrays(GL_TRIANGLES, 0, 6);
				
				glfwSwapBuffers(window);  // Display the rendered image
			}
		}
		glfwMakeContextCurrent(nullptr);
		glDeleteTextures(1, &texture);
		glDeleteTextures(1, &dmxDataTexture);
		glDeleteFramebuffers(1, &framebuffer);
		glDeleteVertexArrays(1, &VAO);
		sender.ReleaseSender();
	}
}