/*
 * This file is part of the ovbox software tool, see <http://orlandoviols.com/>.
 *
 * Copyright (c) 2020 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
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

#ifndef UDP_SOCKET_H
#define UDP_SOCKET_H

#include "common.h"
#include <atomic>
#if defined(LINUX) || defined(linux) || defined(__APPLE__)
#include <netinet/ip.h>
#include <sys/socket.h>
#endif

#if defined(WIN32) || defined(UNDER_CE)
#include <winsock2.h>

#include <winsock.h>
#endif

#include <sys/types.h>

#include <unistd.h>

typedef struct sockaddr_in endpoint_t;

endpoint_t ovgethostbyname(const std::string& host);

std::string addr2str(const struct in_addr& addr);
std::string ep2str(const endpoint_t& ep);
std::string ep2ipstr(const endpoint_t& ep);

std::string getmacaddr();
endpoint_t getipaddr();
std::vector<std::string> getnetworkdevices();

inline bool is_same_network(const endpoint_t& a, const endpoint_t& b)
{
  return ((a.sin_addr.s_addr & 0xffffff) == (b.sin_addr.s_addr & 0xffffff)) &&
         (a.sin_addr.s_addr != 0) && (b.sin_addr.s_addr != 0);
};

/**
 * Send and receive UDP messages
 */
class udpsocket_t {
public:
  /**
   * Default constructor.
   *
   * This call will open a socket of AF_INET domain and of type
   * SOCK_DGRAM, i.e., a UDP socket. If the socket creation failed, an
   * exception of type ErrMsg with an appropriate error message is
   * thrown.
   *
   * On Linux and Windows the type of service (IPTOS) is set to CS6
   * and socket priority is set to 5.
   */
  udpsocket_t();
  /**
   * Deconstructor. This will close the socket.
   */
  ~udpsocket_t();
  /**
   * Set the receive timeout.
   *
   * @param usec Timeout in Microseconds, or zero to set to blocking
   * mode (default)
   */
  void set_timeout_usec(int usec);
  /**
   * Set priority for all packets (SO_PRIORITY)
   *
   * @param priority Priority value, 0 to 6.
   */
  void set_netpriority(int priority);
  /**
   * Set flags for low loss, low latency, low jitter, assured
   * bandwidth, end-to-end service according to RFC2598
   */
  void set_expedited_forwarding_PHB();
  /**
   * Bind the socket to a port.
   *
   * @param port Port number to bind the port to.
   * @param loopback Use loopback device (otherwise use 0.0.0.0)
   * @return The actual port used.
   *
   * Upon error (the underlying system call returned -1), an exception
   * of type ErrMsg with an appropriate error message is thrown.
   */
  port_t bind(port_t port, bool loopback = false);
  /**
   * Set destination host or IP address.
   * @param host Host name or IP address of destination.
   *
   * This function resolves the hostname and sets the resulting IP
   * address as a destination. Upon error, an exception of type ErrMsg
   * with an appropriate error message is thrown.
   */
  void set_destination(const char* host);
  /**
   * Send a message to a port at the previously configured destination
   *
   * Upon success, the tx_bytes counter is increased by the number of
   * bytes sent.
   *
   * @param buf Start of memory area containing the message
   * @param len Length of message in bytes
   * @param portno Destination port number
   * @return The number of bytes sent, or -1 in case of failure
   */
  ssize_t send(const char* buf, size_t len, uint16_t portno);
  /**
   * Send a message to a destination.
   *
   * Upon success, the tx_bytes counter is increased by the number of
   * bytes sent.
   *
   * @param buf Start of memory area containing the message
   * @param len Length of message in bytes
   * @param ep Destination address and port
   * @return The number of bytes sent, or -1 in case of failure
   */
  ssize_t send(const char* buf, size_t len, const endpoint_t& ep);
  /**
   * Receive a message.
   *
   * Upon success, the rx_bytes counter is increased by the number of
   * bytes received.
   *
   * @param buf Start of memory area where the message should be stored
   * @param len Lnegth of provided memory area in bytes
   * @param addr Reference to an IP address, will be filled with sender address
   * @return Number of bytes received, or -1 in case of failure
   */
  ssize_t recvfrom(char* buf, size_t len, endpoint_t& addr);
  /**
   * Return the address where the socket is currently bound to.
   *
   * @return Address
   */
  endpoint_t getsockep();
  /**
   * Close the socket.
   */
  void close();
  /**
   * Return name of default destination.
   */
  const std::string addrname() const { return ep2str(serv_addr); };
  /**
   * Return address of default destination.
   */
  const endpoint_t& get_destination() const { return serv_addr; };

private:
  int sockfd;
  endpoint_t serv_addr;
  bool isopen;

public:
  /**
   * Number of bytes sent through this socket.
   */
  std::atomic_size_t tx_bytes;
  /**
   * Number of bytes received through this socket.
   */
  std::atomic_size_t rx_bytes;
};

class sequence_map_t : public std::map<port_t, sequence_t> {
public:
  inline sequence_t& operator[](port_t port)
  {
    auto it = find(port);
    if(it == end()) {
      insert(std::pair<port_t, sequence_t>(port, 0));
    }
    return at(port);
  };
};

/**
 * @ingroup networkprotocol
 * Container for an unpacked message buffer.
 */
class msgbuf_t {
public:
  /**
   * Default constructor, invalidates and allocates BUFSIZE bytes in
   * buffer
   */
  msgbuf_t();
  ~msgbuf_t();
  msgbuf_t(const msgbuf_t&) = delete;
  void copy(const msgbuf_t& src);
  /**
   * @ingroup networkprotocol
   * Serialize header and original message info a destination buffer for
   * transmission to peers.
   * @param[in] secret Access code for session
   * @param[in] callerid Identification of sender device
   * @param[in] destport Destination port of message
   * @param[in] seq Sequence number
   * @param[in] msg Start of memory area of original message
   * @param[in] msglen Lenght of original message
   *
   */
  void pack(secret_t secret, stage_device_id_t callerid, port_t destport,
            sequence_t seq, const char* msg, size_t msglen);
  /**
   * Unpack a packed message stored in the raw buffer.
   *
   * @param msglen Length of packed source message
   *
   * On success valid member is set to true and the msg pointer is
   * updated, otherwise valid is set to false.
   */
  void unpack(size_t msglen);
  /**
   * Return age of a message in Milliseconds.
   * The age is measured since it was unpacked.
   */
  double get_age();
  /**
   * Reset timer for age measurement.
   */
  void set_tick();
  bool valid; ///< Status of message buffer, if true, the data can be read, if
              ///< false, it can be overwritten
  stage_device_id_t cid; ///< Device ID in session
  port_t destport;       ///< Destination port
  sequence_t seq;        ///< Sequence number
  size_t size;           ///< Actual size of the unpacked message
  char* rawbuffer;       ///< Data containing the packed message
  char* msg;             ///< Data containing the unpacked message
  endpoint_t sender;     ///< IP address and port of sender
private:
  std::chrono::high_resolution_clock::time_point t;
};

class ovbox_udpsocket_t : public udpsocket_t {
public:
  ovbox_udpsocket_t(secret_t secret, stage_device_id_t id);
  void send_ping(const endpoint_t& ep, stage_device_id_t destid = 0,
                 port_t proto = PORT_PING);
  void send_registration(epmode_t, port_t port, const endpoint_t& localep);
  /**
   * Receive a message, extract header and validate secret.
   *
   * @param inputbuf Start of memory area where data can be written
   * @param[out] ilen Number of bytes received from network
   * @param[out] len Length of unpacked message, in bytes
   * @param[out] cid Session ID of sending device
   * @param[out] destport Destination port number or special control code
   * @param[out] seq Sequence number
   * @param[out] addr Sender IP address and port number
   *
   * @return Start of memory area where the unpacked data can be read,
   *    or NULL in case of timeout, insufficient memory or invalid
   *    secret code
   *
   */
  char* recv_sec_msg(char* inputbuf, size_t& ilen, size_t& len,
                     stage_device_id_t& cid, port_t& destport, sequence_t& seq,
                     endpoint_t& addr);
  /**
   * Receive a message, extract header and validate secret.
   *
   * @param msg Reference to message buffer to be updated
   * @param[out] addr Sender IP address and port number
   *
   * @return True if receiving, decoding and validation of secret was
   * successful, false otherwise.
   *
   */
  bool recv_sec_msg(msgbuf_t& msg);
  void set_secret(secret_t s) { secret = s; };
  /**
   * Pack a message with current secret, caller id and sequence number.
   *
   * @param[out] destbuf Start of memory area where the data is stored.
   * @param[in] maxlen Size of the data memory in bytes.
   * @param[in] destport Destination port of message
   * @param[in] msg Start of memory area of original message
   * @param[in] msglen Lenght of original message
   *
   * If successful, the size of the serialized new message will be
   * returned. If the size of the destination buffer is not large enough
   * to pack header and message, then zero is returned.
   *
   * This method will increment the port and receiver specific sequence number.
   */
  size_t packmsg(char* destbuf, size_t maxlen, port_t destport, const char* msg,
                 size_t msglen);
  /**
   * Pack and send message
   *
   * @param destport Packed destination port
   * @param msg Start of message buffer
   * @param msglen Length of message in bytes
   * @param remoteport Remote server port
   */
  bool pack_and_send(port_t destport, const char* msg, size_t msglen,
                     port_t remoteport);

protected:
  secret_t secret;
  stage_device_id_t callerid;
  sequence_map_t seqmap;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
