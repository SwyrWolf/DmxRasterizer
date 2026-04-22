module;

#include <ranges>
#include <algorithm>
#include <print>

#include "glad.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glfw3.h>

#include "wereMacro.hpp"
#include "buildstamp.hpp"

export module render.ui;
import weretype;
import appState;
import net.artnet;
import net.winsock;
import netThread;
import settings;

import :util;
import :footer;
import :sidebar;

// Embed raw RGBA8 bytes (W*H*4 bytes)
constexpr u8 Icon32[] = {
	#embed EMBED(resource/dmxr32.rgba)
};
constexpr u8 Icon16[] = {
	#embed EMBED(resource/dmxr16.rgba)
};
constexpr GLFWimage imgs[2]{
	{ 16, 16, const_cast<u8*>(Icon16)},
	{ 32, 32, const_cast<u8*>(Icon32)},
};

constexpr u8 FontAwesomeTTF[] = {
	#embed EMBED(resource/fa-solid-900.ttf)
};

int fbw{}, fbh{};
struct PanelBox {
	f32 x{}, y{}, w{}, h{};
};
struct vec2 {
	f32 x{}, y{};
};

constexpr const char* Title = "DMX Rasterizer | " APP_BUILD_VERSION;

export bool SetupWindow() {
	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
	glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	if (!app::GuiWindow) {
		GLFWwindow* uiWindow = glfwCreateWindow(730, 565, Title, nullptr, nullptr);
		if (!uiWindow) {
			std::println(stderr, "Setup Window Failed!");
			return false;
		}

		app::GuiWindow = uiWindow;
		glfwMakeContextCurrent(app::GuiWindow);
		
		glfwSetWindowIcon(app::GuiWindow, 2, imgs);
	}
	glfwMakeContextCurrent(nullptr);
	return true;
}

export void ImGuiLoop(int& Channels) {
	glfwMakeContextCurrent(app::GuiWindow);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;

	ImFontConfig font_cfg;
	font_cfg.SizePixels = 20.0f;
	font_cfg.OversampleH = 3;
	font_cfg.OversampleV = 3;
	font_cfg.PixelSnapH = false;
	const ImWchar font_ranges[] = { 0x0020, 0x00FF, 0x2600, 0x26FF, 0 };
	io.FontDefault = io.Fonts->AddFontFromFileTTF(
		"C:/Windows/Fonts/segoeui.ttf",
		font_cfg.SizePixels,
		&font_cfg,
		font_ranges
	);
	
	ImFontConfig fa_cfg;
	fa_cfg.MergeMode = true; // merge glyphs into the previous font
	fa_cfg.FontDataOwnedByAtlas = false; // data is constexpr, don't free it
	fa_cfg.GlyphMinAdvanceX = font_cfg.SizePixels; // monospaced icons
	static const ImWchar fa_ranges[] = { 0xF000, 0xF999, 0 };
	io.Fonts->AddFontFromMemoryTTF(
		const_cast<u8*>(FontAwesomeTTF), sizeof(FontAwesomeTTF),
		font_cfg.SizePixels, &fa_cfg, fa_ranges
	);
	
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);

	ImGui_ImplGlfw_InitForOpenGL(app::GuiWindow, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");
	
	bool showRelaySettings = false;

	while(app::running) {
		
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		::sidebar(Channels);
		::footer();

		ImGui::Render(); // Render UI

		glfwGetFramebufferSize(app::GuiWindow, &fbw, &fbh);
		glViewport(0, 0, fbw, fbh);
		if (fbw <= 0 || fbh <= 0) continue;

		glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(app::GuiWindow);
	}

	std::println("ImGui Ended!");
	
	glfwMakeContextCurrent(nullptr);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}