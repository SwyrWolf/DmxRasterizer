module;

#include <winsock2.h>
#include <ws2tcpip.h>

export module net.winsock;
import appState;
import weretype;

export namespace winsock {

	enum class SOCK : int {
		TCP = SOCK_STREAM,		// 1 : stream socket
		UDP = SOCK_DGRAM,			// 2 : datagram socket
		RAW = SOCK_RAW,				// 3 : raw-protocol interface
		RDM = SOCK_RDM, 			// 4 : reliably-delivered message
		SEQ = SOCK_SEQPACKET,	// 5: sequenced packet stream
	};

	WSADATA wsaData;
	sockaddr_in ListenAddr{};
	sockaddr_in SenderAddr{};
	SOCKET openSock = INVALID_SOCKET;

	int SenderAddrSize = sizeof(SenderAddr);

	[[nodiscard]] bool Operational() noexcept {
		if (WSAStartup(0x0202, &wsaData) != 0) {
			return false;
		}
		return true;
	}
}