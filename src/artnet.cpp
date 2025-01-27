#include "artnet.hpp"
#include "global.hpp"

namespace ArtNet {
	void Universe::SetActive() {
		if (!active) { active = true; }
	}
	void Universe::SetInactive() {
		if (active) { active = false; }
	}
	bool Universe::IsActive() const { return active; }

	void UniverseLogger::MeasureTimeDelta(uint16_t universeID) {
		auto now = std::chrono::steady_clock::now();
		if (networkTimeRecord.find(universeID) != networkTimeRecord.end()) {
			networkTimeDelta[universeID] = now - networkTimeRecord.at(universeID);
		} else {
			networkTimeDelta[universeID] = std::chrono::duration<double>::zero();
		}

		networkTimeRecord[universeID] = now;
	}

	std::unordered_map<uint16_t, double> UniverseLogger::GetTimeDeltasMs() const {
		std::unordered_map<uint16_t, double> deltas;
		for (const auto& [id, delta] : networkTimeDelta) {
			deltas[id] = delta.count() * 1000;
		}
		return deltas;
	}

}

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

void receiveArtNetData(SOCKET sock, std::unordered_map<uint16_t, std::vector<uint8_t>>& dmxDataMap, uint8_t* dmxData, ArtNet::UniverseLogger& logger) {
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
		uint16_t universeID = buffer[14] | (buffer[15] << 8);

		int dmxDataLength = 0;
		if (universeID < 4) {
			const auto dmxStart = reinterpret_cast<uint8_t*>(&buffer[18]);
			dmxDataLength = std::min(bytesReceived - 18, static_cast<int>(ArtNet::DMX_UNIVERSE_SIZE));

			if (dmxDataMap.find(universeID) == dmxDataMap.end()) {
					dmxDataMap[universeID] = std::vector<uint8_t>(ArtNet::DMX_UNIVERSE_SIZE, 0);
			}

			std::memcpy(dmxDataMap[universeID].data(), dmxStart, dmxDataLength);

			int universeOffset = universeID * ArtNet::DMX_UNIVERSE_SIZE;
			if (universeOffset + dmxDataLength <= ArtNet::TOTAL_DMX_CHANNELS) {
					std::memcpy(&dmxData[universeOffset], dmxStart, dmxDataLength);
			}

			logger.MeasureTimeDelta(universeID);
		}

		std::cout << "Received Data:\n";
		auto deltas = logger.GetTimeDeltasMs();
		for (const auto& [id, deltaMs] : deltas) {
			std::cout << "-> Universe " << id << ": " << deltaMs << "ms\n";
		}

		std::cout << "\033[" << (deltas.size() + 1) << "A" << std::flush;
	}
}