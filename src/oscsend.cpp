#include "oscsend.hpp"

namespace OSC {

	OSCSender client(OSC::LOCAL_HOST, 12000);

	OSCSender::OSCSender(const std::string& ip, int port) {
		WSADATA wsa;
		WSAStartup(MAKEWORD(2, 2), &wsa);

		sock = socket(AF_INET, SOCK_DGRAM, 0);
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(port);
		
		if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) != 1) {
			std::cerr << "Failed to parse IP address: " << ip << std::endl;
			serverAddr.sin_addr.s_addr = INADDR_NONE;
		}
	}

	void OSCSender::padString(std::vector<std::byte>& buffer, const std::string& str) {
		buffer.insert(buffer.end(), reinterpret_cast<const std::byte*>(str.data()), 
			reinterpret_cast<const std::byte*>(str.data() + str.size()));
		buffer.push_back(std::byte{0});
		while (buffer.size() % 4 != 0) buffer.push_back(std::byte{0}); // Pad to 4-byte alignment
	}

	void OSCSender::sendOSCMessage(int channel, uint8_t val) {
		std::vector<std::byte> buffer;

		std::string address = std::string(OSC::VRSL_PREFIX) + std::to_string(channel);
		padString(buffer, address);
		padString(buffer, ",i");

		uint32_t intValue = 0x00'00'00'00 | val;
		intValue = htonl(intValue);

		buffer.insert(buffer.end(), reinterpret_cast<std::byte*>(&intValue),
			reinterpret_cast<std::byte*>(&intValue) + sizeof(intValue));

		sendto(sock, reinterpret_cast<const char*>(buffer.data()), buffer.size(), 0,
			reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
	}

	void OSCSender::toggleOSC(bool set) {
		senderToggle = set;
	}

	bool OSCSender::getToggle() {
		return senderToggle;
	}

	OSCSender::~OSCSender() {
		closesocket(sock);
		WSACleanup();
	}
}