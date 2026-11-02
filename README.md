# libov - Library for Real-Time Audio Networking

## Overview

`libov` is a C++ library that provides the core networking functionality for the OVBOX system, enabling real-time audio streaming and session management across distributed audio devices. It implements a peer-to-peer and server-based communication protocol optimized for low-latency audio applications in professional audio environments.

The library serves as the foundation for `ov-client`, the main application that handles audio rendering, session management, and configuration synchronization. It's designed to work with the OVBOX ecosystem, which includes:
- **Configuration server** (`ov-webfrontend`): Manages device registration, session keys, and configuration distribution
- **Audio relay server** (`ov-server`): Handles real-time audio packet routing between devices
- **OVBOX hardware**: Specialized audio processing devices running the client software

More information can be found at
[ovbox.de](https://ovbox.de/) or
[github.com/gisogrimm/ov-client](https://github.com/gisogrimm/ov-client). The
code of the configuration server can be found at
[github.com/gisogrimm/ov-webfrontend](https://github.com/gisogrimm/ov-webfrontend).

## Core Functionality

### Audio Relay Protocol ("Room Service")

The library implements a custom UDP-based protocol for real-time audio streaming with these key features:

- **Session-based communication**: Devices join sessions using a shared secret (PIN)
- **Hybrid architecture**: Supports both peer-to-peer and server-relayed communication
- **Low-latency optimization**: Uses DSCP EF (Expedited Forwarding) for audio packets
- **Sequence number handling**: Detects and recovers from packet loss and out-of-order delivery
- **Encryption support**: Optional AES encryption using libsodium for secure communication
- **Network awareness**: Distinguishes between local and remote networks for optimal routing

### Configuration Management

The library provides REST API client functionality to communicate with the configuration server.

### Network Discovery and Management

- **IP Address Detection**: Automatically finds the primary network interface IP
- **MAC Address Retrieval**: Cross-platform MAC address detection
- **Network Interface Enumeration**: Lists available network devices
- **Subnet Detection**: Determines if devices are on the same local network

## Technical Architecture

### Key Components

1. **`ovboxclient_t`**: Main communication class handling:
   - Session server connection
   - Peer-to-peer communication
   - Ping/pong latency measurement
   - Endpoint management

2. **`ov_render_tascar_t`**: Audio rendering backend that:
   - Manages TASCAR virtual acoustics engine
   - Handles JACK audio connections
   - Processes audio routing and effects

3. **`ov_client_orlandoviols_t`**: Frontend interface that:
   - Communicates with configuration server
   - Manages device lifecycle
   - Handles configuration updates


## Operation Modes

The following operation modes are supported:

| Flag                          | Value  | Description                                 |
|-------------------------------|--------|---------------------------------------------|
| `B_PEER2PEER`                 | `0x01` | Enable peer-to-peer communication           |
| `B_RECEIVEDOWNMIX_deprecated` | `0x02` | Receive downmixed signals only (deprecated) |
| `B_DONOTSEND`                 | `0x04` | Don't send audio to this client             |
| `B_SENDDOWNMIX_deprecated`    | `0x08` | Send downmixed signals (deprecated)         |
| `B_USINGPROXY`                | `0x10` | Use proxy for communication                 |
| `B_ENCRYPTION`                | `0x20` | Enable encryption                           |

### Routing Logic

The routing decision depends on the operation modes of both the local device and the target device:

| Local Mode    | Target Mode   | Action                                   |
|---------------|---------------|------------------------------------------|
| PEER2PEER     | PEER2PEER     | Send directly to peer (if not DONOTSEND) |
| PEER2PEER     | Not PEER2PEER | Send to server                           |
| Not PEER2PEER | Any           | Send to server                           |

### Proxy Mode

When `USINGPROXY` is enabled:
- The device receives data only from the proxy, not directly from the server or peers
- The proxy forwards messages from external networks to the local network
- Devices in the same network as the proxy can receive data directly from the proxy

### Encryption

When `ENCRYPTION` is enabled:
- Messages are encrypted using libsodium's `crypto_box_seal` function
- Public keys must be exchanged between peers for encryption to work
- Encryption is only used when both sender and receiver have encryption enabled and have each other's public keys

### Deprecated Modes

The `RECEIVEDOWNMIX_deprecated` and `SENDDOWNMIX_deprecated` modes are deprecated and should not be used in new implementations.
