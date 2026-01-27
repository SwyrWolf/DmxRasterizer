// Cedit to the Protocol Standard: "Art-Net™ Designed by and Copyright Artistic Licence". View official Art-Net™ specs via https://art-net.org.uk/downloads/art-net.pdf

module;

#include <cstring>
#include <array>
#include <string>
#include <expected>
#include <span>
#include <algorithm>
#include <iostream>

export module net.artnet;
import weretype;

export namespace artnet {
	constexpr std::array<u8, 8> ARTNET_SIGNATURE = {'A','r','t','-','N','e','t',0x00}; //Art-Net [0-8] Fixed first 8 bytes of an art-net m_packet
	constexpr u16 ARTNET_VERSION = 0x000E; // Fixed version number 1.4 or "14" [u8:ProtVerHi & u8:ProtVerLo]
	constexpr u16 ARTNET_PORT = 0x1936; // AKA :6454

	// --- Art-Net operation codes ---
	enum class Op : u16 {
		Poll                = 0x2000, // ArtPoll m_packet, no other data is contained in this UDP m_packet.
		PollReply           = 0x2100, // ArtPollReply Packet. It contains device status information.
		DiagData            = 0x2300, // Diagnostics and data logging m_packet.
		Command             = 0x2400, // ArtCommand m_packet. It is used to send text based parameter commands.
		DataRequest         = 0x2700, // ArtDataRequest m_packet. It is used to request data such as products URLs.
		DataReply           = 0x2800, // his is an ArtDataReply m_packet. It is used to reply to ArtDataRequest m_packets.
		Dmx                 = 0x5000, // ArtDmx data m_packet. It contains zero start code DMX512 information for a single Universe.
		Nzs                 = 0x5100, // ArtNzs data m_packet. It contains non-zero start code (except RDM) DMX512 information for a single Universe.
		Sync                = 0x5200, // ArtSync data m_packet. It is used to force synchronous transfer of ArtDmx m_packets to a node’s temp_BinaryPacketput.
		Address             = 0x6000, // ArtAddress m_packet. It contains remote programming information for a Node.
		Input               = 0x7000, // ArtInput m2_packet. It contains enable – disable data for DMX inputs.
		TodRequest          = 0x8000, // ArtTodRequest m_packet. It is used to request a Table of Devices (ToD) for RDM discovery.
		TodData             = 0x8100, // ArtTodData m_packet. It is used to send a Table of Devices (ToD) for RDM discovery.
		TodControl          = 0x8200, // ArtTodControl m_packet. It is used to send RDM discovery control messages.
		Rdm                 = 0x8300, // ArtRdm m_packet. It is used to send all non discovery RDM messages.
		RdmSub              = 0x8400, // ArtRdmSub m_packet. It is used to send compressed, RDM Sub-Device data.
		VideoSetup          = 0xa010, // ArtVideoSetup m_packet. It contains video screen setup information for nodes that implement the extended video features.
		VideoPalette        = 0xa020, // ArtVideoPalette m_packet. It contains colour palette setup information for nodes that implement the extended video features.
		VideoData           = 0xa040, // ArtVideoData m_packet. It contains display data for nodes that implement the extended video features.
		MacMaster           = 0xf000, // This m_packet is deprecated.
		MacSlave            = 0xf100, // This m_packet is deprecated.
		FirmwareMaster      = 0xf200, // ArtFirmwareMaster m_packet. It is used to upload new firmware or firmware extensions to the Node.
		FirmwareReply       = 0xf300, // ArtFirmwareReply m_packet. It is returned by the node to acknowledge receipt of an ArtFirmwareMaster m_packet or ArtFileTnMaster m_packet.
		FileTnMaster        = 0xf400, // Uploads user file to node.
		FileFnMaster        = 0xf500, // Downloads user file from node.
		FileFnReply         = 0xf600, // Server to Node acknowledge for download m_packets.
		IpProg              = 0xf800, // ArtIpProg m_packet. It is used to reprogramme the IP address and Mask of the Node.
		IpProgReply         = 0xf900, // ArtIpProgReply m_packet. It is returned by the node to acknowledge receipt of an ArtIpProg m_packet.
		Media               = 0x9000, // ArtMedia m_packet. It is Unicast by a Media Server and acted upon by a Controller.
		MediaPatch          = 0x9100, // ArtMediaPatch m_packet. It is Unicast by a Controller and acted upon by a Media Server.
		MediaControl        = 0x9200, // ArtMediaControl m_packet. It is Unicast by a Controller and acted upon by a Media Server.
		MediaContrlReply    = 0x9300, // ArtMediaControlReply m_packet. It is Unicast by a Media Server and acted upon by a Controller.
		TimeCode            = 0x9700, // ArtTimeCode m_packet. It is used to transport time code over the network.
		TimeSync            = 0x9800, // Used to synchronise real time date and clock.
		Trigger             = 0x9900, // Used to send trigger macros.
		Directory           = 0x9a00, // Requests a node's file list.
		DirectoryReply      = 0x9b00, // Replies to OpDirectory with file list.
	};

	// --- Other ArtNet related messages ---
	enum class NodeFlag : u8 {
		None                    = 0x00,    // no bits set
		SendPollReplyOnChange   = 1u << 1, // Bit-1
		ReceiveDiagnostics      = 1u << 2, // Bit-2
		UnicastDiagnostics      = 1u << 3, // Bit-3
		DisableVLC              = 1u << 4, // Bit-4
		EnableTargetedMode      = 1u << 5, // Bit-5
		// Bits 6 & 7 are reserved and must remain zero
	};

	enum class DiagPriority : u8 { //The lowest priority of diagnostics message that should be sent.
		Low         = 0x10, // Low priority message.
		Med         = 0x40, // Medium priority message.
		High        = 0x80, // High priority message.
		Critical    = 0xe0, // Critical priority message.
		Volatile    = 0xf0, // Displayed in a single line in DMX-Workshop diagnostics display. All other types are displayed in a list box.
	};

	constexpr std::size_t MIN_PACKET_SIZE = 18;
	constexpr std::size_t MAX_PACKET_SIZE = 1024;

	#pragma pack(push, 1)
	struct DMX_BinaryPacket {
		std::array<u8, 8> signature; // [0-7] Always "Art-Net,0x00"
		Op operation;                // [8-9] The OpCode defines the class of data within this UDP m_packet.
		u16 version;                 // [10-11] Always "0x000E" or "14"
		u8 sequenceID;               // [12] Packet Sequence order tracking [0x01 -> 0xFF || 0x00 "disabled"]
		u8 physical;                 // [13] Physical input port for DMX512 input.
		u16 universeID;              // [14-15] [u8 - SubUni] [u8 - Net] // Big Endian Description [0x0000] where {Net(0b0111 1111), SubNet(0b1111), Universe(0b1111)}
		u16 dmxLength;               // [16-17] The length of the DMX512 data array. This value should be an even number in the range 2 – 512.
		std::array<u8, 512> dmxData; // [18-530] A variable length array of DMX512 lighting data.
	};
	#pragma pack(pop)

	enum class Err {
		InvalidPacketSize,
		InvalidSignature,
		InvalidOpCode,
		InvalidDmxLength
	};

	static_assert(std::tuple_size_v<decltype(DMX_BinaryPacket{}.dmxData)> == 512);
	static_assert(sizeof(DMX_BinaryPacket) >= 18 + 512);
	// Raw Art-Net packets to DMX data storage.
	auto ProcessDmxPacket(
		std::span<const u8> buffer, // Art-Net UDP packet (Read-Only)
		std::span<u8>       storage // DMX Storage Location (Mutable)
	) -> std::expected<u16, std::string> {

		if (buffer.size() < MIN_PACKET_SIZE || buffer.size() > MAX_PACKET_SIZE) {
			return std::unexpected("Invalid Size");
		}
		
		if (std::memcmp(buffer.data(), &ARTNET_SIGNATURE, 8) != 0) {
			return std::unexpected("Invalid Signature");
		}
		
		DMX_BinaryPacket pkt{};
		std::memcpy(&pkt, buffer.data(), sizeof(DMX_BinaryPacket));

		if (pkt.operation != Op::Dmx) {
			return std::unexpected("Invalid Op Code");
		}

		if (pkt.dmxLength > 512 || (pkt.dmxLength % 2) != 0) {
			return std::unexpected("Invalid DMX length");
		}

		if (pkt.universeID > 9) {
			return pkt.universeID;
		}

		const std::size_t offset   = pkt.universeID * 520;
		std::memcpy(storage.data() + offset, pkt.dmxData.data(), 512);
		return pkt.universeID;
	}
}