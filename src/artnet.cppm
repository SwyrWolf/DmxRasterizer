module;

#include <iostream>
#include <optional>
#include <array>
#include <chrono>
#include <mutex>
#include <atomic>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "oscsend.hpp"

export module artnet;
import net.artnet;
import appState;
import weretype;

export namespace ArtNet {
	constexpr int DMX_UNIVERSE_SIZE = 512;
	constexpr int VRSL_UNIVERSE_GRID = (DMX_UNIVERSE_SIZE + 8);
	constexpr int VRSL_MAX_UNIVERSES = 3;
	constexpr int TOTAL_DMX_CHANNELS = VRSL_UNIVERSE_GRID * VRSL_MAX_UNIVERSES;
	
	constexpr int H_RENDER_WIDTH = 1920;
	constexpr int H_RENDER_HEIGHT = 208;
	constexpr int H_GRID_COLUMNS = H_RENDER_WIDTH / 16;
	constexpr int H_GRID_ROWS = H_RENDER_HEIGHT / 16;
	
	constexpr int V_RENDER_WIDTH = 208;
	constexpr int V_RENDER_HEIGHT = 1072;
	constexpr int V_GRID_COLUMNS = V_RENDER_WIDTH / 16;
	constexpr int V_GRID_ROWS = V_RENDER_HEIGHT / 16;

	class UniverseLogger {
	public:
		int Universes{3};
		int Channels{1560};
		std::vector<u8> dmxData;
		std::vector<f32> dmxDataNormalized;
		artnet::DmxIO dmxIO{};
		artnet::GridNode gridNode{};

		void MeasureTimeDelta(byte universeID) {
			auto now = std::chrono::steady_clock::now();
	
			if (networkTimeRecord[universeID].time_since_epoch().count() == 0) {
				networkTimeDelta[universeID] = std::chrono::duration<double>::zero();
			} else {
				networkTimeDelta[universeID] = now - networkTimeRecord[universeID];
			}
			networkTimeRecord[universeID] = now;
		}

		auto GetTimeDeltasMs() {
			 return networkTimeDelta
				| std::views::transform([](const auto& i) { return i.count() * 1000; });
		}
	
		void signalRender() {
			renderReady.store(true, std::memory_order_release);
			renderReady.notify_one();
		}
	
		void waitForRender() {
			renderReady.wait(false, std::memory_order_acquire);
			renderReady.store(false, std::memory_order_release);
		}

	private:
		std::vector<std::chrono::steady_clock::time_point> networkTimeRecord{9};
		std::vector<std::chrono::duration<double>> networkTimeDelta{9};
	
		std::mutex dmxRenderPass;
		std::atomic<bool> renderReady{true};
	};
}

export SOCKET setupArtNetSocket(int port, const std::optional<std::string>& bindIpOpt) {
	WSADATA wsaData;
	if (WSAStartup(0x0202, &wsaData) != 0) {
		std::cerr << "Failed to initialize Winsock." << std::endl;
		exit(-1);
	}

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) {
		std::cerr << "Failed to create socket." << std::endl;
		WSACleanup();
		exit(-1);
	}

	
	int enable = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(enable)) < 0) {
		std::cerr << "Failed to set socket options." << std::endl;
		closesocket(sock);
		WSACleanup();
		exit(-1);
	}

	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&enable, sizeof(enable)) < 0) {
		std::cerr << "Failed to enable SO_BROADCAST." << std::endl;
		closesocket(sock);
		WSACleanup();
		exit(-1);
	}
	
	sockaddr_in localAddr{};
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(port); // Change to 6455 if testing another port
	
	if (bindIpOpt.has_value()) {
		IN_ADDR addr{};
		if (InetPtonA(AF_INET, bindIpOpt->c_str(), &addr) != 1) {
			std::cerr << "Invalid IP address format: " << *bindIpOpt << "\n";
			exit(-1);
		}
		localAddr.sin_addr = addr;
		std::cout << "Binding to interface: " << *bindIpOpt << std::endl;
	} else {
		localAddr.sin_addr.s_addr = INADDR_LOOPBACK;
		std::cerr << "Warning: No bind IP specified. Using INADDR_ANY (0.0.0.0).\n";
		std::cerr << "         Broadcast packets may not be received reliably on multi-NIC systems.\n";
	}

	if (bind(sock, raw<sockaddr*>(&localAddr), sizeof(localAddr)) == SOCKET_ERROR) {
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

export void receiveArtNetData(SOCKET sock, std::span<u8> dmxData, ArtNet::UniverseLogger& logger) {
	while (app::running) {
		std::array<char, 1024> buffer{};
		sockaddr_in senderAddr{};
		int senderAddrSize = sizeof(senderAddr);

		int bytesReceived = recvfrom(sock, buffer.data(), buffer.size(), 0, (sockaddr*)&senderAddr, &senderAddrSize);
		if (bytesReceived == SOCKET_ERROR) {
			int error = WSAGetLastError();
			std::cerr << "Failed to receive UDP packet. Error: " << error << std::endl;
			return;
		}

		std::span<u8> buf{raw<u8*>(buffer.data()), buffer.size()};
		auto res = logger.dmxIO.ProcessPacket(buf);
		if (!res) { 
			std::cerr << "net.Artnet failed! \n"; 
		} else {
			auto r = logger.gridNode.set(res.value(), logger.dmxIO.Read());
			if (!r) { 
				std::cerr << "net.Artnet failed at step 2\n"; 
			}
		}

		
		if (bytesReceived > 18 && std::strncmp(buffer.data(), "Art-Net", 7) == 0) {
			u16 universeID = buffer[14] | (buffer[15] << 8);
			
			if (universeID < logger.Universes) {
				int dmxDataLength = std::min<int>(bytesReceived - 18, as<int>(ArtNet::DMX_UNIVERSE_SIZE));
				std::span<const u8> dmxStart{raw<const u8*>(buffer.data() + 18), 512};

				std::array<u8, 512> dmxArray;
				std::memcpy(dmxArray.data(), dmxStart.data(), dmxArray.size());
				
				int universeOffset = (universeID * ArtNet::VRSL_UNIVERSE_GRID);
				if (universeOffset + dmxDataLength <= logger.Channels) {
					std::memcpy(&dmxData[universeOffset], dmxStart.data(), dmxDataLength);

					if (OSC::client.getToggle()) {
						// OSC::client.sendOSCBundle(dmxArray, universeOffset);
						for (auto [index, value] : dmxData | std::views::enumerate) {
							OSC::client.sendOSCMessage(index, value);
						}
					}
				}

				logger.MeasureTimeDelta(universeID);
				logger.signalRender();
			}

			std::cout << "Received Data:\n";
			auto deltas = logger.GetTimeDeltasMs();

			for (auto [index, value] : deltas | std::views::enumerate) {
				std::cout << "\r\033[K";
				std::cout << "-> Universe " << index << ": " << value << "ms\n";
			} 

			std::cout << "\033[" << (deltas.size() + 1) << "A" << std::flush;
		}
	}
}