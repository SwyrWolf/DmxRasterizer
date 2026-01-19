module;

#include <string>
#include <optional>
#include <atomic>
#include <ranges>
#include <chrono>

#include <glfw3.h>

export module appState;

export namespace app {
	std::string Version{"0.8.3"};
	std::atomic<bool> running{true};

	GLFWwindow* SpoutWindow{nullptr};
	GLFWwindow* GuiWindow{nullptr};

	bool verticalMode{false};
	bool oscMode{false};
	bool nineMode{false};
	bool debugMode{false};

	bool RGBmode{false};
	bool Unicast{false};
	bool OpenConnection{false};
	bool VsyncEnabled{true};

	std::array<char,16> ipStr = {"127.0.0.1"};
	int ipPort{6454};
	std::optional<std::string> bindIp;

	int FrameRateSel = 3;
	using namespace std::chrono_literals;

	struct FpsEntry {
		const char* label;
		std::chrono::duration<double, std::milli> ms;
	};

	constexpr auto FPS_ITEMS = std::to_array<FpsEntry>({
		{"30", 1s / 30.0},
		{"60", 1s / 60.0},
		{"120", 1s / 120.0},
		{"144", 1s / 144.0},
		{"165", 1s / 165.0},
		{"200", 1s / 200.0},
		{"244", 1s / 244.0},
		{"300", 1s / 300.0},
	});

	constexpr auto FPS_LABELS_VIEW = FPS_ITEMS | std::views::transform([](auto&& e) {
		return e.label;
	});

	template <typename T, std::size_t N, typename F>
	constexpr auto to_array(const std::array<T, N>& src, F&& transform) {
		std::array<std::invoke_result_t<F, T>, N> out{};
		for (std::size_t i = 0; i < N; ++i)
			out[i] = transform(src[i]);
		return out;
	}

	constexpr auto FPS_LABELS = to_array(FPS_ITEMS, [](const FpsEntry& e) {
		return e.label;
	});
}