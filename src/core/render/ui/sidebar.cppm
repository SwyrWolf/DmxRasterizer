module;

#include <print>
#include <ranges>
#include <algorithm>
#include <thread>

#include "glad.h"
#include "imgui.h"
#include <glfw3.h>

export module render.ui:sidebar;
import weretype;
import fmtwrap;

import appState;
import net.artnet;
import net.winsock;
import net.relay;
import net.control;
import settings;
import console;
import :util;

static bool showSettings_Graphics{false};
static bool showSettings_ArtNet{false};
static bool showSettings_Relay{false};
static bool showSettings_Logs{false};
static bool showSettings_Lights{false};

void showGraphicsMenu() {
		if (showSettings_Graphics) {
		ImGui::SetNextWindowSize(ImVec2(200.0f, 100.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Graphics Settings", &showSettings_Graphics);

		// ImGui::Text("FPS Limit");
		// ImGui::SetNextItemWidth(150.0f);
		// ImGui::BeginDisabled(app::VsyncEnabled);
		// if (ImGui::Combo("##FPS Limit", &app::FrameRateSel, app::FPS_LABELS.data(), as<int>(app::FPS_ITEMS.size()))) {
		// 	std::println("fps selected");
		// }
		// ImGui::EndDisabled();
		
		// ImGui::SameLine();
		if (ImGui::Checkbox("Vsync", &app::VsyncEnabled)) {
			glfwSwapInterval(app::VsyncEnabled ? 1 : 0);
		}

		ImGui::End();
	}
}

void showArtNetMenu() {
		if (showSettings_ArtNet) {
		ImGui::SetNextWindowSize(ImVec2(300.0f, 250.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Art-Net Settings", &showSettings_ArtNet);

		ImGui::Text("Network");
		ImGui::SetNextItemWidth(155.0f);
		ImGui::InputText("##IPv4", app::ipStr.data(), app::ipStr.size());
		ImGui::SetNextItemWidth(50.0f);
		ImGui::SameLine();
		ImGui::InputInt("##Port", &app::ipPort, 0, 0, ImGuiInputTextFlags_None);
		
		ImGui::BeginDisabled(app::NetConnection.has_value());
		if (ImGui::Button("Connect")) {
			::StartNetwork();
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
		ImGui::End();
	}
}

void showRelayMenu() {
		if (showSettings_Relay) {
		ImGui::SetNextWindowSize(ImVec2(250.0f, 250.0f), ImGuiCond_FirstUseEver);

		ImGui::Begin("Relay Settings", &showSettings_Relay);
		if (ImGui::BeginTable("##relay_table", 2)) {
				ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("##input", ImGuiTableColumnFlags_WidthFixed, 200.0f);
				ImGui::TableNextRow(); ImGui::TableNextColumn();
				ImGui::Text("Address"); ImGui::TableNextColumn(); ImGui::SetNextItemWidth(-1);
				ImGui::InputText("##relay_address", relay::address.data(), relay::address.size());
				ImGui::TableNextRow(); ImGui::TableNextColumn();
				ImGui::Text("Name");    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(-1);
				ImGui::InputText("##relay_name", relay::displayName.data(), relay::displayName.size());
				ImGui::TableNextRow(); ImGui::TableNextColumn();
				ImGui::Text("Access");  ImGui::TableNextColumn(); ImGui::SetNextItemWidth(-1);
				ImGui::InputText("##relay_access", relay::access.data(), relay::access.size());
				ImGui::EndTable();
			}
		ImGui::TextUnformatted(relay::status.c_str());
		if (relay::tcpEndpoint) {
			if (ImGui::Button("Disconnect"))
				std::thread(relay::Disconnect).detach();
		} else {
			if (ImGui::Button("Connect")) {
				relay::isSending  = true;
				glfwSwapInterval(1);
				settings::Save();
				std::thread(relay::Connect).detach();
			}
		}
		ImGui::SameLine();
		{
			constexpr const char* modes[] = { "Send", "Listen" };
			int modeIdx = as<int>(relay::mode);
			ImGui::SetNextItemWidth(80.0f);
			if (ImGui::Combo("##relay_mode", &modeIdx, modes, 2))
				relay::mode = as<relay::Mode>(modeIdx);
		}
		
		ImGui::End();
	}
}

void showLogMenu() {
		if (showSettings_Logs) {
		ImGui::SetNextWindowSize(ImVec2(500.0f, 100.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Logs menu", &showSettings_Logs);

		ImGui::Text("%ls",app::Debug.c_str());

		ImGui::End();
	}
}

void showLightMenu(int& Channels) {
		if (showSettings_Lights) {
		ImGui::SetNextWindowSize(ImVec2(250.0f, 400.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Lights data", &showSettings_Lights);

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
	}
}

export void sidebar(int& Channels) {

	showGraphicsMenu();
	showArtNetMenu();
	showRelayMenu();
	showLogMenu();
	showLightMenu(Channels);

	ImGuiViewport* vp = ImGui::GetMainViewport();

	float pad = 10.0f;
	float sideWidth = 60.0f;
	float btnHeight = 40.0f;

	ImGui::SetNextWindowPos(ImVec2(
		vp->Pos.x + pad,
		vp->Pos.y + pad
	));

ImGui::SetNextWindowSizeConstraints(ImVec2(sideWidth, 0.0f), ImVec2(sideWidth, FLT_MAX));

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, pad);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, HexColor(0x282a36));

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar 
		| ImGuiWindowFlags_NoMove 
		|	ImGuiWindowFlags_NoResize 
		|	ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_AlwaysAutoResize;

	ImGui::Begin("Sidebar", nullptr, flags);

	if (ImGui::Button(fa_icon::display, ImVec2(-FLT_MIN, btnHeight))) {
		showSettings_Graphics = !showSettings_Graphics;
	}
	if (ImGui::Button(fa_icon::network, ImVec2(-FLT_MIN, btnHeight))) {
		showSettings_ArtNet = !showSettings_ArtNet;
	}
	if (ImGui::Button(fa_icon::server, ImVec2(-FLT_MIN, btnHeight))) {
		showSettings_Relay = !showSettings_Relay;
	}
	if (ImGui::Button(fa_icon::file_lines, ImVec2(-FLT_MIN, btnHeight))) {
		showSettings_Logs = !showSettings_Logs;
	}
	if (ImGui::Button(fa_icon::lightbulb, ImVec2(-FLT_MIN, btnHeight))) {
		showSettings_Lights = !showSettings_Lights;
	}

	ImGui::End();

	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
}