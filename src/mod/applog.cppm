module;

#include <chrono>
#include <ranges>

export module applog;
import weretype;

export namespace applog {

	class UniverseTimer {
	public:

		void MeasureTimeDelta(u16 universeID) {
			auto now = std::chrono::steady_clock::now();

			if (m_TimeRecord[universeID].time_since_epoch().count() == 0) {
				m_TimeDelta[universeID] = std::chrono::duration<double>::zero();
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

	private:
		std::array<std::chrono::steady_clock::time_point, 9> m_TimeRecord{};
		std::array<std::chrono::duration<double>, 9>         m_TimeDelta{};
	};
}