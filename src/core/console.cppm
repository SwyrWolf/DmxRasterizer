module;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <io.h>
#include <fcntl.h>

#include <string>
#include <print>
#include <iostream>
#include <expected>

export module console;

bool g_consoleAllocated{false};
bool g_consoleVisible{false};

export namespace console {
	auto ConsoleAlloc() -> std::expected<void, std::string> {
		if (g_consoleAllocated) return std::unexpected("Console Allocated");
		
		if (GetConsoleWindow() != nullptr) {
			g_consoleAllocated = true;
			return std::unexpected("Console already attached");
		}
		
		if (!AllocConsole()) {
			DWORD err = GetLastError();
			return std::unexpected(std::format("Alloc Console Failed: {}", err));
		}

		FILE* fp = nullptr;
		freopen_s(&fp, "CONOUT$", "w", stdout);
		freopen_s(&fp, "CONOUT$", "w", stderr);
		freopen_s(&fp, "CONIN$",  "r", stdin);

		std::ios::sync_with_stdio(true);
		g_consoleAllocated = true;

		return{};
	}

	void ConsoleSet(bool visible) {
		if (visible && !g_consoleAllocated) {
			if (auto r = ConsoleAlloc(); !r) {
				std::println("Error: {}", r.error());
			}
		}

		HWND hwnd = GetConsoleWindow();
		if (!hwnd) return;

		ShowWindow(hwnd, visible ? SW_SHOW : SW_HIDE);
		g_consoleVisible = visible;
	}

	void ConsoleToggle() { 
		ConsoleSet(!g_consoleVisible); 
		std::println("Toggling console: {}", g_consoleVisible);
	}
}