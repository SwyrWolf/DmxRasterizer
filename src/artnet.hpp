#pragma once

// #define NOMINMAX  // Disable Windows min/max macros

#include <iostream>
#include <cstdint>
#include <array>
#include <ranges>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <atomic>

#include <winsock2.h>
#include <ws2tcpip.h>

// DMX's full name is DMX512
// VRSL only supports 3 universes skipping 8 channels per universe

namespace ArtNet {
	constexpr int DMX_UNIVERSE_SIZE = 512;
	constexpr int VRSL_MAX_UNIVERSES = 3;
	constexpr int VRSL_UNIVERSE_GRID = (DMX_UNIVERSE_SIZE + 8);
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
	private:
		std::array<std::chrono::steady_clock::time_point, VRSL_MAX_UNIVERSES> networkTimeRecord = {};
		std::array<std::chrono::duration<double>, VRSL_MAX_UNIVERSES> networkTimeDelta = {};

		std::mutex dmxRenderPass;
		std::atomic<bool> renderReady = true;
		
		public:
		std::atomic<bool> running = true;
		void MeasureTimeDelta(byte universeID);
		auto GetTimeDeltasMs();
		
		void signalRender();
		void waitForRender();
	};
}

SOCKET setupArtNetSocket(int port, const std::optional<std::string>& bindIpOpt);
void receiveArtNetData(SOCKET sock, std::array<byte, ArtNet::TOTAL_DMX_CHANNELS>& dmxData, ArtNet::UniverseLogger& logger);
