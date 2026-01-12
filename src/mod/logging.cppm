module;

#include <chrono>
#include <ranges>

export module logging;
import weretype;

export namespace logging {
	class UniLog {
	public:

		void MeasureTimeDlete(u16 universeID) {
			auto now = std::chrono::steady_clock::now();

			if (m_networkTimeRecord[universeID].time_since_epoch().count() == 0) {
				m_networkTimeDelta[universeID] = std::chrono::duration<double>::zero();
			} else {
				m_networkTimeDelta[universeID] = now - m_networkTimeRecord[universeID];
			}
			m_networkTimeRecord[universeID] = now;
		}

		auto GetTimeDeltasMs() {
			 return m_networkTimeDelta
				| std::views::transform([](const auto& i) { return i.count() * 1000; });
		}

	private:
		std::vector<std::chrono::steady_clock::time_point> m_networkTimeRecord{9};
		std::vector<std::chrono::duration<double>> m_networkTimeDelta{9};
	};
}