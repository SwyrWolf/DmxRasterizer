module;

#include <array>
#include <optional>
#include <expected>
#include <stop_token>
#include <atomic>
#include <format>
#include <print>
#include <span>
#include <thread>

export module netThread;
import weretype;
import net.winsock;
import net.artnet;
import net.relay;
import render;
import appState;

export struct NetManager {
	std::atomic<bool> Operating{true};
	std::jthread worker;
	std::array<u8, 1024> buffer{};

	void Start(std::optional<winsock::Endpoint>& connection) {
		worker = std::jthread([this, &connection](std::stop_token st) {
			this->operatingLoop(st, connection);
		});
	}

	void Resume() {
		Operating.store(true, std::memory_order_release);
		Operating.notify_one();
		auto ipStr = app::ipString();
		app::Debug = std::format(L"Listening for Art-Net on [{}:{}]", std::wstring(ipStr.begin(), ipStr.end()), app::NetConnection->port);
	}
	void Wait() {
		Operating.wait(false, std::memory_order_acquire);
		Operating.store(false, std::memory_order_release);
	}

	void operatingLoop(std::stop_token st, std::optional<winsock::Endpoint>& ep) {
		while (!st.stop_requested()) {
			Wait();

			while (!st.stop_requested() && ep.has_value()) {

				auto R_NetPacket = winsock::RecieveNetPacket(buffer, ep.value());
				if (!R_NetPacket) {
					if (R_NetPacket.error() == winsock::Err::recieve_SocketClosed) break;
					std::println(stderr, "Failed to recieve DMX data.");
					continue;
				}

				const auto NetPacketSize = as<std::size_t>(*R_NetPacket);
				if (NetPacketSize < 18 || NetPacketSize > 530) continue;
				auto bytes = as<std::size_t>(*R_NetPacket);

				auto data = std::span<const u8>(buffer.data(), bytes);
				if (auto r = artnet::ProcessDmxPacket(data, Render::DmxTexture.DmxData, 8); !r) {
					std::println(stderr, "Failed to process DMX data. {}", as<int>(r.error()));
					continue;
				} else { 
					app::times.MeasureTimeDelta(r.value());
					app::times.signalRender();
					if (app::RelaySend && app::RelayUDP) {
						relay::SendDmx(
							r.value(), std::span<const u8>(buffer.data() + artnet::MIN_PACKET_SIZE, artnet::DMX_SIZE)
						);
					}
				}
			}
			app::Debug = L"Stopped Listening for Art-Net packets.";
		}
	}
};

export std::optional<NetManager> netManager;

// export std::atomic<bool> NetReady{true};

// export void ContinueNetThread() {
// 	NetReady.store(true, std::memory_order_release);
// 	NetReady.notify_one();

// 	auto ipStr = app::ipString();
// 	app::Debug = std::format(L"Listening for Art-Net on [{}:{}]", std::wstring(ipStr.begin(), ipStr.end()), app::NetConnection->port);
// }

// export void NetThreadWait() {
// 	NetReady.wait(false, std::memory_order_acquire);
// 	NetReady.store(false, std::memory_order_release);
// }

// export void NetworkThread( std::stop_token st, std::optional<winsock::Endpoint>& ep ) {
// 	std::array<u8, 1024> buffer{};
// 	std::size_t bytes{};

// 	while (!st.stop_requested()) {
// 		NetThreadWait();

// 		while (!st.stop_requested() && ep.has_value()) {

// 			auto R_NetPacket = winsock::RecieveNetPacket(buffer, ep.value());
// 			if (!R_NetPacket) {
// 				if (R_NetPacket.error() == winsock::Err::recieve_SocketClosed) break;
// 				std::println(stderr, "Failed to recieve DMX data.");
// 				continue;
// 			}

// 			const auto NetPacketSize = *R_NetPacket;
// 			if (NetPacketSize < 18 || NetPacketSize > 530) continue;
// 			bytes = as<std::size_t>(*R_NetPacket);

// 			auto data = std::span<const u8>(buffer.data(), bytes);
// 			if (auto r = artnet::ProcessDmxPacket(data, Render::DmxTexture.DmxData, 8); !r) {
// 				std::println(stderr, "Failed to process DMX data. {}", as<int>(r.error()));
// 				continue;
// 			} else { 
// 				app::times.MeasureTimeDelta(r.value());
// 				app::times.signalRender();
// 				if (app::RelaySend && app::RelayUDP) {
// 					relay::SendDmx(
// 						r.value(), std::span<const u8>(buffer.data() + artnet::MIN_PACKET_SIZE, artnet::DMX_SIZE)
// 					);
// 				}
// 			}
// 		}
// 		app::Debug = L"Stopped Listening for Art-Net packets.";
// 	}
// }