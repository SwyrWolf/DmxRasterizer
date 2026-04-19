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
import appState;

export class NetManager {
private:
	std::atomic<bool> Operating{true};
	std::array<u8, 1024> m_buffer{};
	std::jthread m_worker;
	std::span<u8> m_dmxData;

public:
	static void create(std::span<u8> storage); // declaration only

	void start(std::optional<winsock::Endpoint>& connection) {
		m_worker = std::jthread([this, &connection](std::stop_token st) {
			this->operatingLoop(st, connection);
		});
	}
	void resume() {
		Operating.store(true, std::memory_order_release);
		Operating.notify_one();
		auto ipStr = app::ipString();
		app::Debug = std::format(L"Listening for Art-Net on [{}:{}]", std::wstring(ipStr.begin(), ipStr.end()), app::NetConnection->port);
	}
	void wait() {
		Operating.wait(false, std::memory_order_acquire);
		Operating.store(false, std::memory_order_release);
	}
	void terminate() {
		m_worker.request_stop();
	}

private:
	void operatingLoop(std::stop_token st, std::optional<winsock::Endpoint>& ep) {
		while (!st.stop_requested()) {
			wait();

			while (!st.stop_requested() && ep.has_value()) {

				auto R_NetPacket = winsock::RecieveNetPacket(m_buffer, ep.value());
				if (!R_NetPacket) {
					if (R_NetPacket.error() == winsock::Err::recieve_SocketClosed) break;
					std::println(stderr, "Failed to recieve DMX data.");
					continue;
				}

				const auto NetPacketSize = as<std::size_t>(*R_NetPacket);
				if (NetPacketSize < 18 || NetPacketSize > 530) continue;
				auto bytes = as<std::size_t>(*R_NetPacket);

				auto data = std::span<const u8>(m_buffer.data(), bytes);
				if (auto r = artnet::ProcessDmxPacket(data, m_dmxData, 8); !r) {
					std::println(stderr, "Failed to process DMX data. {}", as<int>(r.error()));
					continue;
				} else { 
					app::times.MeasureTimeDelta(r.value());
					app::times.signalRender();
					if (app::RelaySend && app::RelayUDP) {
						relay::SendDmx(
							r.value(), std::span<const u8>(m_buffer.data() + artnet::MIN_PACKET_SIZE, artnet::DMX_SIZE)
						);
					}
				}
			}
			app::Debug = L"Stopped Listening for Art-Net packets.";
		}
	}
};

export class ManagedOptional {
	std::optional<NetManager> m_value;
	friend class NetManager;

public:
	const std::optional<NetManager>& get() const { return m_value; }
	NetManager* operator->() { return &m_value.value(); }
	const NetManager* operator->() const { return &m_value.value(); }
	bool exists() const { return m_value.has_value(); }
	explicit operator bool() const { return m_value.has_value(); }
};

export ManagedOptional netManager;

void NetManager::create(std::span<u8> storage) {
	netManager.m_value.emplace();
	netManager.m_value->m_dmxData = storage;
}