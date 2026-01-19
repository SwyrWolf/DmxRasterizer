module;

#include <iostream>
#include <ranges>

#include "../../external/vendor/glad.h"
#include "../../external/vendor/imGui/imgui.h"
#include "../../external/vendor/imGui/backends/imgui_impl_glfw.h"
#include "../../external/vendor/imGui/backends/imgui_impl_opengl3.h"
#include <glfw3.h>

export module render.ui;
import appState;
import weretype;
import artnet;
import fmtwrap;

int fbw{}, fbh{};

export bool SetupWindow() {
	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	if (!app::GuiWindow) {
		GLFWwindow* uiWindow = glfwCreateWindow(1280, 720, "DMX Rasterizer UI", nullptr, nullptr);
		if (!uiWindow) {
			std::cerr << "SetupWindow Failed!\n";
			return false;
		}
		app::GuiWindow = uiWindow;
	}
	return true;
}

export void ImGuiLoop() noexcept {
	glfwMakeContextCurrent(app::GuiWindow);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGuiIO& io = ImGui::GetIO();

	ImFontConfig font_cfg;
	font_cfg.SizePixels = 15.0f;
	io.FontDefault = io.Fonts->AddFontDefault(&font_cfg);
	
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);

	ImGui_ImplGlfw_InitForOpenGL(app::GuiWindow, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");
	
	while(app::running) {
		
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Build UI
		// UI Panel 1
		ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(400, 100), ImGuiCond_Always);
		ImGui::Begin("DmxMainLog", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
		ImGui::Text("DMX Rasterizer | v1.0.0");
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

		std::string keys_pressed;
		for (ImGuiKey key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key = (ImGuiKey)(key + 1)) {
			if (ImGui::IsKeyDown(key)) {
				if (const char* name = ImGui::GetKeyName(key)) {
					keys_pressed += name;
					keys_pressed += " ";
				}
			}
		}
		ImGui::TextWrapped("Keys: %s", keys_pressed.c_str());
		ImGui::End();

		// UI Panel 2 -- Settings
		ImGui::SetNextWindowPos(ImVec2(10, 120), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(400, 145), ImGuiCond_Always);
		ImGui::Begin("DmxControls", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
		
		ImGui::Text("FPS Limit");
		ImGui::SetNextItemWidth(150.0f);
		ImGui::BeginDisabled(app::VsyncEnabled);
		if (ImGui::Combo("##FPS Limit", &app::FrameRateSel, app::FPS_LABELS.data(), as<int>(app::FPS_ITEMS.size()))) {
			std::wcerr << L"\nfps selected";
		}
		ImGui::EndDisabled();
		
		ImGui::SameLine(); ImGui::Checkbox("Vsync", &app::VsyncEnabled);
		ImGui::Text("");
		ImGui::Text("Network");
		ImGui::SetNextItemWidth(162.0f);
		ImGui::InputText("##IPv4", app::ipStr.data(), app::ipStr.size());
		ImGui::SetNextItemWidth(50.0f);
		ImGui::SameLine();
		ImGui::InputInt("##Port", &app::ipPort, 0, 0, ImGuiInputTextFlags_None);
		
		ImGui::BeginDisabled(true);
		if (ImGui::Button("Connect")) {
			// if (net::Operational()) {
			// 	net::OpenNetworkSocket(0, state::ipPort);
			// }
		}
		ImGui::EndDisabled();
		
		ImGui::BeginDisabled(false);
		ImGui::SameLine();
		if (ImGui::Button("Disconnect")) {
			// net::CloseNetworkSocket();
		}
		ImGui::EndDisabled();
		ImGui::End();

		// UI Panel 3 -- Right
		ImGui::SetNextWindowPos(ImVec2(420, 10), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(300, 255), ImGuiCond_Always);
		ImGui::Begin("DmxLogs", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

		std::string uniStatus;
		uniStatus.reserve(256);

		ImGui::Checkbox(
			fmt::cat("9 Universe Mode: ", app::RGBmode ? "Enabled" : "Disabled").c_str(),
			&app::RGBmode
		);

		for (int i : std::views::iota(0, 9)) {
			uniStatus += fmt::cat("UNI-0", i, ": ",app::times.GetTimeDeltaMsAt(i), "ms\n");
		}
		ImGui::Text("%s",uniStatus.c_str());
		ImGui::End();

		// UI Panel 4 -- Bottom
		ImGui::SetNextWindowPos(ImVec2(10, 275), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(710, 230), ImGuiCond_Always);
		ImGui::Begin("Reporting", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

		ImGui::Text("%ls",app::Debug.c_str());
		ImGui::End();

		ImGui::Render(); // Render UI

		glfwGetFramebufferSize(app::GuiWindow, &fbw, &fbh);
		glViewport(0, 0, fbw, fbh);
		if (fbw <= 0 || fbh <= 0) continue;

		glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(app::GuiWindow);
	}
	
	glfwMakeContextCurrent(nullptr);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}