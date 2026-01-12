module;

#include <iostream>
#include <expected>
#include <optional>
#include <span>
#include <winsock2.h>
#include <ws2tcpip.h>

export module net.winsock;
import appState;
import weretype;

namespace winsock {

	enum class SOCK : int {
		TCP = SOCK_STREAM,		// 1 : stream socket
		UDP = SOCK_DGRAM,			// 2 : datagram socket
		RAW = SOCK_RAW,				// 3 : raw-protocol interface
		RDM = SOCK_RDM, 			// 4 : reliably-delivered message
		SEQ = SOCK_SEQPACKET,	// 5: sequenced packet stream
	};

	export enum class NetError : int {
		AlreadyOperational,
		OperationalFailed,
		NotOperational,
		SocketAlreadyOpen,
		CreationFailed,
		OptionsFailed,
		BindFailed,
		InvalidIP,
		PortUnavailable,
		PermissionDenied,
		RecvFailure,
	};

	export auto ip_str_u32(const std::string& str) -> std::expected<u32, NetError> {
		u32 addr_net{};
		if (InetPtonA(AF_INET, str.c_str(), &addr_net) != 1) {
			return std::unexpected(NetError::InvalidIP);
		}
		return ntohl(addr_net);
	}

	class NetOperational {
	public:
		[[nodiscard]] bool Operational() const { return m_operational; }
		[[nodiscard]] const WSADATA* get() const { return &m_wsaData; }

		auto InitOperational() -> std::expected<void, NetError> {
			if (m_operational) return std::unexpected(NetError::AlreadyOperational);
			if (WSAStartup(0x0202, &m_wsaData) != 0) return std::unexpected(NetError::OperationalFailed);

			return {};
		}
	
	private:
		bool m_operational{false};
		WSADATA m_wsaData;
	};

	export class NetworkState {
	public:
		NetworkState() = default;
		~NetworkState() {
			cleanup();
		}

		NetworkState(const NetworkState&) = delete;
		NetworkState& operator=(const NetworkState&) = delete;
		NetworkState(NetworkState&& other) = delete;
		NetworkState& operator=(const NetworkState&&) = delete;

		void init() {
			auto result = m_operating.InitOperational();
			if (!result) {
				std::cerr << "Failed to create WSA Data! Error Code: " << as<int>(result.error()) << "\n";
			}
		}

		auto OpenNetworkSocket(std::optional<u32> ipaddr, u16 port) noexcept -> std::expected<void, NetError> {

			if (!m_operating.Operational()) return std::unexpected(NetError::NotOperational);
			if (m_openSock != INVALID_SOCKET) return std::unexpected(NetError::SocketAlreadyOpen);

			m_openSock = WSASocketW(AF_INET, SOCK_DGRAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
			if (m_openSock == INVALID_SOCKET) {
				cleanup();
				return std::unexpected(NetError::CreationFailed);
			}

			int enable = 1;
			if (setsockopt(m_openSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(enable)) < 0) {
				cleanup();
				return std::unexpected(NetError::OptionsFailed);
			}

			m_listenAddr.sin_family = AF_INET;
			m_listenAddr.sin_port = htons(port);

			if (ipaddr.has_value()) {
				m_listenAddr.sin_addr.s_addr = htonl(ipaddr.value());
			} else {
				m_listenAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			}

			if (bind(m_openSock, raw<sockaddr*>(&m_listenAddr), sizeof(m_listenAddr)) == SOCKET_ERROR) {
				int error = WSAGetLastError();
				std::wcerr << L"Failed to bind socket. WSA error code: " << error << L"\n";

				if (error == WSAEADDRINUSE) {
					std::cerr << "Port " << port << " is already in use by another application.\n";
				} else if (error == WSAEACCES) {
					cleanup();
					return std::unexpected(NetError::PermissionDenied);
				}
			}
			m_openConnection = true;
			std::cerr << "Listening for Art-Net packets on port " << port << "...\n";
			return{};
		}

		void CloseNetworkSocket() noexcept {
			if (m_openSock != INVALID_SOCKET) {
				if (closesocket(m_openSock) == SOCKET_ERROR) {
					const int error = WSAGetLastError();
					std::cerr << "Failed to close socket. WSA error Code: " << error << "\n";
				} else {
					std::cerr << "Socket Closed Successfully.\n";
				}
				m_openConnection = false;
				m_listenAddr = {};
				m_senderAddr = {};
				m_openSock = INVALID_SOCKET;
			} else {
				std::cerr << "No open socket to close.\n";
			}

			if (WSACleanup() != 0) {
				const int error = WSAGetLastError();
				std::cerr << "WSACleanup failed. WSA error code: " << error << "\n";
			}
		}

		auto RecieveNetPacket() noexcept -> std::expected<std::span<u8>, NetError> {
			std::array<u8, 1024> buffer{};

			int errCode = recvfrom(m_openSock, raw<char*>(buffer.data()), buffer.size(), 0, raw<sockaddr*>(&m_senderAddr), &m_senderAddrSize);
			if (errCode == SOCKET_ERROR) {
				int error = WSAGetLastError();
				std::cerr << "Failed to recieve UDP Packet. Error: " << error << "\n";
				return std::unexpected(NetError::RecvFailure);
			}
			return{};
		}

	private:
		NetOperational m_operating{};
		SOCKET m_openSock{INVALID_SOCKET};
		bool m_openConnection{false};
		
		sockaddr_in m_listenAddr{};
		sockaddr_in m_senderAddr{};
		int m_senderAddrSize = sizeof(m_senderAddr);

		void cleanup() noexcept {
			if (m_openSock != INVALID_SOCKET) {
				closesocket(m_openSock);
				m_openSock = INVALID_SOCKET;
			}

			if (m_operating.Operational()) {
				WSACleanup();
			}

			m_openConnection = false;
		}
	};
}