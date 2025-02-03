#pragma once

#define NOMINMAX  // Disable Windows min/max macros

#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <optional>
#include <vector>
#include <chrono>

#include <winsock2.h>

// DMX's full name is DMX512
// VRSL only supports 3 universes skipping 8 channels per universe

namespace ArtNet {
	constexpr int DMX_UNIVERSE_SIZE = 512;
	constexpr int VRSL_UNIVERSE_GRID = (DMX_UNIVERSE_SIZE + 8);
	constexpr int TOTAL_DMX_CHANNELS = VRSL_UNIVERSE_GRID * 3;

	class Universe {
		private:
 
			bool active = false;

		public:
			void SetActive();
			void SetInactive();
			bool IsActive() const;
	};

	class UniverseLogger {
	private:
		std::unordered_map<uint16_t, std::chrono::steady_clock::time_point> networkTimeRecord;
		std::unordered_map<uint16_t, std::chrono::duration<double>> networkTimeDelta;

	public:
		void MeasureTimeDelta(uint16_t universeID);
		std::unordered_map<uint16_t, double> GetTimeDeltasMs() const;
	};
}

SOCKET setupArtNetSocket(int port);
void receiveArtNetData(SOCKET sock, std::unordered_map<uint16_t, std::vector<uint8_t>>& dmxDataMap, uint8_t* dmxData, ArtNet::UniverseLogger& logger);