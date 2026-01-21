#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include <iostream>
#include <unordered_map>
#include <thread>
#include <span>
#include <expected>

#include "./external/vendor/glad.h"
#include <glfw3.h>
#include <SpoutGL/SpoutSender.h>
#include "../../external/vendor/imGui/imgui.h"
#include "../../external/vendor/imGui/backends/imgui_impl_glfw.h"
#include "../../external/vendor/imGui/backends/imgui_impl_opengl3.h"

import weretype;
import appState;
import shader;
import artnet;
import render;
import net.artnet;
import net.winsock;

int main(int argc, char* argv[]) {
	ArtNet::UniverseLogger dmxLogger;

	enum ArgType {VERSION, PORT, DEBUG, BINDIP, CH9, UNKNOWN };
	std::unordered_map<std::string, ArgType> argMap = {
		{"-v", VERSION},
		{"--version", VERSION},
		{"-d", DEBUG},
		{"--debug", DEBUG},
		{"-p", PORT},
		{"--port", PORT},
		{"--bind", BINDIP},
		{"-9", CH9},
		{"--RGB", CH9}
	};
	
	if (argc > 1) {
		for (int i = 1; i < argc; ++i) {
			std::string arg = argv[i];
			
			auto theArg = argMap.find(arg);
			ArgType argType = (theArg != argMap.end()) ? theArg->second : UNKNOWN;
			
			switch (argType) {
				case VERSION:
				std::cout << "DMX Rasterizer: " << app::Version << std::endl;
				break;
				case PORT:
				if (i + 1 < argc) {
					if (auto port = [](std::string_view s) -> std::expected<int, std::string> {
						int value{};
						auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);

						if (ec != std::errc{} || ptr != s.data() + s.size()) return std::unexpected("invalid numeric port");
						if (value < 1 || value > 65535) return std::unexpected("port out of range (1–65535)");

						return value;
					}(argv[++i])) {
						app::ipPort = *port;
					} else {
						std::cerr << "Error parsing --port: " << port.error() << '\n';
						std::cerr << "Using default port: " << app::ipPort << '\n';
					}
				} else {
					std::cerr << "Error: Missing value for " << arg << " flag.\n";
					break;
				}
				
				case DEBUG:
				app::debugMode = true;
				break;
				
				case BINDIP:
				if (i + 1 < argc) {
					std::string ip = argv[++i];
					IN_ADDR testAddr; // <-- Declare here
					if (InetPtonA(AF_INET, ip.c_str(), &testAddr) != 1) {
						std::cerr << "Warning: Invalid IP format: " << ip << "\n";
					} else {
						app::bindIp.emplace(ip);
					}
				} else {
					std::cerr << "Error: Missing IP address after --bind\n";
				}
				break;
				
				case CH9:
				app::RGBmode = true;
				Render::DmxTexture.Universes = 9;
				Render::DmxTexture.Channels = ArtNet::TOTAL_DMX_CHANNELS * 3;
				std::cout << "RGB mode on\n";
				break;
				
				case UNKNOWN:
				default:
				std::cerr << "Unkown argument: " << arg << std::endl;
				break;
			}
		}
	}

	if (!app::bindIp) {
		int input;
		std::cout << "Unicase Mode (0 - No | 1 - True)\nInput: ";
		std::cin >> input;

		app::bindIp = (input == 0) ? "0.0.0.0"
			: (input == 1) ? "127.0.0.1"
			: "0.0.0.0";
	}
	
	dmxLogger.dmxData.resize(Render::DmxTexture.Channels);
	Render::DmxTexture.ChannelsNormalized.resize(Render::DmxTexture.Channels);

	auto init = Render::InitGLFW(Render::DmxTexture)
		.and_then(Render::InitGLAD);
	if (!init) {
		std::cerr << init.error() << "\n";
		return -1;
	}

	Render::SetupDmxDataTexture();
	auto shader = Render::SetupShaderLoad();
	Render::SetupVertexArrBuf();
	Render::SetupTextureAndBuffer();
	
	winsock::Net net{};
	winsock::AddressV4 addr{0, 5555};
	std::string ipstr{app::ipStr.data(), app::ipStr.size()};

	auto ipResult = winsock::ipv4_fromString(ipstr);
	if (!ipResult) {
		std::cerr << init.error() << "\n";
		return -1;
	}
	addr.ip = ipResult.value();
	
	auto socketResult = net.OpenNetworkSocket(addr);
	if (!socketResult) {
		std::cerr << socketResult.error() << "\n";
		return -1;
	}
	
	SOCKET artNetSocket = setupArtNetSocket(app::ipPort, app::bindIp);
	std::span<byte> dmxSpan(dmxLogger.dmxData);
	std::thread artNetThread(receiveArtNetData, artNetSocket, dmxSpan, std::ref(dmxLogger));

	glfwMakeContextCurrent(nullptr);
	std::thread renderThread(
		Render::renderLoop,
		std::ref(dmxLogger),
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
	std::thread guiThread(
		ImGuiLoop
	);
	
	//Main thread loop
	while (!glfwWindowShouldClose(app::GuiWindow)) {
		glfwPollEvents();
	}
	
	app::running = false;
	dmxLogger.signalRender();
	
	closesocket(artNetSocket);
	artNetThread.join();
	renderThread.join();
	guiThread.join();
	
	glDeleteBuffers(1, &Render::VBO);
	glfwTerminate();
	WSACleanup();
	return 0;
}
