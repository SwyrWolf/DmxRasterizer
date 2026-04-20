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
import applog;

export class NetManager {
private:
	std::atomic<bool> m_operating{true};
	std::array<u8, 1024> m_buffer{};
	std::jthread m_worker;

	std::span<u8> mv_dmxData;
	applog::UniverseTimer* p_times = nullptr;
	std::wstring* p_debug = nullptr;
	std::optional<winsock::Endpoint>* p_netConn = nullptr;
	std::span<char> mv_ipStr;
	
	public:
	NetManager() = default;
	~NetManager() {
		terminate();
	}
	static auto create(
		std::span<u8> storage,
		applog::UniverseTimer* times,
		std::wstring* debug
	) -> std::unique_ptr<NetManager> {
		auto manager = std::unique_ptr<NetManager>(new NetManager());
		manager->mv_dmxData = storage;
		manager->p_times = times;
		manager->p_debug = debug;

		return manager;
	}

	void start(std::optional<winsock::Endpoint>& connection) {
		m_worker = std::jthread([this, &connection](std::stop_token st) {
			this->operatingLoop(st, connection);
		});
	}

	void resume() {
		m_operating.store(true, std::memory_order_release);
		m_operating.notify_one();
	}

	void wait() {
		m_operating.wait(false, std::memory_order_acquire);
		m_operating.store(false, std::memory_order_release);
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
				if (auto r = artnet::ProcessDmxPacket(data, mv_dmxData, 8); !r) {
					std::println(stderr, "Failed to process DMX data. {}", as<int>(r.error()));
					continue;
				} else { 
					p_times->MeasureTimeDelta(r.value());
					p_times->signalRender();
					if (relay::isSending && relay::udpEndpoint) {
						relay::SendDmx(
							r.value(), std::span<const u8>(m_buffer.data() + artnet::MIN_PACKET_SIZE, artnet::DMX_SIZE)
						);
					}
				}
			}
			*p_debug = L"Stopped Listening for Art-Net packets.";
		}
	}
};