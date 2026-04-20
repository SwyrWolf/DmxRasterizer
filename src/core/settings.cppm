module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include <objbase.h>

#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <charconv>
#include <filesystem>

export module settings;
import appState;
import net.relay;
import weretype;

constexpr auto dir_AppDataRoaming() -> std::filesystem::path {
	PWSTR path = nullptr;
	if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path))) {
		return {};
	}
	
	std::filesystem::path result = path;
	CoTaskMemFree(path);
	return result;
}

template<std::size_t N>
void fill(std::array<char, N>& dst, std::string_view src) {
	auto len = std::min(src.size(), N - 1);
	src.copy(dst.data(), len);
	dst[len] = '\0';
}

export namespace settings {
	constexpr std::string_view FILE_NAME = "dmxr.cfg";
	const auto fileDestination = dir_AppDataRoaming() / "SwyrWolf" / FILE_NAME;

	void Load() {
		std::ifstream f{ fileDestination };
		if (!f.is_open()) return;

		std::string line;
		while (std::getline(f, line)) {
			if (line.empty() || line[0] == '#') continue;
			auto eq = line.find('=');
			if (eq == std::string::npos) continue;
			std::string_view key{ line.data(), eq };
			std::string_view val{ line.data() + eq + 1, line.size() - eq - 1 };

			if (key == "address") fill(relay::address, val);
			else if (key == "name")    fill(relay::displayName, val);
			else if (key == "access")  fill(relay::access, val);
			else if (key == "mode") {
				int m{};
				std::from_chars(val.data(), val.data() + val.size(), m);
				relay::mode = as<relay::Mode>(m);
			}
		}
	}

	void Save() {
		std::filesystem::create_directories(fileDestination.parent_path());
		std::ofstream f{ fileDestination, std::ios::trunc };

		if (!f.is_open()) return;

		f << "address=" << relay::address.data() << '\n';
		f << "name="    << relay::displayName.data()  << '\n';
		f << "access="  << relay::access.data()  << '\n';
		f << "mode="    << as<int>(relay::mode)  << '\n';
	}

}
