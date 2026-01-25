module;

#include <expected>
#include <optional>
#include <string>
#include <span>
#include <charconv>

#include <winsock2.h>
#include <ws2tcpip.h>

export module net.winsock;
import weretype;
import fmtwrap;

export namespace winsock {

	constexpr u32 localhost = INADDR_LOOPBACK;
	constexpr u16 artnetport = 6454;

	// Socket types
	enum class SOCK : int {
		TCP = SOCK_STREAM,		// 1 : stream socket
		UDP = SOCK_DGRAM,			// 2 : datagram socket
		RAW = SOCK_RAW,				// 3 : raw-protocol interface
		RDM = SOCK_RDM, 			// 4 : reliably-delivered message
		SEQ = SOCK_SEQPACKET,	// 5 : sequenced packet stream
	};

	struct Endpoint {
		u32 ip{localhost};
		u16 port{artnetport};
		SOCKET Socket{INVALID_SOCKET};
		sockaddr_in ListenAddr{};
		sockaddr_in SenderAddr{};
	};

	////////////////////////////////////////
	WSADATA g_WsaData{};
	bool g_WinsockStatus{false};

	std::optional<Endpoint> m_Connection;
	SOCKET openSock{INVALID_SOCKET};
	sockaddr_in ListenAddr{};
	sockaddr_in SenderAddr{};
	////////////////////////////////////////

	// Global initialized to verify winsock is functioning.
	auto winsockInit() -> std::expected<void, std::string> {
		if (!g_WinsockStatus) {
			if (WSAStartup(0x0202, &g_WsaData) != 0) return std::unexpected("WSA startup failed");
			g_WinsockStatus = true;
		}
		return {};
	}

	// Validate_ipAddr("127.0.0.1") to verify ipv4 address and return as u32
	auto validate_ipAddr(const std::string& strInput)
	-> std::expected<u32, std::string> {
		u32 addr_net{};
		if (InetPtonA(AF_INET, strInput.c_str(), &addr_net) != 1) {
			return std::unexpected("Invalid IP address");
		}
		return ntohl(addr_net);
	}

	auto validate_ipPort(const std::string& strInput) 
	-> std::expected<u16, std::string> {
		int value{};
		auto [ptr, ec] = std::from_chars(strInput.data(), strInput.data() + strInput.size(), value);

		if (ec != std::errc{} || ptr != strInput.data() + strInput.size()) return std::unexpected("invalid numeric port");
		if (value < 1 || value > 0xFFFF)                                   return std::unexpected("port out of range (1-65535)");

		return value;
	}

	auto CreateAddress(const std::string& ipStr, const std::string& portStr) 
	-> std::expected<Endpoint, std::string> {
		auto ip = validate_ipAddr(ipStr);
		if (!ip) return std::unexpected(ip.error());

		auto port = validate_ipPort(portStr);
		if (!port) return std::unexpected(port.error());

		return Endpoint{ *ip, *port };
	}
	auto CreateAddress(const std::string& ipStr, const u16 port) 
	-> std::expected<Endpoint, std::string> {
		auto ip = validate_ipAddr(ipStr);
		if (!ip) return std::unexpected(ip.error());

		return Endpoint{ *ip, port };
	}
	
	auto OpenNetworkSocket(Endpoint&& ep)
	-> std::expected<Endpoint, std::string> {
		if (auto r = winsockInit(); !r) return std::unexpected(r.error());

		ep.Socket = WSASocketW(AF_INET, SOCK_DGRAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
		if (ep.Socket == INVALID_SOCKET) {
			return std::unexpected("Failure to open socket");
		}
		
		int enable = 1;
		if (setsockopt(ep.Socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(enable)) < 0) {
			const int wsa = WSAGetLastError();
			return std::unexpected(fmt::cat("Failure to set socket options! Return: ", wsa));
		}
		ep.ListenAddr.sin_family      = AF_INET;
		ep.ListenAddr.sin_addr.s_addr = htonl(ep.ip);
		ep.ListenAddr.sin_port        = htons(ep.port);
		
		if (bind(ep.Socket, raw<sockaddr*>(&ep.ListenAddr), sizeof(ep.ListenAddr)) == SOCKET_ERROR) {
			int error = WSAGetLastError();
			if (error == WSAEADDRINUSE) {
				return std::unexpected("Port already in use.");
			} else if (error == WSAEACCES) {
				return std::unexpected("Permission denied.");
			}
			return std::unexpected(fmt::cat("Failed to bind socket. Error: ", error));
		}

		return ep;
	}
	
	[[nodiscard]] std::expected<void, std::string>
	CloseNetworkSocket(Endpoint& ep) noexcept {
		if (openSock == INVALID_SOCKET) return std::unexpected("No open socket.");
		if (closesocket(openSock) == SOCKET_ERROR) return std::unexpected("Failed to close socket.");

		m_Connection.reset();
		ListenAddr = {};
		SenderAddr = {};
		openSock = INVALID_SOCKET;
		constexpr int SenderAddrSize = sizeof(SenderAddr);
		return {};
	}

	[[nodiscard]] std::expected<void, std::string>
	RecieveNetPacket(std::span<u8> dst, Endpoint& ep) {
		
		int SenderAddrSize = sizeof(SenderAddr);
		int errCode = recvfrom(ep.Socket, raw<char*>(dst.data()), dst.size(), 0, raw<sockaddr*>(&SenderAddr), &SenderAddrSize);
		if (errCode == SOCKET_ERROR) {
			return std::unexpected("Failed to recieve packet.");
		}

		return {};
	}
}