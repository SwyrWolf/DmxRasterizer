module;

#include <ranges>
#include <print>
#include <thread>

#include "glad.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glfw3.h>

#include "wereMacro.hpp"
#include "buildstamp.hpp"

export module render.ui;
import appState;
import weretype;
import net.artnet;
import net.winsock;
import fmtwrap;
import netThread;
import net.relay;
import console;

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

constexpr const char* Title = "DMX Rasterizer | " APP_BUILD_VERSION;

export bool SetupWindow() {
	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
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
	
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);

	ImGui_ImplGlfw_InitForOpenGL(app::GuiWindow, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");
	
	bool showRelaySettings = false;

	while(app::running) {
		
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Build UI
		// UI Panel 1
		ImGui::SetNextWindowPos(ImVec2(Panels[0].x, Panels[0].y), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(Panels[0].w, Panels[0].h), ImGuiCond_Always);
		ImGui::Begin("DmxMainLog", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
		ImGui::Text("DMX Rasterizer");
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
			std::println("fps selected");
		}
		ImGui::EndDisabled();
		
		ImGui::SameLine(); 
		if (ImGui::Checkbox("Vsync", &app::VsyncEnabled)) {
			glfwSwapInterval(app::VsyncEnabled ? 1 : 0);
		};
		ImGui::Text("");
		ImGui::Text("Network");
		ImGui::SetNextItemWidth(155.0f);
		ImGui::InputText("##IPv4", app::ipStr.data(), app::ipStr.size());
		ImGui::SetNextItemWidth(50.0f);
		ImGui::SameLine();
		ImGui::InputInt("##Port", &app::ipPort, 0, 0, ImGuiInputTextFlags_None);
		
		ImGui::BeginDisabled(app::NetConnection.has_value());
		if (ImGui::Button("Connect")) {
			if (winsock::winsockInit()) {
				auto Addr = winsock::CreateAddress(app::ipString(), as<u16>(app::ipPort)).and_then(winsock::OpenNetworkSocket);
				if (!Addr) {
					std::println(stderr, "Err: 0x{:x}", as<int>(Addr.error()));
					break;
				}
				app::NetConnection.emplace(std::move(*Addr));
				::ContinueNetThread();
			}
		}
		ImGui::EndDisabled();
		
		ImGui::BeginDisabled(!app::NetConnection.has_value());
		ImGui::SameLine();
		if (ImGui::Button("Disconnect")) {
			if (auto r = winsock::CloseNetworkSocket(app::NetConnection.value()); !r) {
				std::println(stderr, "Error closing network socket: 0x{:x}", as<int>(r.error()));
			}
			app::NetConnection.reset();
		}
		ImGui::EndDisabled();
		if (ImGui::Checkbox("Relay", &app::RelaySend)) {
			glfwSwapInterval(app::RelaySend ? 1 : 0);
		};
		ImGui::SameLine();
		if (ImGui::Button("Manage Relay")) showRelaySettings = true;
		ImGui::End();

		// UI Panel 3 -- Right
		ImGui::SetNextWindowPos(ImVec2(Panels[2].x, Panels[2].y), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(Panels[2].w, Panels[2].h), ImGuiCond_Always);
		ImGui::Begin("DmxLogs", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

		std::string uniStatus;
		uniStatus.reserve(256);

		if (ImGui::Checkbox(
			fmt::cat("9 Universe Mode: ", app::RGBmode ? "Enabled" : "Disabled").c_str(),
			&app::RGBmode
		)) {
			Channels = app::RGBmode ? 4680 : 1560;
		}
		if (ImGui::Checkbox(
			fmt::cat("View DMX Texture: ", app::ViewTexture ? "Enabled" : "Disabled").c_str(),
			&app::ViewTexture
		)) { 
			app::ViewTexture ? glfwShowWindow(app::SpoutWindow) : glfwHideWindow(app::SpoutWindow);
		}
		if (ImGui::Checkbox(
			fmt::cat("Show Console: ", app::ViewConsole ? "Enabled" : "Disabled").c_str(),
			&app::ViewConsole
		)) { 
			console::ConsoleToggle();
		}

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

		if (showRelaySettings) {
			ImGui::Begin("Relay Settings", &showRelaySettings, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
			if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
				showRelaySettings = false;
			ImGui::BeginTable("##relay_table", 2);
			ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("##input", ImGuiTableColumnFlags_WidthFixed, 200.0f);
			ImGui::TableNextRow(); ImGui::TableNextColumn();
			ImGui::Text("Address"); ImGui::TableNextColumn(); ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##relay_address", app::relayAddress.data(), app::relayAddress.size());
			ImGui::TableNextRow(); ImGui::TableNextColumn();
			ImGui::Text("Name");    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##relay_name", app::displayName.data(), app::displayName.size());
			ImGui::TableNextRow(); ImGui::TableNextColumn();
			ImGui::Text("Access");  ImGui::TableNextColumn(); ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##relay_access", app::relayAccess.data(), app::relayAccess.size());
			ImGui::EndTable();
			ImGui::TextUnformatted(app::relayStatus.c_str());
			if (app::RelayTCP) {
				if (ImGui::Button("Disconnect"))
					std::thread(relay::Disconnect).detach();
			} else {
				if (ImGui::Button("Connect")) {
					app::RelaySend = true;
					glfwSwapInterval(1);
					std::thread(relay::Connect).detach();
				}
			}
			ImGui::SameLine();
			{
				constexpr const char* modes[] = { "Send", "Listen" };
				int modeIdx = as<int>(app::relayMode);
				ImGui::SetNextItemWidth(80.0f);
				if (ImGui::Combo("##relay_mode", &modeIdx, modes, 2))
					app::relayMode = as<app::RelayMode>(modeIdx);
			}
			ImGui::End();
		}

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