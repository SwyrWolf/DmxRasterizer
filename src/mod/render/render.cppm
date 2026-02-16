module;

#include <expected>
#include <vector>
#include <string>

#include "glad.h"
#include <glfw3.h>
#include <SpoutGL/SpoutSender.h>
#include "wereMacro.hpp"

export module render;
import weretype;
import shader;
import appState;

export namespace Render {

	constexpr char vertex_src_data[] = {
		#embed EMBED(shader/vertex.glsl)
	};
	constexpr std::string_view vertex_src{vertex_src_data, std::size(vertex_src_data)};
	
	constexpr char frag_src_data[] = {
		#embed EMBED(shader/frag.glsl)
	};
	constexpr std::string_view frag_src{frag_src_data, std::size(frag_src_data)};
	
	constexpr char frag9_src_data[] = {
		#embed EMBED(shader/frag9.glsl)
	};
	constexpr std::string_view frag9_src{frag9_src_data, std::size(frag9_src_data)};

	f32 vertices[] = {
		-1.0f,  1.0f,  0.0f, 1.0f, // Top-left
		-1.0f, -1.0f,  0.0f, 0.0f, // Bottom-left
		1.0f, -1.0f,  1.0f, 0.0f, // Bottom-right
		
		-1.0f,  1.0f,  0.0f, 1.0f, // Top-left
		1.0f, -1.0f,  1.0f, 0.0f, // Bottom-right
		1.0f,  1.0f,  1.0f, 1.0f  // Top-right
	};

	struct DmxShaderData {
		int Width{1920};
		int Height{208};
		int Channels{4680}; // 9 universe 4680 / 3 1560
		std::vector<u8> DmxData = std::vector<u8>(4680);
	};
	DmxShaderData DmxTexture{};

	bool g_imguiInitialized{false};
	GLuint dmxDataTexture{}, VAO{}, VBO{}, texture{}, framebuffer{};

	// Initialize GLFW window first
	auto InitGLFW(DmxShaderData& ds) -> std::expected<void, std::string> {

		if (!glfwInit()) {
			return std::unexpected("Failed to initialize GLFW");
		}
		
		glfwDefaultWindowHints();
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		app::SpoutWindow = glfwCreateWindow(ds.Width, ds.Height, "DMX Shader Renderer", nullptr, nullptr);
		if (!app::SpoutWindow) {
			return std::unexpected("Failed to create GLFW window");
		}
		glfwMakeContextCurrent(app::SpoutWindow);
		glfwSetWindowCloseCallback(app::SpoutWindow,
    [](GLFWwindow* window) {
			app::ViewTexture = false;
			glfwHideWindow(window);
    });
		return {};
	}

	// After GLFW initialized, Initialize the GLAD GL loader
	auto InitGLAD() -> std::expected<void, std::string> {
		if (!gladLoadGLLoader(raw<GLADloadproc>(glfwGetProcAddress))) { 
			return std::unexpected("Failed to initialize Glad");
		}
		return {};
	}

	void SetupDmxDataTexture() {
		glGenTextures(1, &dmxDataTexture);
		glBindTexture(GL_TEXTURE_1D, dmxDataTexture);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, DmxTexture.Channels, 0, GL_RED, GL_UNSIGNED_BYTE, DmxTexture.DmxData.data());
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	[[nodiscard]] auto SetupShaderLoad(std::string_view vert, std::string_view frag) -> std::unique_ptr<Shader> {
		auto shader = std::make_unique<Shader>(vert, frag);
		glUseProgram(shader->m_ID);
		glUniform2f(glGetUniformLocation(shader->m_ID, "resolution"), Render::DmxTexture.Width, Render::DmxTexture.Height);

		return shader;
	}

	void SetupVertexArrBuf() {
		glGenVertexArrays(1, &Render::VAO);
		glGenBuffers(1, &Render::VBO);
		
		glBindVertexArray(Render::VAO);
		glBindBuffer(GL_ARRAY_BUFFER, Render::VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Render::vertices), Render::vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);
	}

	void SetupTextureAndBuffer() {
		glGenTextures(1, &Render::texture);
		glBindTexture(GL_TEXTURE_2D, Render::texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Render::DmxTexture.Width, Render::DmxTexture.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glGenFramebuffers(1, &Render::framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, Render::framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Render::texture, 0);
	}

	void renderLoop(
		std::array<std::unique_ptr<Shader>,2>& shader,
		GLuint dmxDataTexture,
		GLuint framebuffer,
		GLuint texture
	) {
		if (!app::SpoutWindow) {
			#ifdef _DEBUG
			std::cerr << "Render: No SpoutWindow";
			#endif
			return;
		}
		glfwMakeContextCurrent(app::SpoutWindow);

		SpoutSender sender;
		if (!sender.CreateSender("DmxRasterizer",Render::DmxTexture.Width, Render::DmxTexture.Height)) {
			#ifdef _DEBUG
			std::cerr << "Failed to create Spout sender!";
			#endif
			return;
		}

		while (app::running) {
			app::times.waitForRender();
			
			// Update the DMX data into texture (OpenGL auto-normalizes u8 to [0.0, 1.0])
			glBindTexture(GL_TEXTURE_1D, dmxDataTexture);
			glTexSubImage1D(GL_TEXTURE_1D, 0, 0, DmxTexture.Channels, GL_RED, GL_UNSIGNED_BYTE, DmxTexture.DmxData.data());

			// Rendering
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
			glViewport(0, 0, DmxTexture.Width, DmxTexture.Height);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			
			u64 s = app::RGBmode ? 1 : 0;
			shader[s]->use();
			glBindVertexArray(Render::VAO);
			
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_1D, dmxDataTexture);
			glUniform1i(glGetUniformLocation(shader[s]->m_ID, "dmxDataTexture"), 0);
			
			glDrawArrays(GL_TRIANGLES, 0, 6);
			sender.SendTexture(texture, GL_TEXTURE_2D, DmxTexture.Width, DmxTexture.Height);
			
			if (app::ViewTexture) {
				glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

				glBlitFramebuffer(
					0, 0, DmxTexture.Width, DmxTexture.Height,
					0, 0, DmxTexture.Width, DmxTexture.Height,
					GL_COLOR_BUFFER_BIT, GL_NEAREST
				);
				
				glfwSwapBuffers(app::SpoutWindow);  // Display the rendered image
			}
		}
		glDeleteTextures(1, &texture);
		glDeleteTextures(1, &dmxDataTexture);
		glDeleteFramebuffers(1, &framebuffer);
		glDeleteVertexArrays(1, &Render::VAO);
		glfwMakeContextCurrent(nullptr);
		sender.ReleaseSender();
	}
}