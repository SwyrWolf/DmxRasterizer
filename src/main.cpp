#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include <unordered_map>
#include <thread>
#include <expected>
#include <optional>
#include <format>
#include <string>
#include <print>

#include "glad.h"
#include <glfw3.h>
#include <SpoutGL/SpoutSender.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

import weretype;
import fmtwrap;
import appState;
import netThread;
import shader;
import render;
import render.ui;
import net.artnet;
import net.winsock;

int main(int argc, char* argv[]) {

	enum ArgType {VERSION, PORT, DEBUG, BINDIP, CH9, UNKNOWN };
	std::unordered_map<std::string, ArgType> argMap = {
		{"-d", DEBUG},
		{"--debug", DEBUG}
	};
	
	if (argc > 1) {
		for (int i = 1; i < argc; ++i) {
			std::string arg = argv[i];
			
			auto theArg = argMap.find(arg);
			ArgType argType = (theArg != argMap.end()) ? theArg->second : UNKNOWN;
			
			switch (argType) {			
				case DEBUG:
				app::debugMode = true;
				break;
				
				case UNKNOWN:
				default:
				std::println("Unkown argument: {}", arg);
				break;
			}
		}
	}

	auto init = Render::InitGLFW(Render::DmxTexture)
		.and_then(Render::InitGLAD);
	if (!init) {
		std::println(stderr, "InitGLFW Failed: {}", init.error());
		return -1;
	}

	Render::SetupDmxDataTexture();
	std::array<std::unique_ptr<Shader>, 2> shader {
		Render::SetupShaderLoad(Render::vertex_src, Render::frag_src),
		Render::SetupShaderLoad(Render::vertex_src, Render::frag9_src),
	};

	Render::SetupVertexArrBuf();
	Render::SetupTextureAndBuffer();
	
	auto ipStr = app::ipString();
	auto Addr = winsock::CreateAddress(ipStr, as<u16>(app::ipPort)).and_then(winsock::OpenNetworkSocket);
	if (!Addr) {
		std::println(stderr, "Err: 0x{:x}", as<int>(Addr.error()));
		return -1;
	}
	app::NetConnection = std::move(*Addr);
	#ifdef _DEBUG
	std::println(stderr, "IP Address: 0x{:x}", app::NetConnection->ip);
	std::println(stderr, "IP Port: 0x{:x}", app::NetConnection->port);
	#endif
	app::Debug = std::format(L"Listening for Art-Net on [{}:{}]", std::wstring(ipStr.begin(), ipStr.end()), app::NetConnection->port);
	

	std::jthread artNetThread(::NetworkThread, std::ref(app::NetConnection.value()));

	glfwMakeContextCurrent(nullptr);
	std::thread renderThread(
		Render::renderLoop,
		std::ref(shader),
		Render::dmxDataTexture,
		Render::framebuffer,
		Render::texture
	);


	// Creation of the Gui Window required on main thread to prevent nullptr race condition
	if (!SetupWindow()) {
		std::println("SetupWindow Failed");
		return -1;
	}
	
	std::thread guiThread(ImGuiLoop, std::ref(Render::DmxTexture.Channels));
	
	//Main thread loop
	while (!glfwWindowShouldClose(app::GuiWindow)) {
		glfwWaitEvents();  // Blocks until an event occurs (CPU efficient)
	}
	
	app::running = false;
	artNetThread.request_stop();
	app::times.signalRender();
	renderThread.join();
	guiThread.join();
	
	glDeleteBuffers(1, &Render::VBO);
	glfwTerminate();
	if (auto r = winsock::CloseNetworkSocket(app::NetConnection.value()); !r) {
		std::println("Winsock CloseNetworkSocket Failed: 0x{:x}", as<int>(r.error()));
	}
	app::NetConnection.reset();
	std::exit(0);
}
