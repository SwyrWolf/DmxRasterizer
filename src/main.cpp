#include <winsock2.h>
#include <windows.h>

#include <iostream>
#include <array>
#include <cstdint>
#include <unordered_map>
#include <thread>

#include "./external/vendor/glad.h"
#include <glfw3.h>
#include <SpoutGL/SpoutSender.h>

#include "shader.hpp"
#include "artnet.hpp"
#include "oscsend.hpp"

std::array<uint8_t, ArtNet::TOTAL_DMX_CHANNELS> dmxData{};
std::array<float, ArtNet::TOTAL_DMX_CHANNELS> dmxDataNormalized{};
GLuint dmxDataTexture;

auto RENDER_HEIGHT = ArtNet::H_RENDER_HEIGHT;
auto RENDER_WIDTH = ArtNet::H_RENDER_WIDTH;
auto RENDER_COLUMNS = ArtNet::H_GRID_COLUMNS;
auto RENDER_ROWS = ArtNet::H_GRID_ROWS;

bool debug = false;

void renderLoop(GLFWwindow* window, ArtNet::UniverseLogger& logger, Shader& shader, GLuint VAO, GLuint dmxDataTexture, SpoutSender& sender, GLuint framebuffer, GLuint texture) {

	glfwMakeContextCurrent(window);

	while (logger.running) {
		logger.waitForRender();
		for (int i = 0; i < ArtNet::TOTAL_DMX_CHANNELS; ++i) {
			dmxDataNormalized[i] = dmxData[i] / 255.0f;
		}

		// Update the DMX data into texture
		glBindTexture(GL_TEXTURE_1D, dmxDataTexture);
		glTexSubImage1D(GL_TEXTURE_1D, 0, 0, ArtNet::TOTAL_DMX_CHANNELS, GL_RED, GL_FLOAT, dmxDataNormalized.data());

		// Rendering
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glViewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		shader.use();
		glBindVertexArray(VAO);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_1D, dmxDataTexture);
		glUniform1i(glGetUniformLocation(shader.ID, "dmxDataTexture"), 0);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		sender.SendTexture(texture, GL_TEXTURE_2D, RENDER_WIDTH, RENDER_HEIGHT);

    if (debug) {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);
			glClear(GL_COLOR_BUFFER_BIT);

			shader.use();
			glBindVertexArray(VAO);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_1D, dmxDataTexture);
			glUniform1i(glGetUniformLocation(shader.ID, "dmxDataTexture"), 0);

			glDrawArrays(GL_TRIANGLES, 0, 6);

			glfwSwapBuffers(window);  // Display the rendered image
    }
	}
	glfwMakeContextCurrent(nullptr);
	glDeleteTextures(1, &texture);
	glDeleteTextures(1, &dmxDataTexture);
	glDeleteFramebuffers(1, &framebuffer);
	glDeleteVertexArrays(1, &VAO);
	sender.ReleaseSender();
}

int main(int argc, char* argv[]) {
	ArtNet::UniverseLogger dmxLogger;

	bool vertical = false;
	bool oscArg = false;
	int port = 6454;
	std::optional<std::string> bindIp;
	bool NineChannels = false;
	
	enum ArgType { PORT, DEBUG, VERTICAL, OSCSEND, BINDIP, CH9, UNKNOWN };
	std::unordered_map<std::string, ArgType> argMap = {
		{"-p", PORT},
		{"--port", PORT},
		{"-d", DEBUG},
		{"--debug", DEBUG},
		{"-v", VERTICAL},
		{"-o", OSCSEND},
		{"--bind", BINDIP}
		{"-9", CH9}
		{"--RGB", CH9}
	};

	if (argc > 1) {
		for (int i = 1; i < argc; ++i) {
			std::string arg = argv[i];

			auto theArg = argMap.find(arg);
			ArgType argType = (theArg != argMap.end()) ? theArg->second : UNKNOWN;

			switch (argType) {
				case PORT:
					if (i + 1 < argc) {
						try {
							port = std::stoi(argv[++i]);
							if (port < 1 || port > 65535) {
								throw std::out_of_range("Port out of valid range");
							}
						} catch (const std::exception& e) {
							std::cerr << "Error: Missing value for " << arg << " flag." << std::endl;
							std::cerr << "using default port: " << port << std::endl;
						}
						break;
					}

				case DEBUG:
					debug = true;
					break;

				case VERTICAL:
					vertical = true;
					RENDER_HEIGHT = ArtNet::V_RENDER_HEIGHT;
					RENDER_WIDTH = ArtNet::V_RENDER_WIDTH;
					RENDER_COLUMNS = ArtNet::V_GRID_COLUMNS;
					RENDER_ROWS = ArtNet::V_GRID_ROWS;
					break;

				case OSCSEND:
					OSC::client.toggleOSC(true);
					std::cout << "OSC Enabled" << std::endl;
					break;
				
				case BINDIP:
					if (i + 1 < argc) {
						std::string ip = argv[++i];
						IN_ADDR testAddr; // <-- Declare here
						if (InetPtonA(AF_INET, ip.c_str(), &testAddr) != 1) {
							std::cerr << "Warning: Invalid IP format: " << ip << "\n";
						} else {
							bindIp = ip;
						}
					} else {
						std::cerr << "Error: Missing IP address after --bind\n";
					}
					break;

				case CH9:
					NineChannels = true;
					
				case UNKNOWN:
				default:
					std::cerr << "Unkown argument: " << arg << std::endl;
					break;
			}
		}
	}

	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}


	if(!debug){
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

	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glViewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);

	glGenTextures(1, &dmxDataTexture);
	glBindTexture(GL_TEXTURE_1D, dmxDataTexture);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, ArtNet::TOTAL_DMX_CHANNELS, 0, GL_RED, GL_FLOAT, dmxDataNormalized.data());
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	Shader shader("shaders/vertex.glsl", "shaders/frag.glsl");
	glUseProgram(shader.ID);
	glUniform2f(glGetUniformLocation(shader.ID, "resolution"), RENDER_WIDTH, RENDER_HEIGHT);
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

	SOCKET artNetSocket = setupArtNetSocket(port, bindIp);
	std::thread artNetThread(receiveArtNetData, artNetSocket, std::ref(dmxData), std::ref(dmxLogger));

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
	std::thread renderThread(renderLoop, window, std::ref(dmxLogger), std::ref(shader), VAO, dmxDataTexture, std::ref(sender), framebuffer, texture);

	//Main thread loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	dmxLogger.running = false;
	dmxLogger.signalRender();

	closesocket(artNetSocket);
	artNetThread.join();
	renderThread.join();

	glDeleteBuffers(1, &VBO);
	glfwTerminate();
	WSACleanup();
	return 0;
}
