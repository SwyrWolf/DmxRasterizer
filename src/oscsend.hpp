#pragma once

#define NOMINMAX  // Disable Windows min/max macros
#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <cstddef>
#include <winsock2.h>
#include <ws2tcpip.h>  // ✅ Required for `inet_pton`
#pragma comment(lib, "ws2_32.lib")

namespace OSC {
	constexpr std::string VRSL_PREFIX = "/VRSL/ch";
	constexpr std::string LOCAL_HOST = "127.0.0.1";

	class OSCSender {
	private:
		SOCKET sock;
		sockaddr_in serverAddr;
		bool senderToggle = false;
		
		void padString(std::vector<std::byte>& buffer, const std::string& str);

	public:
		OSCSender(const std::string& ip, int port);
		void sendOSCMessage(int channel, uint8_t val);
		void toggleOSC(bool set);
		bool getToggle();
		~OSCSender();
	};

	extern OSCSender client;
}