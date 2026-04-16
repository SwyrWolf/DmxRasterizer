module;

#include <array>
#include <optional>
#include <expected>
#include <stop_token>
#include <atomic>
#include <format>
#include <print>
#include <span>

export module netThread;
import weretype;
import net.winsock;
import net.artnet;
import net.relay;
import render;
import appState;

export std::atomic<bool> NetReady{true};

export void ContinueNetThread() {
	NetReady.store(true, std::memory_order_release);
	NetReady.notify_one();

	auto ipStr = app::ipString();
	app::Debug = std::format(L"Listening for Art-Net on [{}:{}]", std::wstring(ipStr.begin(), ipStr.end()), app::NetConnection->port);
}

void NetThreadWait() {
	NetReady.wait(false, std::memory_order_acquire);
	NetReady.store(false, std::memory_order_release);
}

export void NetworkThread( std::stop_token st, std::optional<winsock::Endpoint>& ep ) {
	std::array<u8, 1024> buffer{};

	while (!st.stop_requested()) {
		NetThreadWait();
		while (!st.stop_requested()) {
			if (!ep.has_value()) break;

			if (auto r = winsock::RecieveNetPacket(buffer, ep.value()); !r) {
				if (r.error() == winsock::Err::recieve_SocketClosed) break;
				std::println(stderr, "Failed to recieve DMX data.");
				continue;
			} else {
				std::println("size: {}", r.value());
			}
			
			if (auto r = artnet::ProcessDmxPacket(buffer, Render::DmxTexture.DmxData, 8); !r) {
				std::println(stderr, "Failed to process DMX data. {}", as<int>(r.value()));
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