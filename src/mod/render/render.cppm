module;

#include <expected>
#include <vector>
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

	struct DmxShaderData {
		int Width{1920};
		int Height{208};
		int Channels{1560};
		int Universes{3};
		std::vector<u8> DmxData = std::vector<u8>(1560);
		std::vector<f32> ChannelsNormalized{1560};
	};
	DmxShaderData DmxTexture{};

	bool g_imguiInitialized{false};
	GLuint dmxDataTexture{}, VAO{}, VBO{}, texture{}, framebuffer{};

	// Initialize GLFW window first
	[[nodiscard]] std::expected<void, std::string>
	InitGLFW(DmxShaderData& ds) {

		if (!glfwInit()) {
			return std::unexpected("Failed to initialize GLFW");
		}
		
		glfwDefaultWindowHints();
		if(!app::debugMode){
			glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		} else {
			glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
		}
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		app::SpoutWindow = glfwCreateWindow(ds.Width, ds.Height, "DMX Shader Renderer", nullptr, nullptr);
		if (!app::SpoutWindow) {
			return std::unexpected("Failed to create GLFW window");
		}
		glfwMakeContextCurrent(app::SpoutWindow);
		return {};
	}

	// After GLFW initialized, Initialize the GLAD GL loader
	[[nodiscard]] std::expected<void, std::string>
	InitGLAD() {
		if (!gladLoadGLLoader(raw<GLADloadproc>(glfwGetProcAddress))) { 
			return std::unexpected("Failed to initialize Glad");
		}
		return {};
	}

	void SetupDmxDataTexture() {
		glGenTextures(1, &dmxDataTexture);
		glBindTexture(GL_TEXTURE_1D, dmxDataTexture);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, DmxTexture.Channels, 0, GL_RED, GL_FLOAT, DmxTexture.ChannelsNormalized.data());
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	[[nodiscard]] Shader
	SetupShaderLoad() {
		std::string_view fragShader = app::RGBmode ? Render::frag9_src : Render::frag_src;
		std::string_view vertexShader = Render::vertex_src;
		Shader shader(vertexShader, fragShader);
		glUseProgram(shader.m_ID);
		glUniform2f(glGetUniformLocation(shader.m_ID, "resolution"), Render::DmxTexture.Width, Render::DmxTexture.Height);

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

	[[nodiscard]] std::expected<void, std::string>
	CreateGUI() {
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

		app::GuiWindow = uiWindow;
		return {};
	}

	void renderLoop(
		Shader& shader,
		GLuint dmxDataTexture,
		GLuint framebuffer,
		GLuint texture
	) noexcept {
		if (!app::SpoutWindow) {
			std::cerr << "Render: No SpoutWindow";
			return;
		}
		glfwMakeContextCurrent(app::SpoutWindow);

		SpoutSender sender;
		if (!sender.CreateSender("DmxRasterizer",Render::DmxTexture.Width, Render::DmxTexture.Height)) {
			std::cerr << "Failed to create Spout sender!";
			return;
		}

		while (app::running) {
			// logger.waitForRender();
			app::times.waitForRender();
			for (auto&& [src, dst] : std::views::zip(DmxTexture.DmxData, DmxTexture.ChannelsNormalized)) {
				dst = as<f32>(src) / 255.0f;
			}
			
			// Update the DMX data into texture
			glBindTexture(GL_TEXTURE_1D, dmxDataTexture);
			glTexSubImage1D(GL_TEXTURE_1D, 0, 0, DmxTexture.Channels, GL_RED, GL_FLOAT, DmxTexture.ChannelsNormalized.data());

			// Rendering
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
			glViewport(0, 0, DmxTexture.Width, DmxTexture.Height);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			
			shader.use();
			glBindVertexArray(Render::VAO);
			
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_1D, dmxDataTexture);
			glUniform1i(glGetUniformLocation(shader.m_ID, "dmxDataTexture"), 0);
			
			glDrawArrays(GL_TRIANGLES, 0, 6);
			sender.SendTexture(texture, GL_TEXTURE_2D, DmxTexture.Width, DmxTexture.Height);
			
			if (app::debugMode) {
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