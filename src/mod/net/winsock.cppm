module;

#include <expected>
#include <optional>
#include <string>

#include <winsock2.h>
#include <ws2tcpip.h>

export module net.winsock;
import weretype;
import fmtwrap;
import appState;

export namespace winsock {

	constexpr u32 localhost = INADDR_LOOPBACK;
	constexpr u16 artnetport = 6454;

	bool g_WinsockStatus{false};
	WSADATA g_WsaData{};

	// Socket types
	enum class SOCK : int {
		TCP = SOCK_STREAM,		// 1 : stream socket
		UDP = SOCK_DGRAM,			// 2 : datagram socket
		RAW = SOCK_RAW,				// 3 : raw-protocol interface
		RDM = SOCK_RDM, 			// 4 : reliably-delivered message
		SEQ = SOCK_SEQPACKET,	// 5 : sequenced packet stream
	};

	struct AddressV4 {
		u32 ip{localhost};
		u16 port{artnetport};
	};

	// Verify and convert string -> ipv4 address.
	[[nodiscard]] std::expected<u32, std::string>
	ipv4_fromString(const std::string& strInput) noexcept {
		u32 addr_net{};
		if (InetPtonA(AF_INET, strInput.c_str(), &addr_net) != 1) {
			return std::unexpected("Invalid IP address");
		}
		return ntohl(addr_net);
	}

	[[nodiscard]] std::expected<void, std::string>
	Operational() {
		if (!g_WinsockStatus) {
			if (WSAStartup(0x0202, &g_WsaData) != 0) {
				return std::unexpected("WSA startup failed");
			}
			g_WinsockStatus = true;
		}
		return {};
	}

	class Net {
		std::optional<AddressV4> m_Connection;
		SOCKET openSock{INVALID_SOCKET};
		sockaddr_in ListenAddr{};
		sockaddr_in SenderAddr{};

		public:
		[[nodiscard]] std::expected<void, std::string>
		OpenNetworkSocket(const AddressV4& ipv4) {
			if (auto r = Operational(); !r) return std::unexpected(r.error());

			openSock = socket(AF_INET, SOCK_DGRAM, 0);
			if (openSock == INVALID_SOCKET) {
				return std::unexpected("Failure to open socket");
			}

			
			int enable = 1;
			if (setsockopt(openSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(enable)) < 0) {
				const int wsa = WSAGetLastError();
				return std::unexpected(fmt::cat("Failure to set socket options! Return: ", wsa));
			}
			ListenAddr.sin_family = AF_INET;
			ListenAddr.sin_port = htons(ipv4.port);
			if (inet_pton(AF_INET, app::ipStr.data(), &ListenAddr.sin_addr) != 1) {
				return std::unexpected("Invalid ipv4 Address");
			}
			
			if (bind(openSock, raw<sockaddr*>(&ListenAddr), sizeof(ListenAddr)) == SOCKET_ERROR) {
				int error = WSAGetLastError();
				if (error == WSAEADDRINUSE) {
					return std::unexpected("Port already in use.");
				} else if (error == WSAEACCES) {
					return std::unexpected("Permission denied.");
				}
				return std::unexpected("Failed to bind socket.");
			}
			
			m_Connection.emplace(ipv4);
			return{};
		}
		
		[[nodiscard]] std::expected<void, std::string>
		CloseNetworkSocket() noexcept {
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
		RecieveNetPacket() noexcept {
			std::array<u8, 1024> buffer{};
			
			int SenderAddrSize = sizeof(SenderAddr);
			int errCode = recvfrom(openSock, raw<char*>(buffer.data()), buffer.size(), 0, raw<sockaddr*>(&SenderAddr), &SenderAddrSize);
			if (errCode == SOCKET_ERROR) {
				return std::unexpected("Failed to recieve packet.");
			}

			return {};
		}
	};
}