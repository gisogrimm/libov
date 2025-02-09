/*
 * This file is part of the ovbox software tool, see <http://orlandoviols.com/>.
 *
 * Copyright (c) 2020 2021 Giso Grimm
 */
/*
 * ovbox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * ovbox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with ovbox. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COMMON_H
#define COMMON_H

/**
 * @defgroup operationmodes Operation modes of devices
 * @ingroup proxymode
 */

/**
 * @defgroup networkprotocol Definition of the network protocol
 */

#include <getopt.h>
#include <iostream>
#include <mutex>

#include "ov_types.h"

#ifndef DEBUG
#define DEBUG(x)                                                               \
  std::cerr << __FILE__ << ":" << __LINE__ << ": " << #x << "=" << x           \
            << std::endl
#endif

/**
 * Maximum buffer size in bytes.
 *
 * This size limits the maximum size of unpacked messages, which can
 * be BUFSIZE-HEADERLEN. Currently no check against mtu of network
 * connection is performed.
 */
#define BUFSIZE 8192

/// Maximum number of clients in a stage
#define MAX_STAGE_ID 32
/// Special stage ID of server
#define STAGE_ID_SERVER 0xff

/**
 * @ingroup networkprotocol
 * Numbers of port_t with special meanings reserved for control packages
 */
enum {
  /// Register or update a device in a session
  PORT_REGISTER,
  /// Query all device IDs in a session
  PORT_LISTCID,
  PORT_PING,
  PORT_PONG,
  PORT_PEERLATREP,
  PORT_SEQREP,
  PORT_SETLOCALIP,
  PORT_PING_SRV,
  PORT_PONG_SRV,
  PORT_PING_LOCAL,
  PORT_PONG_LOCAL,
  PORT_PUBKEY,
  MAXSPECIALPORT
};

extern int verbose;

void log(int portno, const std::string& s, int v = 1);

/**
 * Set priority of current thread.
 * @param prio New priority
 * No error is reported if setting thread priority fails.
 */
void set_thread_prio(unsigned int prio);

void app_usage(const std::string& app_name, struct option* opt,
               const std::string& app_arg = "", const std::string& help = "");

/**
 * @ingroup operationmodes
 * @ingroup proxymode
 * Bit mask describing device operation mode
 */
typedef uint8_t epmode_t;

/**
 * @ingroup operationmodes
 * Use peer-to-peer mode
 */
#define B_PEER2PEER 0x01
/**
 * @ingroup operationmodes
 * Can receive only downmixed signals
 */
#define B_RECEIVEDOWNMIX 0x02
/**
 * @ingroup operationmodes
 * Do not send audio to this client.
 * If also B_USINGPROXY is set, then devices from local network will send.
 */
#define B_DONOTSEND 0x04
/**
 * @ingroup operationmodes
 * This device is sending a session mix, do not send to normal clients.
 */
#define B_SENDDOWNMIX 0x08
/**
 * @ingroup operationmodes
 *
 * This device is using a proxy.
 */
#define B_USINGPROXY 0x10

// the message header is a byte array with:
// - secret
// - stage_device_id
// - sequence number
// - user data

/**
 * @ingroup networkprotocol
 * Position of stage device ID within a packed message
 */
#define POS_CALLERID sizeof(secret_t)
/**
 * @ingroup networkprotocol
 * Position of port number within a packed message
 */
#define POS_PORT (POS_CALLERID + sizeof(stage_device_id_t))
/**
 * @ingroup networkprotocol
 * Position of sequence number within a packed message
 */
#define POS_SEQ (POS_PORT + sizeof(port_t))
/**
 * @ingroup networkprotocol
 * Length of header in a packed message
 */
#define HEADERLEN (POS_SEQ + sizeof(sequence_t))

/**
 * @ingroup networkprotocol
 * Get session secret in a packed message
 * @param m Packed message
 * @return Reference to session secret variable within message
 */
inline secret_t& msg_secret(char* m)
{
  return *((secret_t*)m);
};
/**
 * @ingroup networkprotocol
 * Get device ID in a packed message
 * @param m Packed message
 * @return Reference to device ID variable within message
 */
inline stage_device_id_t& msg_callerid(char* m)
{
  return *((stage_device_id_t*)(&m[POS_CALLERID]));
};
/**
 * @ingroup networkprotocol
 * Get port number in a packed message
 * @param m Packed message
 * @return Reference to port number variable within message
 */
inline port_t& msg_port(char* m)
{
  return *((port_t*)(&m[POS_PORT]));
};
/**
 * @ingroup networkprotocol
 * Get sequence number in a packed message
 * @param m Packed message
 * @return Reference to sequence number variable within message
 */
inline sequence_t& msg_seq(char* m)
{
  return *((sequence_t*)(&m[POS_SEQ]));
};
/**
 * @ingroup networkprotocol
 * Get session secret in a packed message
 * @param m Packed message
 * @return Reference to session secret variable within message
 */
inline const secret_t& msg_secret(const char* m)
{
  return *((secret_t*)m);
};
/**
 * @ingroup networkprotocol
 * Get device ID in a packed message
 * @param m Packed message
 * @return Reference to device ID variable within message
 */
inline const stage_device_id_t& msg_callerid(const char* m)
{
  return *((stage_device_id_t*)(&m[POS_CALLERID]));
};
/**
 * @ingroup networkprotocol
 * Get port number in a packed message
 * @param m Packed message
 * @return Reference to port number variable within message
 */
inline const port_t& msg_port(const char* m)
{
  return *((port_t*)(&m[POS_PORT]));
};
/**
 * @ingroup networkprotocol
 * Get sequence number in a packed message
 * @param m Packed message
 * @return Reference to sequence number variable within message
 */
inline const sequence_t& msg_seq(const char* m)
{
  return *((sequence_t*)(&m[POS_SEQ]));
};

/**
 * @ingroup networkprotocol
 * Serialize header and original message info a destination buffer for
 * transmission to peers.
 * @param[out] destbuf Start of memory area where the data is stored.
 * @param[in] maxlen Size of the data memory in bytes.
 * @param[in] secret Access code for session
 * @param[in] callerid Identification of sender device
 * @param[in] destport Destination port of message
 * @param[in] seq Sequence number
 * @param[in] msg Start of memory area of original message
 * @param[in] msglen Lenght of original message
 *
 * If successful, the size of the serialized new message will be
 * returned. If the size of the destination buffer is not large enough
 * to pack header and message, then zero is returned.
 */
size_t packmsg(char* destbuf, size_t maxlen, secret_t secret,
               stage_device_id_t callerid, port_t destport, sequence_t seq,
               const char* msg, size_t msglen);

/**
 * @ingroup networkprotocol
 * Append data to a message buffer.
 * @param[out] destbuf Start of memory area where the data is stored.
 * @param[in] maxlen Size of the data memory in bytes.
 * @param[in] currentlen Current fill size bytes.
 * @param[in] msg Start of memory area of the data to be appended.
 * @param[in] msglen Lenght of data.
 *
 * If successful, the size of the serialized new message will be
 * returned. If the size of the destination buffer is not large enough
 * to pack header and message, then zero is returned.
 */
size_t addmsg(char* destbuf, size_t maxlen, size_t currentlen, const char* msg,
              size_t msglen);

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
