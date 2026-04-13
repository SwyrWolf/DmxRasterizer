module;

#include <expected>
#include <string>
#include <span>
#include <charconv>
#include <ranges>
#include <bit>
#include <cstring>

#include <winsock2.h>
#include <ws2tcpip.h>

export module net.winsock;
import weretype;
import fmtwrap;

export namespace winsock {

	constexpr u32 LOCALHOST = INADDR_LOOPBACK;
	constexpr u16 ARTNETPORT = 6454;

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
		recieve_SocketClosed,
		recieve_PacketTooLarge,

		connect_HostResolveFail,
		connect_Failure,
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
		u32 ip{LOCALHOST};
		u16 port{ARTNETPORT};
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
		if (Str == "LOCALHOST") return Eval_ipv4(127, 0, 0, 1);
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

			octets[idx++] = as<u8>(val);
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
		
		int enable{1};
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

	// If no error occurs, recvfrom returns the number of bytes received. 
	// If the connection has been gracefully closed, the return value is zero. Otherwise, a value of SOCKET_ERROR
	auto RecieveNetPacket(std::span<u8> dst, Endpoint& ep) 
	-> std::expected<int, Err> {

		int SenderAddrSize = sizeof(ep.SenderAddr);
		int bytes = recvfrom(ep.Socket, raw<char*>(dst.data()), dst.size(), 0, raw<sockaddr*>(&ep.SenderAddr), &SenderAddrSize);
		if (bytes == SOCKET_ERROR) {
			int err = WSAGetLastError();
			if (err == WSAEINTR)    return std::unexpected(Err::recieve_SocketClosed);
			if (err == WSAEMSGSIZE) return std::unexpected(Err::recieve_PacketTooLarge);
			return std::unexpected(Err::recieve_Failure);
		}

		return bytes;
	}

	// Connect a TCP socket to host:port, resolving hostnames via DNS
	auto ConnectTCP(std::string_view host, u16 port) -> std::expected<Endpoint, Err> {
		if (auto r = winsockInit(); !r) return std::unexpected(r.error());

		auto portStr = std::to_string(port);
		std::string hostStr{host};

		addrinfo hints{};
		hints.ai_family   = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		addrinfo* res = nullptr;
		if (getaddrinfo(hostStr.c_str(), portStr.c_str(), &hints, &res) != 0)
			return std::unexpected(Err::connect_HostResolveFail);

		SOCKET sock = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
		if (sock == INVALID_SOCKET) { freeaddrinfo(res); return std::unexpected(Err::socket_OpenFailure); }

		if (::connect(sock, res->ai_addr, static_cast<int>(res->ai_addrlen)) == SOCKET_ERROR) {
			closesocket(sock);
			freeaddrinfo(res);
			return std::unexpected(Err::connect_Failure);
		}

		Endpoint ep{};
		ep.Socket = sock;
		ep.port   = port;
		std::memcpy(&ep.SenderAddr, res->ai_addr, res->ai_addrlen);
		ep.ip = ntohl(ep.SenderAddr.sin_addr.s_addr);
		freeaddrinfo(res);
		return ep;
	}

	// Create an unbound UDP socket with the server address stored in SenderAddr
	auto CreateUDPSocket(std::string_view host, u16 port) -> std::expected<Endpoint, Err> {
		if (auto r = winsockInit(); !r) return std::unexpected(r.error());

		auto portStr = std::to_string(port);
		std::string hostStr{host};

		addrinfo hints{};
		hints.ai_family   = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;

		addrinfo* res = nullptr;
		if (getaddrinfo(hostStr.c_str(), portStr.c_str(), &hints, &res) != 0)
			return std::unexpected(Err::connect_HostResolveFail);

		SOCKET sock = WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED);
		if (sock == INVALID_SOCKET) { freeaddrinfo(res); return std::unexpected(Err::socket_OpenFailure); }

		// Bind to 0.0.0.0:0 so the OS assigns a local port and recvfrom works
		sockaddr_in local{};
		local.sin_family      = AF_INET;
		local.sin_addr.s_addr = INADDR_ANY;
		local.sin_port        = 0;
		if (bind(sock, raw<sockaddr*>(&local), sizeof(local)) == SOCKET_ERROR) {
			closesocket(sock);
			freeaddrinfo(res);
			return std::unexpected(Err::socket_BindingFailure);
		}

		Endpoint ep{};
		ep.Socket = sock;
		ep.port   = port;
		std::memcpy(&ep.SenderAddr, res->ai_addr, res->ai_addrlen);
		ep.ip = ntohl(ep.SenderAddr.sin_addr.s_addr);
		freeaddrinfo(res);
		return ep;
	}
}