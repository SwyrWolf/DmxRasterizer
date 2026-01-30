#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include <iostream>
#include <unordered_map>
#include <thread>
#include <expected>
#include <optional>
#include <format>
#include <string>

#include "./external/vendor/glad.h"
#include <glfw3.h>
#include <SpoutGL/SpoutSender.h>
#include "../../external/vendor/imGui/imgui.h"
#include "../../external/vendor/imGui/backends/imgui_impl_glfw.h"
#include "../../external/vendor/imGui/backends/imgui_impl_opengl3.h"

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
		{"--debug", DEBUG},
		{"--bind", BINDIP}
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
				std::cerr << "Unkown argument: " << arg << std::endl;
				break;
			}
		}
	}

	auto init = Render::InitGLFW(Render::DmxTexture)
		.and_then(Render::InitGLAD);
	if (!init) {
		std::cerr << init.error() << "\n";
		return -1;
	}

	std::cout << "Main Frag length: " << sizeof(Render::frag_src) << "\n";
	std::cout << "Main Frag9 length: " << sizeof(Render::frag9_src) << "\n";

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
		std::cerr << Addr.error() << "\n";
		return -1;
	}
	app::NetConnection = std::move(*Addr);
	#ifdef _DEBUG
	std::cerr << "IP Address: 0x" << std::hex << app::NetConnection->ip << "\n";
	std::cerr << "IP Port: 0x" << std::hex << app::NetConnection->port << "\n";
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
		std::cerr << "FAILED\n";
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
	WSACleanup();
	return 0;
}
