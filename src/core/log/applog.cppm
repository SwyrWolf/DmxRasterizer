module;

#include <chrono>
#include <ranges>
#include <atomic>

export module applog;
import weretype;

export namespace applog {
	using TimePoint = std::chrono::steady_clock::time_point;
	using TimeDelta = std::chrono::duration<f64>;

	class UniverseTimer {
		std::array<TimePoint, 9> m_TimeRecord{};
		std::array<TimeDelta, 9> m_TimeDelta{};

		std::atomic<bool> m_renderReady{true};

	public:
		void MeasureTimeDelta(u16 universeID) {
			auto now = std::chrono::steady_clock::now();

			if (m_TimeRecord[universeID].time_since_epoch().count() == 0) {
				m_TimeDelta[universeID] = std::chrono::duration<f64>::zero();
			} else {
				m_TimeDelta[universeID] = now - m_TimeRecord[universeID];
			}
			m_TimeRecord[universeID] = now;
		}

		auto GetTimeDeltasMs() {
			return m_TimeDelta
			| std::views::transform([](const auto& i) { return i.count() * 1000; });
		}

		const f64 GetTimeDeltaMsAt(std::size_t i) {
			return m_TimeDelta[i].count() * 1000.0;
		}

		void signalRender() {
			m_renderReady.store(true, std::memory_order_release);
			m_renderReady.notify_one();
		}

		void waitForRender() {
			m_renderReady.wait(false, std::memory_order_acquire);
			m_renderReady.store(false, std::memory_order_release);
		}
	};
}