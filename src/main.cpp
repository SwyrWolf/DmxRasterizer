#include <winsock2.h>
#include <windows.h>

#include <iostream>
#include <unordered_map>
#include <thread>
#include <span>

#include "./external/vendor/glad.h"
#include <glfw3.h>
#include <SpoutGL/SpoutSender.h>
#include <ws2tcpip.h>

import weretype;
import shader;
import artnet;
import render;
import appState;

constexpr char vertex_src[] = {
	#embed "../shaders/vertex.glsl"
};
constexpr char frag_src[] = {
	#embed "../shaders/frag.glsl"
};
constexpr char frag9_src[] = {
	#embed "../shaders/frag9.glsl"
};

GLuint dmxDataTexture;

auto RENDER_HEIGHT = ArtNet::H_RENDER_HEIGHT;
auto RENDER_WIDTH = ArtNet::H_RENDER_WIDTH;
auto RENDER_COLUMNS = ArtNet::H_GRID_COLUMNS;
auto RENDER_ROWS = ArtNet::H_GRID_ROWS;

int main(int argc, char* argv[]) {
	ArtNet::UniverseLogger dmxLogger;

	enum ArgType {VERSION, PORT, DEBUG, VERTICAL, OSCSEND, BINDIP, CH9, UNKNOWN };
	std::unordered_map<std::string, ArgType> argMap = {
		{"-v", VERSION},
		{"--version", VERSION},
		{"-p", PORT},
		{"--port", PORT},
		{"-d", DEBUG},
		{"--debug", DEBUG},
		{"-v", VERTICAL},
		{"-o", OSCSEND},
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
					try {
						app::port = std::stoi(argv[++i]);
						if (app::port < 1 || app::port > 65535) {
							throw std::out_of_range("Port out of valid range");
						}
					} catch (const std::exception& e) {
						std::cerr << "Error: Missing value for " << arg << " flag." << std::endl;
						std::cerr << "using default port: " << app::port << std::endl;
					}
					break;
				}
				
				case DEBUG:
				app::debugMode = true;
				break;
				
				case VERTICAL:
				app::verticalMode = true;
				RENDER_HEIGHT = ArtNet::V_RENDER_HEIGHT;
				RENDER_WIDTH = ArtNet::V_RENDER_WIDTH;
				RENDER_COLUMNS = ArtNet::V_GRID_COLUMNS;
				RENDER_ROWS = ArtNet::V_GRID_ROWS;
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
				app::nineMode = true;
				dmxLogger.Universes = 9;
				dmxLogger.Channels = ArtNet::TOTAL_DMX_CHANNELS * 3;
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
	
	dmxLogger.dmxData.resize(dmxLogger.Channels);
	dmxLogger.dmxDataNormalized.resize(dmxLogger.Channels);

	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}
	
	
	if(!app::debugMode){
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(RENDER_WIDTH, RENDER_HEIGHT, "DMX Shader Renderer", nullptr, nullptr);
	if (!window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	
	if (!gladLoadGLLoader(raw<GLADloadproc>(glfwGetProcAddress))) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	
	glViewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);
	
	glGenTextures(1, &dmxDataTexture);
	glBindTexture(GL_TEXTURE_1D, dmxDataTexture);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, dmxLogger.Channels, 0, GL_RED, GL_FLOAT, dmxLogger.dmxDataNormalized.data());
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	std::string_view fragShader = app::nineMode ? frag9_src : frag_src;
	std::string_view vertexShader = vertex_src;
	Shader shader(vertexShader, fragShader);
	glUseProgram(shader.m_ID);
	glUniform2f(glGetUniformLocation(shader.m_ID, "resolution"), RENDER_WIDTH, RENDER_HEIGHT);
	float vertices[] = {
		-1.0f,  1.0f,  0.0f, 1.0f, // Top-left
		-1.0f, -1.0f,  0.0f, 0.0f, // Bottom-left
		1.0f, -1.0f,  1.0f, 0.0f, // Bottom-right
		
		-1.0f,  1.0f,  0.0f, 1.0f, // Top-left
		1.0f, -1.0f,  1.0f, 0.0f, // Bottom-right
		1.0f,  1.0f,  1.0f, 1.0f  // Top-right
	};
	
	GLuint VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	
	SOCKET artNetSocket = setupArtNetSocket(app::port, app::bindIp);
	
	std::span<byte> dmxSpan(dmxLogger.dmxData);
	std::thread artNetThread(receiveArtNetData, artNetSocket, dmxSpan, std::ref(dmxLogger));
	
	SpoutSender sender;
	if (!sender.CreateSender("DmxRasterizer", RENDER_WIDTH, RENDER_HEIGHT)) {
		std::cerr << "Failed to create Spout sender!" << std::endl;
    return -1;
	}
	
	GLuint texture, framebuffer;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RENDER_WIDTH, RENDER_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	
	glfwMakeContextCurrent(nullptr);
	std::thread renderThread(Render::renderLoop, window, std::ref(dmxLogger), std::ref(shader), VAO, dmxDataTexture, std::ref(sender), framebuffer, texture);
	
	//Main thread loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
	
	app::running = false;
	dmxLogger.signalRender();
	
	closesocket(artNetSocket);
	artNetThread.join();
	renderThread.join();
	
	glDeleteBuffers(1, &VBO);
	glfwTerminate();
	WSACleanup();
	return 0;
}
