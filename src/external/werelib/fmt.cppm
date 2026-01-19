module;

#include <sstream>
#include <string>
#include <utility>

export module fmtwrap;

export namespace fmt {
	template <typename... Args>
	std::string cat(Args&&... args) {
		std::ostringstream oss;
		(oss << ... << std::forward<Args>(args));
		return oss.str();
	}

	template <typename... Args>
	std::wstring wcat(Args&&... args) {
		std::wostringstream woss;
		(woss << ... << std::forward<Args>(args));
		return woss.str();
	}
}