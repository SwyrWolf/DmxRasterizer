#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <glad/glad.h>
#include <glfw3.h>
#include "shader.h"

#define NOMINMAX
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

unsigned char dmxData[512] = {0};

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

SOCKET setupArtNetSocket() {
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
    localAddr.sin_port = htons(6454); // Change to 6455 if testing another port
    localAddr.sin_addr.s_addr = INADDR_ANY;

    // Enable reuse of the address to prevent "address already in use" errors
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

    std::cout << "Listening for Art-Net packets on port 6454..." << std::endl;
    return sock;
}

// Function to receive DMX data from Art-Net
void receiveArtNetData(SOCKET sock, unsigned char* dmxData) {
    char buffer[1024];
    sockaddr_in senderAddr;
    int senderAddrSize = sizeof(senderAddr);

    // Receive UDP packet
    int bytesReceived = recvfrom(sock, buffer, sizeof(buffer), 0, (sockaddr*)&senderAddr, &senderAddrSize);
    if (bytesReceived == SOCKET_ERROR) {
        int error = WSAGetLastError();
        std::cerr << "Failed to receive UDP packet. Error: " << error << std::endl;
        return;
    }

    // Use WSAAddressToStringW for wide-character output
    wchar_t senderIP[INET_ADDRSTRLEN];
    DWORD senderIPLen = INET_ADDRSTRLEN;

    sockaddr_in tempAddr = senderAddr;
    if (WSAAddressToStringW((LPSOCKADDR)&tempAddr, sizeof(tempAddr), NULL, senderIP, &senderIPLen) == SOCKET_ERROR) {
        std::cerr << "Failed to convert sender address to string. Error: " << WSAGetLastError() << std::endl;
        return;
    }

    // Convert wide-character string to narrow-character string using _wcstombs_s
    char senderIPNarrow[INET_ADDRSTRLEN];
    size_t convertedChars = 0;
    errno_t err = wcstombs_s(&convertedChars, senderIPNarrow, INET_ADDRSTRLEN, senderIP, _TRUNCATE);
    if (err != 0) {
        std::cerr << "Failed to convert wide-character address to multibyte string." << std::endl;
        return;
    }

    std::cout << "Received " << bytesReceived << " bytes from " << senderIPNarrow << ":" << ntohs(senderAddr.sin_port) << std::endl;

    // If it's an Art-Net packet, parse and process DMX data
    if (bytesReceived > 18 && std::strncmp(buffer, "Art-Net", 7) == 0) {
        std::cout << "Art-Net packet detected!" << std::endl;

		uint16_t universe = (static_cast<uint8_t>(buffer[14])) | (static_cast<uint8_t>(buffer[15]) << 8);
        std::cout << "Universe: " << universe << std::endl;


        uint8_t* dmxStart = (uint8_t*)&buffer[18];
        std::memcpy(dmxData, dmxStart, std::min(bytesReceived - 18, 512));

        // Print the first few DMX channels
        std::cout << "DMX Data: ";
        for (int i = 0; i < std::min(16, 512); ++i) {
            std::cout << static_cast<int>(dmxData[i]) << " ";
        }
        std::cout << std::endl;
    }
}

int main() {
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	GLFWwindow* window = glfwCreateWindow(960, 64, "DMX Shader Renderer", NULL, NULL);
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

	glViewport(0, 0, 800, 600);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	Shader shader("vertex.glsl", "frag.glsl");
	float vertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f, // Top-left
        -1.0f, -1.0f,  0.0f, 0.0f, // Bottom-left
         1.0f, -1.0f,  1.0f, 0.0f, // Bottom-right

        -1.0f,  1.0f,  0.0f, 1.0f, // Top-left
         1.0f, -1.0f,  1.0f, 0.0f, // Bottom-right
         1.0f,  1.0f,  1.0f, 1.0f  // Top-right
    };

	unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

	SOCKET artNetSocket = setupArtNetSocket();

	float dmxDataNormalized[15]; // Normalized DMX data for the first 15 channels

	while (!glfwWindowShouldClose(window)) {
		receiveArtNetData(artNetSocket, dmxData);

		for (int i = 0; i < 15; ++i) {
			dmxDataNormalized[i] = dmxData[i] / 255.0f;
		}

		for (int i = 0; i < 15; ++i) {
			std::cout << "DMX Channel " << (i + 1) << ": " << dmxDataNormalized[i] << std::endl;
		}

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT);

		// Use the shader program
		shader.use();

		// Pass normalized DMX data to the shader
		int uniformLocation = glGetUniformLocation(shader.ID, "dmxData");
		if (uniformLocation != -1) {
			glUniform1fv(uniformLocation, 15, dmxDataNormalized);
		} else {
			std::cerr << "ERROR: Uniform 'dmxData' not found in the shader." << std::endl;
		}

		// Render the fullscreen quad
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

    closesocket(artNetSocket);
    WSACleanup();
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}