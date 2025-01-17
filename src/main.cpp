#define WIN32_LEAN_AND_MEAN
#define NOMINMAX  // Disable Windows min/max macros
#include <winsock2.h>
#include <windows.h>

#include <iostream>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cstdint>

#include "shader.hpp"

#include <glad/glad.h>
#include <glfw3.h>
#include <SpoutGL/SpoutSender.h>

#pragma comment(lib, "Ws2_32.lib")

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

const uint32_t TOTAL_DMX_CHANNELS = 1560;
const uint32_t RENDER_WIDTH = 1920;
const uint32_t RENDER_HEIGHT = 208;

unsigned char dmxData[TOTAL_DMX_CHANNELS] = {0};
float dmxDataNormalized[TOTAL_DMX_CHANNELS] = {0.0f};
GLuint dmxDataTexture;

SOCKET setupArtNetSocket(int port) {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "Failed to initialize Winsock." << std::endl;
		exit(-1);
	}

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) {
		std::cerr << "Failed to create socket." << std::endl;
		WSACleanup();
		exit(-1);
	}

	sockaddr_in localAddr{};
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(port); // Change to 6455 if testing another port
	localAddr.sin_addr.s_addr = INADDR_ANY;

	int enable = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(enable)) < 0) {
		std::cerr << "Failed to set socket options." << std::endl;
		closesocket(sock);
		WSACleanup();
		exit(-1);
	}

	if (bind(sock, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
		int error = WSAGetLastError();
		std::cerr << "Failed to bind socket. Error code: " << error << std::endl;

		if (error == WSAEADDRINUSE) {
			std::cerr << "Port 6454 is already in use by another application." << std::endl;
		} else if (error == WSAEACCES) {
			std::cerr << "Permission denied. Try running as administrator." << std::endl;
		}

		closesocket(sock);
		WSACleanup();
		exit(-1);
	}

	std::cout << "Listening for Art-Net packets on port " << port << "..." << std::endl;
	return sock;
}

// Function to receive DMX data from Art-Net
void receiveArtNetData(SOCKET sock, unsigned char* dmxData) {
	char buffer[1024];
	sockaddr_in senderAddr;
	int senderAddrSize = sizeof(senderAddr);

	int bytesReceived = recvfrom(sock, buffer, sizeof(buffer), 0, (sockaddr*)&senderAddr, &senderAddrSize);
	if (bytesReceived == SOCKET_ERROR) {
		int error = WSAGetLastError();
		std::cerr << "Failed to receive UDP packet. Error: " << error << std::endl;
		return;
	}

	wchar_t senderIP[INET_ADDRSTRLEN];
	DWORD senderIPLen = INET_ADDRSTRLEN;

	sockaddr_in tempAddr = senderAddr;
	if (WSAAddressToStringW((LPSOCKADDR)&tempAddr, sizeof(tempAddr), NULL, senderIP, &senderIPLen) == SOCKET_ERROR) {
		std::cerr << "Failed to convert sender address to string. Error: " << WSAGetLastError() << std::endl;
		return;
	}

	char senderIPNarrow[INET_ADDRSTRLEN];
	size_t convertedChars = 0;
	errno_t err = wcstombs_s(&convertedChars, senderIPNarrow, INET_ADDRSTRLEN, senderIP, _TRUNCATE);
	if (err != 0) {
		std::cerr << "Failed to convert wide-character address to multibyte string." << std::endl;
		return;
	}

	if (bytesReceived > 18 && std::strncmp(buffer, "Art-Net", 7) == 0) {
		uint16_t universeID = buffer[14] | (buffer[15] << 8);
		int universeOffset = (universeID % 2) * 512;
		uint8_t* dmxStart = (uint8_t*)&buffer[18];
		std::memcpy(dmxData, dmxStart, std::min(bytesReceived - 18, 512));
	}
}

int main(int argc, char* argv[]) {
	int port = 6454;

	if (argc > 1) {
		try {
			port = std::stoi(argv[1]);
			if (port < 1 || port > 65535) {
				throw std::out_of_range("Port out of valid range");
			}
		} catch (const std::exception& e) {
			std::cerr << "Invalid port number provided: " << argv[1] << std::endl;
			std::cerr << "Using default port: " << port << std::endl;
		}
	}

	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(RENDER_WIDTH, RENDER_HEIGHT, "DMX Shader Renderer", NULL, NULL);
	if (!window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glViewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);

	glGenTextures(1, &dmxDataTexture);
	glBindTexture(GL_TEXTURE_1D, dmxDataTexture);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, TOTAL_DMX_CHANNELS, 0, GL_RED, GL_FLOAT, dmxDataNormalized);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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

	SOCKET artNetSocket = setupArtNetSocket(port);

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

	while (!glfwWindowShouldClose(window)) {
		receiveArtNetData(artNetSocket, dmxData);

		for (int i = 0; i < TOTAL_DMX_CHANNELS; ++i) {
			dmxDataNormalized[i] = dmxData[i] / 255.0f;
		}

		glBindTexture(GL_TEXTURE_1D, dmxDataTexture);
		glTexSubImage1D(GL_TEXTURE_1D, 0, 0, TOTAL_DMX_CHANNELS, GL_RED, GL_FLOAT, dmxDataNormalized);

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glClear(GL_COLOR_BUFFER_BIT);
		shader.use();
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		sender.SendTexture(texture, GL_TEXTURE_2D, RENDER_WIDTH, RENDER_HEIGHT);

		glfwPollEvents();
	}

	sender.ReleaseSender();
	glDeleteTextures(1, &texture);
	glDeleteTextures(1, &dmxDataTexture);
	glDeleteFramebuffers(1, &framebuffer);
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glfwTerminate();
	closesocket(artNetSocket);
	WSACleanup();
	return 0;
}