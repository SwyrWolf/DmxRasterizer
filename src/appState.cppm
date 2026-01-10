module;

#include <string>
#include <optional>
#include <atomic>

export module appState;

export namespace app {
	std::string Version{"0.8.3"};
	std::atomic<bool> running{true};

	bool verticalMode{false};
	bool oscMode{false};
	bool nineMode{false};
	bool debugMode{false};

	int port{6454};
	std::optional<std::string> bindIp;
}