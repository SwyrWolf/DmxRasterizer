# Packet Formats

## Packet Types

| Constant                   | Value |
|----------------------------|-------|
| PacketTypeHandshake        | 0x01  |
| PacketTypeHandshakeAck     | 0x02  |
| PacketTypeDMX              | 0x03  |
| PacketTypeHeartbeat        | 0x04  |
| PacketTypeHeartbeatAck     | 0x05  |
| PacketTypeRegisterListener | 0x06  |

---

## TCP Packets

All TCP packets share a 15-byte relay header followed by an optional body.

### Relay Header (15 bytes)

| Field     | Type   | Size    | Notes                    |
|-----------|--------|---------|--------------------------|
| Signature | bytes  | 8 bytes | ASCII `dmxrelay`         |
| Version   | uint16 | 2 bytes | Big-endian; currently 1  |
| Type      | uint8  | 1 byte  | Packet type constant     |
| BodyLen   | uint32 | 4 bytes | Big-endian; body length  |

### Handshake (0x01) — Client → Server

| Field     | Type   | Size            |
|-----------|--------|-----------------|
| NameLen   | uint16 | 2 bytes         |
| Name      | string | NameLen bytes   |
| AccessLen | uint16 | 2 bytes         |
| Access    | string | AccessLen bytes |

### HandshakeAck (0x02) — Server → Client

| Field     | Type   | Size           | Notes                         |
|-----------|--------|----------------|-------------------------------|
| SessionID | uint32 | 4 bytes        | Big-endian                    |
| TokenLen  | uint16 | 2 bytes        | Big-endian                    |
| Token     | string | TokenLen bytes | 32-byte hex-encoded UDP token |

### DMX (0x03) — Server → Client (forwarded)

| Field    | Type    | Size      | Notes      |
|----------|---------|-----------|------------|
| Universe | uint16  | 2 bytes   | Big-endian |
| Data     | bytes   | 512 bytes | DMX data   |

### RegisterListener (0x06) — Client → Server

Header only. BodyLen = 0, no body. Registers the sending session as the current listener. Only one listener is tracked at a time; a new registration replaces the previous one. When the listener's session disconnects, the listener is cleared.

### Heartbeat (0x04) / HeartbeatAck (0x05)

Header only. BodyLen = 0, no body.

---

## UDP Packets

UDP uses a separate wire format with its own `dmx` (3-byte) header.

### DMX Data Packet (553 bytes) — Client → Server

| Offset   | Field     | Type    | Size     | Notes      |
|----------|-----------|---------|----------|------------|
| [0:3]    | Header    | bytes   | 3 bytes  | ASCII `dmx` |
| [3:7]    | SessionID | uint32  | 4 bytes  | Big-endian |
| [7:39]   | Token     | string  | 32 bytes | ASCII hex UDP token |
| [39:41]  | Universe  | uint16  | 2 bytes  | Big-endian |
| [41:553] | Data      | bytes   | 512 bytes | DMX data  |

### DMX Forwarded Packet (518 bytes) — Server → Listener

When a DMX data packet is received from a sender, the server forwards it to the registered listener over UDP.

| Offset   | Field    | Type   | Size      | Notes                     |
|----------|----------|--------|-----------|---------------------------|
| [0:3]    | Header   | bytes  | 3 bytes   | ASCII `dmx`               |
| [3]      | Type     | uint8  | 1 byte    | 0x03 (DMX)                |
| [4:6]    | Universe | uint16 | 2 bytes   | Big-endian                |
| [6:518]  | Data     | bytes  | 512 bytes | DMX data                  |

### UDP Heartbeat / HeartbeatAck — Server → Client (4 bytes)

| Offset | Field  | Type  | Size    |
|--------|--------|-------|---------|
| [0:3]  | Header | bytes | 3 bytes | ASCII `dmx` |
| [3]    | Type   | uint8 | 1 byte  | 0x04 (Heartbeat) or 0x05 (HeartbeatAck) |

### UDP Heartbeat / HeartbeatAck — Client → Server (40 bytes)

| Offset  | Field     | Type   | Size     | Notes      |
|---------|-----------|--------|----------|------------|
| [0:3]   | Header    | bytes  | 3 bytes  | ASCII `dmx` |
| [3]     | Type      | uint8  | 1 byte   | 0x04 (Heartbeat) or 0x05 (HeartbeatAck) |
| [4:8]   | SessionID | uint32 | 4 bytes  | Big-endian |
| [8:40]  | Token     | string | 32 bytes | ASCII hex UDP token |