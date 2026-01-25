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
import net.artnet;
import fmtwrap;

int fbw{}, fbh{};

struct PanelBox {
	f32 x{}, y{}, w{}, h{};
};
struct vec2 {
	f32 x{}, y{};
};

namespace grid { // layout
	constexpr f32 pd{10}; // padding
	constexpr f32 pd2 = pd * 2; // padding
	constexpr std::array col{ f32{400}, f32{300} };
	constexpr std::array row{ f32{100}, f32{200}, f32{225} };
}


constexpr std::array Panels{
	PanelBox{
		grid::pd, 
		grid::pd, 
		grid::col[0],
		grid::row[0]
	}, // Panel 0 -- Top left
	PanelBox{
		grid::pd,
		grid::pd2 + grid::row[0],
		grid::col[0],
		grid::row[1]
	}, // Panel 1 -- Mid left
	PanelBox{
		grid::col[0] + grid::pd2,
		grid::pd,
		grid::col[1],
		grid::row[0] + grid::pd + grid::row[1]
	}, // Panel 2 -- span right
	PanelBox{
		grid::pd,
		grid::row[0] + (grid::pd * 3) + grid::row[1],
		grid::col[0] + grid::pd + grid::col[1],
		grid::row[2]
	} // Panel 3 -- span bottom
};

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
	font_cfg.SizePixels = 20.0f;
	font_cfg.OversampleH = 3;
	font_cfg.OversampleV = 3;
	font_cfg.PixelSnapH = false;
	io.FontDefault = io.Fonts->AddFontFromFileTTF(
		"C:/Windows/Fonts/segoeui.ttf",
		font_cfg.SizePixels,
		&font_cfg
	);
	
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
		ImGui::SetNextWindowPos(ImVec2(Panels[0].x, Panels[0].y), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(Panels[0].w, Panels[0].h), ImGuiCond_Always);
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
		ImGui::SetNextWindowPos(ImVec2(Panels[1].x, Panels[1].y), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(Panels[1].w, Panels[1].h), ImGuiCond_Always);
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
		ImGui::SetNextItemWidth(155.0f);
		ImGui::InputText("##IPv4", app::ipStr.data(), app::ipStr.size());
		ImGui::SetNextItemWidth(50.0f);
		ImGui::SameLine();
		ImGui::InputInt("##Port", &app::ipPort, 0, 0, ImGuiInputTextFlags_None);
		
		ImGui::BeginDisabled(app::OpenConnection);
		if (ImGui::Button("Connect")) {
			// if (net::Operational()) {
			// 	net::OpenNetworkSocket(0, state::ipPort);
			// }
		}
		ImGui::EndDisabled();
		
		ImGui::BeginDisabled(!app::OpenConnection);
		ImGui::SameLine();
		if (ImGui::Button("Disconnect")) {
			// net::CloseNetworkSocket();
		}
		ImGui::EndDisabled();
		ImGui::End();

		// UI Panel 3 -- Right
		ImGui::SetNextWindowPos(ImVec2(Panels[2].x, Panels[2].y), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(Panels[2].w, Panels[2].h), ImGuiCond_Always);
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
		ImGui::SetNextWindowPos(ImVec2(Panels[3].x, Panels[3].y), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(Panels[3].w, Panels[3].h), ImGuiCond_Always);
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