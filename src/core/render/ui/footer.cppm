module;

#include <string>
#include "imgui.h"

export module render.ui:footer;
import weretype;
import appState;
import net.relay;
import :util;

const f32 barHeight = 40.0f;
const f32 pad = 5.0f;

export void footer() {
	ImGuiViewport* vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(
		vp->Pos.x + pad,
		vp->Pos.y + vp->Size.y - barHeight - pad
	));
	ImGui::SetNextWindowSize(ImVec2(
		vp->Size.x - (pad * 2),
		barHeight
	));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, HexColor(0x1b2432));
	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoCollapse;
	
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, pad);
	
	ImGui::Begin("Bottom Bar", nullptr, flags);
	
	ImGui::PushStyleColor(ImGuiCol_Text,
		app::NetConnection.has_value() ? HexColor(0x00FF00) : HexColor(0xFFFFFF)
	);
	ImGui::TextUnformatted(fa_icon::wifi);
	ImGui::PopStyleColor();
	ImGui::SameLine();
	
	ImGui::PushStyleColor(ImGuiCol_Text,
		relay::isSending 
		? relay::mode == relay::Mode::Listen
		? HexColor(0xe39343)
		: HexColor(0x00FF00)
		: HexColor(0xFFFFFF)
	);
	ImGui::TextUnformatted(fa_icon::satellite);
	ImGui::PopStyleColor();
	ImGui::SameLine();
	
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	// ImGui::SameLine();
	// std::string keys_pressed;
	// for (ImGuiKey key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key = (ImGuiKey)(key + 1)) {
	// 	if (ImGui::IsKeyDown(key)) {
	// 		if (const char* name = ImGui::GetKeyName(key)) {
	// 			keys_pressed += name;
	// 			keys_pressed += " ";
	// 		}
	// 	}
	// }
	// ImGui::TextWrapped("Keys: %s", keys_pressed.c_str());
	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}