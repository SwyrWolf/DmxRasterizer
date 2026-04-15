module;

#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <charconv>

export module settings;
import appState;
import weretype;

namespace settings_detail {

	static constexpr std::string_view SETTINGS_FILE = "dmxrasterizer.cfg";

	// Copy at most N-1 chars from src into a char array, always null-terminate
	template<std::size_t N>
	void fill(std::array<char, N>& dst, std::string_view src) {
		auto len = std::min(src.size(), N - 1);
		src.copy(dst.data(), len);
		dst[len] = '\0';
	}

} // namespace settings_detail

export namespace settings {

	void Load() {
		using namespace settings_detail;
		std::ifstream f{ SETTINGS_FILE.data() };
		if (!f.is_open()) return;

		std::string line;
		while (std::getline(f, line)) {
			if (line.empty() || line[0] == '#') continue;
			auto eq = line.find('=');
			if (eq == std::string::npos) continue;
			std::string_view key{ line.data(), eq };
			std::string_view val{ line.data() + eq + 1, line.size() - eq - 1 };

			if (key == "address") fill(app::relayAddress, val);
			else if (key == "name")    fill(app::displayName, val);
			else if (key == "access")  fill(app::relayAccess, val);
			else if (key == "mode") {
				int m{};
				std::from_chars(val.data(), val.data() + val.size(), m);
				app::relayMode = as<app::RelayMode>(m);
			}
		}
	}

	void Save() {
		using namespace settings_detail;
		std::ofstream f{ SETTINGS_FILE.data(), std::ios::trunc };
		if (!f.is_open()) return;

		f << "address=" << app::relayAddress.data() << '\n';
		f << "name="    << app::displayName.data()  << '\n';
		f << "access="  << app::relayAccess.data()  << '\n';
		f << "mode="    << as<int>(app::relayMode)  << '\n';
	}

}
