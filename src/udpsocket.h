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

std::string addr2str(const struct in_addr& addr);
std::string ep2str(const endpoint_t& ep);
std::string ep2ipstr(const endpoint_t& ep);

std::string getmacaddr();
endpoint_t getipaddr();

/**
 * Send and receive UDP messages
 */
class udpsocket_t {
public:
  udpsocket_t();
  ~udpsocket_t();
  /**
   * Set the receive timeout
   * @param usec Timeout in Microseconds, or zero to set to blocking mode
   * (default)
   */
  void set_timeout_usec(int usec);
  port_t bind(port_t port, bool loopback = false);
  /**
   * Set destination host or IP address.
   * @param host Host name or IP address of destination.
   *
   * This function resolves the hostname and sets the resulting IP
   * address as a destination.
   */
  void set_destination(const char* host);
  /**
   * Send a message to a port at the previously configured destination
   *
   * @param buf Start of memory area containing the message
   * @param len Length of message in bytes
   * @param portno Destination port number
   * @return The number of bytes sent, or -1 in case of failure
   */
  ssize_t send(const char* buf, size_t len, int portno);
  ssize_t send(const char* buf, size_t len, const endpoint_t& ep);
  ssize_t recvfrom(char* buf, size_t len, endpoint_t& addr);
  endpoint_t getsockep();
  void close();
  const std::string addrname() const { return ep2str(serv_addr); };

private:
  int sockfd;
  endpoint_t serv_addr;
  bool isopen;

public:
  std::atomic_size_t tx_bytes;
  std::atomic_size_t rx_bytes;
};

class ovbox_udpsocket_t : public udpsocket_t {
public:
  ovbox_udpsocket_t(secret_t secret);
  void send_ping(stage_device_id_t cid, const endpoint_t& ep);
  void send_registration(stage_device_id_t cid, epmode_t, port_t port,
                         const endpoint_t& localep);
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
  void set_secret(secret_t s) { secret = s; };

protected:
  secret_t secret;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
