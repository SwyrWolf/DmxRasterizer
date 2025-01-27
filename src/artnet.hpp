#pragma once

#define NOMINMAX  // Disable Windows min/max macros

#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <optional>
#include <vector>
#include <chrono>

#include <winsock2.h>

namespace ArtNet {
	constexpr uint32_t TOTAL_DMX_CHANNELS = 512 * 4;
	constexpr uint16_t DMX_UNIVERSE_SIZE = 512;

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