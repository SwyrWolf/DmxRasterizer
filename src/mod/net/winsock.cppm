module;

#include <expected>
#include <optional>
#include <string>
#include <span>
#include <charconv>
#include <ranges>
#include <bit>

#include <winsock2.h>
#include <ws2tcpip.h>

export module net.winsock;
import weretype;
import fmtwrap;

export namespace winsock {

	constexpr u32 localhost = INADDR_LOOPBACK;
	constexpr u16 artnetport = 6454;

	enum class Err {
		WSA_StartupFailure,

		ipv4_TooManyOctets,
		ipv4_TooLittleOctets,
		ipv4_InvalidOctetLength,
		ipv4_InvalidOctetLeadingZero,
		ipv4_InvalidOctetValue,
		ipv4_InvalidOctetRange,

		port_Invalid,
		port_InvalidRange,

		socket_OpenFailure,
		socket_CloseFailure,
		socket_OptionsFailure,
		socket_PortUsed,
		socket_PermissionDenied,
		socket_BindingFailure,
		socket_NoOpenSocket,

		recieve_Failure,
	};

	constexpr auto hostEndian(u32&& val) -> u32 {
		if (std::endian::native == std::endian::little) return std::byteswap(val);
		return val;
	}

	consteval auto Eval_ipv4(u8 a, u8 b, u8 c, u8 d) -> u32 {
		std::array<u8, 4> bytes{a, b, c, d};
		return hostEndian(std::bit_cast<u32>(bytes));
	}

	// Socket types
	enum class SOCK : int {
		TCP = SOCK_STREAM,		// 1 : stream socket
		UDP = SOCK_DGRAM,			// 2 : datagram socket
		RAW = SOCK_RAW,				// 3 : raw-protocol interface
		RDM = SOCK_RDM, 			// 4 : reliably-delivered message
		SEQ = SOCK_SEQPACKET,	// 5 : sequenced packet stream
	};

	////////////////////////////////////////
	struct Endpoint {
		u32 ip{localhost};
		u16 port{artnetport};
		SOCKET Socket{INVALID_SOCKET};
		sockaddr_in ListenAddr{};
		sockaddr_in SenderAddr{};
	};


	WSADATA g_WsaData{};
	bool g_WinsockStatus{false};
	////////////////////////////////////////

	// Global initialized to verify winsock is functioning.
	auto winsockInit() -> std::expected<void, Err> {
		if (!g_WinsockStatus) {
			if (WSAStartup(0x0202, &g_WsaData) != 0) return std::unexpected(Err::WSA_StartupFailure);
			g_WinsockStatus = true;
		}
		return {};
	}

	// Validate_ipAddr("127.0.0.1") to verify ipv4 address and return as u32
	auto validate_ipAddr(std::string_view Str) -> std::expected<u32, Err> {
		if (Str == "localhost") return Eval_ipv4(127, 0, 0, 1);
		if (Str == "any")       return as<u32>(0);

		std::array<u8, 4> octets{};
		size_t idx{0};

		for (auto part : Str | std::views::split('.')) {
			if (idx >= 4) return std::unexpected(Err::ipv4_TooManyOctets);

			auto sv = std::string_view(part.begin(), part.end());
			if (sv.empty() || sv.size() > 3) return std::unexpected(Err::ipv4_InvalidOctetLength);
			if (sv.size() > 1 && sv[0] == '0') return std::unexpected(Err::ipv4_InvalidOctetLeadingZero);

			u32 val{};
			auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), val);

			if (ec != std::errc{} || ptr != sv.data() + sv.size()) return std::unexpected(Err::ipv4_InvalidOctetValue);
			if (val > 255) return std::unexpected(Err::ipv4_InvalidOctetRange);

			octets[idx++] = static_cast<u8>(val);
		}

		if (idx != 4) return std::unexpected(Err::ipv4_TooLittleOctets);

		return hostEndian(std::bit_cast<u32>(octets));
	}

	auto validate_ipPort(const std::string& strInput) -> std::expected<u16, Err> {
		int value{};
		auto [ptr, ec] = std::from_chars(strInput.data(), strInput.data() + strInput.size(), value);

		if (ec != std::errc{} || ptr != strInput.data() + strInput.size()) return std::unexpected(Err::port_Invalid);
		if (value < 1 || value > 0xFFFF)                                   return std::unexpected(Err::port_InvalidRange);

		return value;
	}

	auto CreateAddress(std::string_view ipStr, const std::string& portStr) 
	-> std::expected<Endpoint, Err> {
		auto ip = validate_ipAddr(ipStr);
		if (!ip) return std::unexpected(ip.error());

		auto port = validate_ipPort(portStr);
		if (!port) return std::unexpected(port.error());

		return Endpoint{ *ip, *port };
	}
	auto CreateAddress(const std::string& ipStr, const u16 port) 
	-> std::expected<Endpoint, Err> {
		auto ip = validate_ipAddr(ipStr);
		if (!ip) return std::unexpected(ip.error());

		return Endpoint{ *ip, port };
	}
	
	auto OpenNetworkSocket(Endpoint&& ep) -> std::expected<Endpoint, Err> {
		if (auto r = winsockInit(); !r) return std::unexpected(r.error());

		ep.Socket = WSASocketW(AF_INET, SOCK_DGRAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
		if (ep.Socket == INVALID_SOCKET) {
			return std::unexpected(Err::socket_OpenFailure);
		}
		
		int enable = 1;
		if (setsockopt(ep.Socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(enable)) < 0) {
			const int wsa = WSAGetLastError();
			return std::unexpected(Err::socket_OptionsFailure);
		}
		ep.ListenAddr.sin_family      = AF_INET;
		ep.ListenAddr.sin_addr.s_addr = htonl(ep.ip);
		ep.ListenAddr.sin_port        = htons(ep.port);
		
		if (bind(ep.Socket, raw<sockaddr*>(&ep.ListenAddr), sizeof(ep.ListenAddr)) == SOCKET_ERROR) {
			int error = WSAGetLastError();
			if (error == WSAEADDRINUSE) {
				return std::unexpected(Err::socket_PortUsed);
			} else if (error == WSAEACCES) {
				return std::unexpected(Err::socket_PermissionDenied);
			}
			return std::unexpected(Err::socket_BindingFailure);
		}

		return ep;
	}
	
	auto CloseNetworkSocket(Endpoint& ep) -> std::expected<void, Err> {
		if (ep.Socket == INVALID_SOCKET) return std::unexpected(Err::socket_NoOpenSocket);
		if (closesocket(ep.Socket) == SOCKET_ERROR) return std::unexpected(Err::socket_CloseFailure);

		return {};
	}

	auto RecieveNetPacket(std::span<u8> dst, Endpoint& ep) -> std::expected<void, Err> {
		
		int SenderAddrSize = sizeof(ep.SenderAddr);
		int errCode = recvfrom(ep.Socket, raw<char*>(dst.data()), dst.size(), 0, raw<sockaddr*>(&ep.SenderAddr), &SenderAddrSize);
		if (errCode == SOCKET_ERROR) {
			return std::unexpected(Err::recieve_Failure);
		}

		return {};
	}
}