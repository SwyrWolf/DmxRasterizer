module;

#include <array>
#include <expected>
#include <stop_token>
#include <atomic>
#include <format>
#include <print>

export module netThread;
import weretype;
import net.winsock;
import net.artnet;
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

export void NetworkThread( std::stop_token st, winsock::Endpoint& ep ) {
	std::array<u8, 1024> buffer{};

	while (!st.stop_requested()) {
		NetThreadWait();
		while (!st.stop_requested()) {

			if (auto r = winsock::RecieveNetPacket(buffer, ep); !r) {
				if (r.error() == winsock::Err::recieve_SocketClosed) break;
				std::println(stderr, "Failed to recieve DMX data.");
				continue;
			} else {
				std::println(stderr, "dmxLength: {:d}", r.value());
			}
			
			if (auto r = artnet::ProcessDmxPacket(buffer, Render::DmxTexture.DmxData, 8); !r) {
				std::println(stderr, "Failed to process DMX data. {}", as<int>(r.value()));
				continue;
			} else { 
				app::times.MeasureTimeDelta(r.value());
				app::times.signalRender();
			}
		}
		app::Debug = L"Stopped Listening for Art-Net packets.";
	}
}