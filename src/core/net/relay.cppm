module;

#include <array>
#include <charconv>
#include <string>
#include <string_view>
#include <expected>
#include <thread>
#include <vector>
#include <span>
#include <cstring>
#include <print>

#include <winsock2.h>

export module net.relay;
import weretype;
import net.winsock;
import appState;
import render;

static std::jthread g_tcpThread;
static std::jthread g_udpThread;

namespace relay_detail {
	bool sendAll(SOCKET s, const void* data, int len) {
		const char* p = as<const char*>(data);
		while (len > 0) {
			int sent = ::send(s, p, len, 0);
			if (sent == SOCKET_ERROR) return false;
			p   += sent;
			len -= sent;
		}
		return true;
	}
	bool recvAll(SOCKET s, void* data, int len) {
		char* p = as<char*>(data);
		while (len > 0) {
			int got = ::recv(s, p, len, 0);
			if (got <= 0) return false;
			p   += got;
			len -= got;
		}
		return true;
	}
	// Forward declarations — defined after export namespace relay
	void tcpLoop(std::stop_token st, SOCKET sock);
	void udpLoop(std::stop_token st, SOCKET sock, sockaddr_in serverAddr);
}

export namespace relay {

	// --- Signatures ---
	constexpr std::array<u8, 8> TCP_SIGNATURE = { 'd','m','x','r','e','l','a','y' };
	constexpr std::array<u8, 3> UDP_SIGNATURE = { 'd','m','x' };
	constexpr u16 VERSION = 1;

	// --- Packet size constants ---
	constexpr u64 DMX_SIZE                  = 512;
	constexpr u64 UDP_TOKEN_SIZE             = 32;
	constexpr u64 UDP_DMX_PACKET_SIZE        = 553;
	constexpr u64 UDP_DMX_FORWARDED_SIZE     = 518;
	constexpr u64 UDP_HEARTBEAT_SERVER_SIZE  = 4;
	constexpr u64 UDP_HEARTBEAT_CLIENT_SIZE  = 40;

	// --- Packet type constants ---
	enum class PacketType : u8 {
		Handshake        = 0x01,
		HandshakeAck     = 0x02,
		DMX              = 0x03,
		Heartbeat        = 0x04,
		HeartbeatAck     = 0x05,
		RegisterListener = 0x06,
	};

	#pragma pack(push, 1)

	// TCP relay header (15 bytes) — all TCP packets
	// Signature(8) | Version(2, BE) | Type(1) | BodyLen(4, BE)
	struct TCPHeader {
		std::array<u8, 8> signature; // ASCII "dmxrelay"
		u16 version;                 // big-endian; currently 1
		u8  type;                    // PacketType
		u32 bodyLen;                 // big-endian; length of body following this header
	};
	static_assert(sizeof(TCPHeader) == 15);

	// Handshake body (0x01) — Client → Server (variable length)
	// Wire layout: NameLen(u16, BE) | Name(NameLen bytes) | AccessLen(u16, BE) | Access(AccessLen bytes)
	struct HandshakePrefix {
		u16 nameLen; // big-endian; bytes of Name that follow
		// Name   (nameLen bytes)
		// AccessLen (u16 BE)
		// Access (AccessLen bytes)
	};

	// HandshakeAck body (0x02) — Server → Client (variable length)
	// Wire layout: SessionID(u32, BE) | TokenLen(u16, BE) | Token(TokenLen bytes, 32-byte hex)
	struct HandshakeAckPrefix {
		u32 sessionID; // big-endian
		u16 tokenLen;  // big-endian; bytes of Token that follow (always 32)
		// Token (tokenLen bytes, ASCII hex-encoded UDP token)
	};

	// DMX body (0x03) — Server → Client (514 bytes)
	struct DMXBody {
		u16                  universe; // big-endian
		std::array<u8, 512>  data;
	};
	static_assert(sizeof(DMXBody) == 514);

	// Heartbeat (0x04) / HeartbeatAck (0x05) — header only, BodyLen = 0


	// UDP DMX data packet (553 bytes) — Client → Server
	struct UDPDMXPacket {
		std::array<u8, 3>   header;    // ASCII "dmx"
		u32                 sessionID; // big-endian
		std::array<u8, 32>  token;     // ASCII hex UDP token
		u16                 universe;  // big-endian
		std::array<u8, 512> data;
	};
	static_assert(sizeof(UDPDMXPacket) == 553);

	// UDP heartbeat / heartbeat ack — Server → Client (4 bytes)
	struct UDPHeartbeatServer {
		std::array<u8, 3> header; // ASCII "dmx"
		u8                type;   // PacketType (0x04 or 0x05)
	};
	static_assert(sizeof(UDPHeartbeatServer) == 4);

	// UDP heartbeat / heartbeat ack — Client → Server (40 bytes)
	struct UDPHeartbeatClient {
		std::array<u8, 3>  header;    // ASCII "dmx"
		u8                 type;      // PacketType (0x04 or 0x05)
		u32                sessionID; // big-endian
		std::array<u8, 32> token;     // ASCII hex UDP token
	};
	static_assert(sizeof(UDPHeartbeatClient) == 40);

	#pragma pack(pop)

	auto ParseAddress(std::string_view addr)
	-> std::expected<std::pair<std::string, u16>, const char*> {
		auto colon = addr.rfind(':');
		if (colon == std::string_view::npos) return std::unexpected("Missing port (expected host:port)");
		auto host   = addr.substr(0, colon);
		auto portSv = addr.substr(colon + 1);
		if (host.empty()) return std::unexpected("Missing host");
		u32 portVal{};
		auto [ptr, ec] = std::from_chars(portSv.data(), portSv.data() + portSv.size(), portVal);
		if (ec != std::errc{} || portVal == 0 || portVal > 0xFFFF)
			return std::unexpected("Invalid port");
		return std::pair{ std::string(host), as<u16>(portVal) };
	}

	void Connect() {
		std::string_view addrField{ app::relayAddress.data() };
		auto parsed = ParseAddress(addrField);
		if (!parsed) { app::relayStatus = parsed.error(); return; }
		auto [host, port] = *parsed;

		app::relayStatus = "Connecting...";

		// Close existing sockets first (unblocks any blocking recv in old threads)
		if (app::RelayTCP) { closesocket(app::RelayTCP->Socket); app::RelayTCP->Socket = INVALID_SOCKET; }
		if (app::RelayUDP) { closesocket(app::RelayUDP->Socket); app::RelayUDP->Socket = INVALID_SOCKET; }
		// Join old threads (they'll exit because their socket is now closed)
		g_tcpThread = {};
		g_udpThread = {};
		app::RelayTCP.reset();
		app::RelayUDP.reset();

		auto tcp = winsock::ConnectTCP(host, port);
		if (!tcp) { app::relayStatus = "TCP connect failed"; return; }

		// --- Build and send Handshake ---
		std::string_view name  { app::displayName.data() };
		std::string_view access{ app::relayAccess.data() };
		u16 nameLen   = as<u16>(name.size());
		u16 accessLen = as<u16>(access.size());
		u32 bodyLen   = 2u + nameLen + 2u + accessLen;

		TCPHeader hdr{};
		hdr.signature = TCP_SIGNATURE;
		hdr.version   = htons(VERSION);
		hdr.type      = as<u8>(PacketType::Handshake);
		hdr.bodyLen   = htonl(bodyLen);
		u16 nameLenBE   = htons(nameLen);
		u16 accessLenBE = htons(accessLen);

		SOCKET sock = tcp->Socket;
		if (   !relay_detail::sendAll(sock, &hdr,        sizeof(hdr))
			|| !relay_detail::sendAll(sock, &nameLenBE,   2)
			|| !relay_detail::sendAll(sock, name.data(),  as<int>(nameLen))
			|| !relay_detail::sendAll(sock, &accessLenBE, 2)
			|| !relay_detail::sendAll(sock, access.data(),as<int>(accessLen))) {
			(void)winsock::CloseNetworkSocket(*tcp);
			app::relayStatus = "Handshake send failed";
			return;
		}

		// --- Read HandshakeAck TCP header ---
		TCPHeader ackHdr{};
		if (!relay_detail::recvAll(sock, &ackHdr, sizeof(ackHdr))) {
			(void)winsock::CloseNetworkSocket(*tcp);
			app::relayStatus = "Handshake: no response";
			return;
		}
		if (ackHdr.signature != TCP_SIGNATURE || ackHdr.type != as<u8>(PacketType::HandshakeAck)) {
			(void)winsock::CloseNetworkSocket(*tcp);
			app::relayStatus = "Handshake: bad response";
			return;
		}

		// --- Read HandshakeAck body: SessionID + TokenLen + Token ---
		HandshakeAckPrefix ackPrefix{};
		if (!relay_detail::recvAll(sock, &ackPrefix, sizeof(ackPrefix))) {
			(void)winsock::CloseNetworkSocket(*tcp);
			app::relayStatus = "Handshake: ack body failed";
			return;
		}
		u16 tokenLen = ntohs(ackPrefix.tokenLen);
		if (tokenLen > as<u16>(app::relayToken.size())) {
			(void)winsock::CloseNetworkSocket(*tcp);
			app::relayStatus = "Handshake: token too large";
			return;
		}
		if (!relay_detail::recvAll(sock, app::relayToken.data(), tokenLen)) {
			(void)winsock::CloseNetworkSocket(*tcp);
			app::relayStatus = "Handshake: token read failed";
			return;
		}
		app::relaySessionID = ntohl(ackPrefix.sessionID);

		// --- Open UDP socket ---
		auto udp = winsock::CreateUDPSocket(host, port);
		if (!udp) {
			(void)winsock::CloseNetworkSocket(*tcp);
			app::relayStatus = "UDP socket failed";
			return;
		}

		app::RelayTCP    = std::move(*tcp);
		app::RelayUDP    = std::move(*udp);

		// If in Listen mode, register this session as a listener over TCP
		if (app::relayMode == app::RelayMode::Listen) {
			TCPHeader regHdr{};
			regHdr.signature = TCP_SIGNATURE;
			regHdr.version   = htons(VERSION);
			regHdr.type      = as<u8>(PacketType::RegisterListener);
			regHdr.bodyLen   = 0;
			if (!relay_detail::sendAll(app::RelayTCP->Socket, &regHdr, sizeof(regHdr))) {
				app::relayStatus = "RegisterListener failed";
				return;
			}
		}

		// Send an initial UDP heartbeat so the server learns our UDP endpoint
		{
			UDPHeartbeatClient reg{};
			reg.header    = UDP_SIGNATURE;
			reg.type      = static_cast<u8>(PacketType::Heartbeat);
			reg.sessionID = htonl(app::relaySessionID);
			reg.token     = app::relayToken;
			(void)sendto(app::RelayUDP->Socket,
			            reinterpret_cast<const char*>(&reg), sizeof(reg), 0,
			            reinterpret_cast<sockaddr*>(&app::RelayUDP->SenderAddr),
			            sizeof(app::RelayUDP->SenderAddr));
		}

		// Start TCP and UDP receive loops
		SOCKET tcpSock = app::RelayTCP->Socket;
		SOCKET udpSock = app::RelayUDP->Socket;
		sockaddr_in udpServer = app::RelayUDP->SenderAddr;
		g_tcpThread = std::jthread(relay_detail::tcpLoop, tcpSock);
		g_udpThread = std::jthread(relay_detail::udpLoop, udpSock, udpServer);

		app::relayStatus = "Connected";
	}

	void Disconnect() {
		if (app::RelayTCP) { closesocket(app::RelayTCP->Socket); app::RelayTCP->Socket = INVALID_SOCKET; }
		if (app::RelayUDP) { closesocket(app::RelayUDP->Socket); app::RelayUDP->Socket = INVALID_SOCKET; }
		g_tcpThread = {};
		g_udpThread = {};
		app::RelayTCP.reset();
		app::RelayUDP.reset();
		app::RelaySend    = false;
		app::relayStatus  = "Not connected";
	}

	void SendDmx(u16 universe, std::span<const u8> data) {
		if (!app::RelayUDP) return;

		UDPDMXPacket pkt{};
		pkt.header    = UDP_SIGNATURE;
		pkt.sessionID = htonl(app::relaySessionID);
		pkt.token     = app::relayToken;
		pkt.universe  = htons(universe);
		const std::size_t copy = data.size() < 512 ? data.size() : 512;
		std::copy_n(data.data(), copy, pkt.data.data());

		(void)sendto(app::RelayUDP->Socket,
		            reinterpret_cast<const char*>(&pkt), sizeof(pkt), 0,
		            reinterpret_cast<sockaddr*>(&app::RelayUDP->SenderAddr),
		            sizeof(app::RelayUDP->SenderAddr));
	}
}

namespace relay_detail {
	// Reads incoming TCP packets; acks heartbeats; exits on socket error or stop
	void tcpLoop(std::stop_token st, SOCKET sock) {
		while (!st.stop_requested()) {
			relay::TCPHeader hdr{};
			if (!recvAll(sock, &hdr, sizeof(hdr))) break;
			auto type    = as<relay::PacketType>(hdr.type);
			u32  bodyLen = ntohl(hdr.bodyLen);
			if (type == relay::PacketType::Heartbeat) {
				relay::TCPHeader ack{};
				ack.signature = relay::TCP_SIGNATURE;
				ack.version   = htons(relay::VERSION);
				ack.type      = as<u8>(relay::PacketType::HeartbeatAck);
				ack.bodyLen   = 0;
				if (!sendAll(sock, &ack, sizeof(ack))) break;
			} else if (bodyLen > 0) {
				std::vector<u8> drain(bodyLen);
				if (!recvAll(sock, drain.data(), as<int>(bodyLen))) break;
			}
		}
		if (!st.stop_requested())
			app::relayStatus = "Disconnected";
	}
	// Listens for UDP packets from the server; acks UDP heartbeats, writes DMX in listen mode
	void udpLoop(std::stop_token st, SOCKET sock, sockaddr_in serverAddr) {
		// Buffer sized for the larger of the two inbound packet types
		std::array<u8, relay::UDP_DMX_PACKET_SIZE> buf{};
		int addrLen = sizeof(serverAddr);
		while (!st.stop_requested()) {
			sockaddr_in from{};
			int fromLen = sizeof(from);
			int got = recvfrom(sock, reinterpret_cast<char*>(buf.data()), as<int>(buf.size()), 0,
			                   reinterpret_cast<sockaddr*>(&from), &fromLen);
			if (got == SOCKET_ERROR) break;
			if (got < 4 || buf[0]!='d' || buf[1]!='m' || buf[2]!='x') continue;

			const auto pktType = as<relay::PacketType>(buf[3]);

			if (pktType == relay::PacketType::Heartbeat) {
				relay::UDPHeartbeatClient ack{};
				ack.header    = relay::UDP_SIGNATURE;
				ack.type      = as<u8>(relay::PacketType::HeartbeatAck);
				ack.sessionID = htonl(app::relaySessionID);
				ack.token     = app::relayToken;
				(void)sendto(sock, reinterpret_cast<const char*>(&ack), sizeof(ack), 0,
				            reinterpret_cast<sockaddr*>(&serverAddr), addrLen);
			} else if (pktType == relay::PacketType::DMX
			        && got == as<int>(relay::UDP_DMX_FORWARDED_SIZE)
			        && app::relayMode == app::RelayMode::Listen) {
				// Forwarded DMX: [0:3]=dmx [3]=0x03 [4:6]=universe(BE) [6:518]=data
				u16 universe;
				std::memcpy(&universe, buf.data() + 4, 2);
				universe = ntohs(universe);
				std::println("Relay UDP DMX: universe={}", universe);
				constexpr std::size_t uniStride = 512 + 8;
				std::size_t offset = universe * uniStride;
				if (offset + 512 <= Render::DmxTexture.DmxData.size()) {
					std::memcpy(Render::DmxTexture.DmxData.data() + offset, buf.data() + 6, 512);
					app::times.signalRender();
				}
			}
		}
	}
}