#include "artnet.hpp"
#include "global.hpp"

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

	if (bind(sock, reinterpret_cast<sockaddr *>(&localAddr), sizeof(localAddr)) == SOCKET_ERROR) {
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

void receiveArtNetData(SOCKET sock, std::unordered_map<uint16_t, std::vector<uint8_t>>& dmxDataMap, uint8_t* dmxData) {
	char buffer[1024];
	sockaddr_in senderAddr{};
	int senderAddrSize = sizeof(senderAddr);

	int bytesReceived = recvfrom(sock, buffer, sizeof(buffer), 0, (sockaddr*)&senderAddr, &senderAddrSize);
	if (bytesReceived == SOCKET_ERROR) {
			int error = WSAGetLastError();
			std::cerr << "Failed to receive UDP packet. Error: " << error << std::endl;
			return;
	}

	if (bytesReceived > 18 && std::strncmp(buffer, "Art-Net", 7) == 0) {
				uint16_t universeID = buffer[14] | (buffer[15] << 8); // Extract the universe ID

				const auto dmxStart = reinterpret_cast<uint8_t*>(&buffer[18]);
				int dmxDataLength = std::min(bytesReceived - 18, static_cast<int>(DMX_UNIVERSE_SIZE));

				if (dmxDataMap.find(universeID) == dmxDataMap.end()) {
						dmxDataMap[universeID] = std::vector<uint8_t>(DMX_UNIVERSE_SIZE, 0);
				}

				std::memcpy(dmxDataMap[universeID].data(), dmxStart, dmxDataLength);

				int universeOffset = universeID * DMX_UNIVERSE_SIZE;
				if (universeOffset + dmxDataLength <= TOTAL_DMX_CHANNELS) {
						std::memcpy(&dmxData[universeOffset], dmxStart, dmxDataLength);
				}

				std::cout << "\rReceived data for Universe " << universeID
									<< ", Bytes: " << dmxDataLength << "\033[K" << std::flush;
		}
}