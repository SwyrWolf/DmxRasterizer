module;

#include <string>
#include <print>

export module net.control;
import weretype;
import appState;
import net.winsock;

export auto StartNetwork() -> bool {
	if (!app::netManager) {
		app::Debug = L"NetManager not initialized.";
		return false;
	}
	if (app::NetConnection) {
		app::Debug = L"Network already running.";
		return true;
	}

	auto ipStr = app::ipString();
	auto addr = winsock::CreateAddress(ipStr, as<u16>(app::ipPort))
		.and_then(winsock::OpenNetworkSocket);

	if (!addr) {
		std::println(stderr, "Winsock CreateAddr Err: 0x{:x}", as<int>(addr.error()));
		app::Debug = L"Failed to open network socket.";
		return false;
	}

	app::NetConnection.emplace(std::move(*addr));

#ifdef _DEBUG
	std::println(stderr, "IP Address: 0x{:x}", app::NetConnection->ip);
	std::println(stderr, "IP Port: 0x{:x}", app::NetConnection->port);
#endif

	app::netManager->start(app::NetConnection);

	app::Debug = std::format(
		L"Listening for Art-Net on [{}:{}]",
		std::wstring(ipStr.begin(), ipStr.end()),
		app::NetConnection->port
	);

	return true;
}
