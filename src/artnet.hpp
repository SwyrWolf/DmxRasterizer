#pragma once

#define NOMINMAX  // Disable Windows min/max macros

#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <optional>

#include <winsock2.h>

namespace ArtNet {
	constexpr uint32_t TOTAL_DMX_CHANNELS = 512 * 4;
	constexpr uint16_t DMX_UNIVERSE_SIZE = 512;

	class Universe {
		private:
			std::optional<uint16_t> id = {};
			uint8_t data[DMX_UNIVERSE_SIZE] = {0};
			bool active = false;

		public:
			void SetActive();
			void SetInactive();
			bool IsActive() const;
	};

}

SOCKET setupArtNetSocket(int port);
void receiveArtNetData(SOCKET sock, std::unordered_map<uint16_t, std::vector<uint8_t>>& dmxDataMap, uint8_t* dmxData);