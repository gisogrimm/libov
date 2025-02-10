/*
 * This file is part of the ovbox software tool, see <http://orlandoviols.com/>.
 *
 * Copyright (c) 2020 Giso Grimm, gereblye
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

#if defined(WIN32) || defined(UNDER_CE)
#include <ws2tcpip.h>
// for ifaddrs.h
#include <iphlpapi.h>
#define MSG_CONFIRM 0

#elif defined(LINUX) || defined(linux) || defined(__APPLE__)
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include "MACAddressUtility.h"
#include "errmsg.h"
#include "udpsocket.h"
#include <errno.h>

#include <stdio.h>
#include <string.h>
#include <strings.h>

#if defined(__linux__)
#include <linux/wireless.h>
#endif

#ifdef __APPLE__
#include "MACAddressUtility.h"
#define MSG_CONFIRM 0
#endif

#include "../tascar/libtascar/include/tscconfig.h"

#define LISTEN_BACKLOG 512

const size_t
    pingbufsize(HEADERLEN +
                sizeof(std::chrono::high_resolution_clock::time_point) +
                sizeof(stage_device_id_t) + sizeof(endpoint_t) + 100);

endpoint_t ovgethostbyname(const std::string& host)
{
  // resolve host:
  struct hostent* server;
  server = gethostbyname(host.c_str());
  if(server == NULL)
#if defined(WIN32) || defined(UNDER_CE)
    // windows:
    throw ErrMsg("No such host: " + std::to_string(WSAGetLastError()));
#else
    throw ErrMsg("No such host: " + std::string(hstrerror(h_errno)));
#endif
  endpoint_t serv_addr;
#if defined(WIN32) || defined(UNDER_CE)
  // windows:
  memset((char*)&serv_addr, 0, sizeof(serv_addr));
#else
  bzero((char*)&serv_addr, sizeof(serv_addr));
#endif
  serv_addr.sin_family = AF_INET;
  memcpy((char*)&serv_addr.sin_addr.s_addr, (char*)server->h_addr,
         server->h_length);
  return serv_addr;
}

udpsocket_t::udpsocket_t() : tx_bytes(0), rx_bytes(0)
{
  // linux part, sets value pointed to by &serv_addr to 0 value:
  // bzero((char*)&serv_addr, sizeof(serv_addr));
  // windows:
  memset((char*)&serv_addr, 0, sizeof(serv_addr));
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if(sockfd < 0)
    throw ErrMsg("Opening socket failed: ", errno);
#ifndef __APPLE__
  // int iptos = IPTOS_CLASS_CS6;
  int iptos = 0xc0;
#if defined(WIN32) || defined(UNDER_CE)
  // setsockopt defined in winsock2.h 3rd parameter const char*
  // windows (cast):
  setsockopt(sockfd, IPPROTO_IP, IP_TOS, reinterpret_cast<const char*>(&iptos),
             sizeof(iptos));
#else
  // on linux:
  setsockopt(sockfd, IPPROTO_IP, IP_TOS, &iptos, sizeof(iptos));
#endif
#endif
#ifdef WIN32
  u_long mode = 1;
  ioctlsocket(sockfd, FIONBIO, &mode);
#endif
  set_netpriority(6);
  isopen = true;
}

void udpsocket_t::set_netpriority(int priority)
{
#ifndef __APPLE__
  // gnu SO_PRIORITY
  // IP_TOS defined in ws2tcpip.h
#if defined(WIN32) || defined(UNDER_CE)
  // no documentation on what SO_PRIORITY does, optname, level in GNU socket
  setsockopt(sockfd, SOL_SOCKET, SO_GROUP_PRIORITY,
             reinterpret_cast<const char*>(&priority), sizeof(priority));
#else
  // on linux:
  setsockopt(sockfd, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority));
#endif
#endif
}

void udpsocket_t::set_expedited_forwarding_PHB()
{
#ifndef __APPLE__
#ifndef WIN32
  int iptos = IPTOS_DSCP_EF;
  setsockopt(sockfd, IPPROTO_IP, IP_TOS, &iptos, sizeof(iptos));
#endif
#endif
  set_netpriority(6);
}

udpsocket_t::~udpsocket_t()
{
  close();
}

void udpsocket_t::set_timeout_usec(int usec)
{
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = usec;
#if defined(WIN32) || defined(UNDER_CE)
  // windows (cast):
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
#else
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
}

void udpsocket_t::close()
{
  if(isopen)
#if defined(WIN32) || defined(UNDER_CE)
    ::closesocket(sockfd);
#else
    ::close(sockfd);
#endif
  isopen = false;
}

void udpsocket_t::set_destination(const char* host)
{
  struct hostent* server;
  server = gethostbyname(host);
  if(server == NULL)
#if defined(WIN32) || defined(UNDER_CE)
    // windows:
    throw ErrMsg("No such host: " + std::to_string(WSAGetLastError()));
#else
    throw ErrMsg("No such host: " + std::string(hstrerror(h_errno)));
#endif
  memset((char*)&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  memcpy((char*)&serv_addr.sin_addr.s_addr, (char*)server->h_addr,
         server->h_length);
}

port_t udpsocket_t::bind(port_t port, bool loopback)
{
  int optval = 1;
  // setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval,
  //           sizeof(int));
  // windows (cast):
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval,
             sizeof(int));

  endpoint_t my_addr;
  memset(&my_addr, 0, sizeof(endpoint_t));
  /* Clear structure */
  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons((unsigned short)port);
  if(loopback) {
    my_addr.sin_addr.s_addr = 0x0100007f;
  }
  if(::bind(sockfd, (struct sockaddr*)&my_addr, sizeof(endpoint_t)) == -1)
    throw ErrMsg("Binding the socket to port " + std::to_string(port) +
                     " failed: ",
                 errno);
  socklen_t addrlen(sizeof(endpoint_t));
  getsockname(sockfd, (struct sockaddr*)&my_addr, &addrlen);
  return ntohs(my_addr.sin_port);
}

endpoint_t udpsocket_t::getsockep()
{
  endpoint_t my_addr;
  memset(&my_addr, 0, sizeof(endpoint_t));
  socklen_t addrlen(sizeof(endpoint_t));
  getsockname(sockfd, (struct sockaddr*)&my_addr, &addrlen);
  return my_addr;
}

ssize_t udpsocket_t::send(const char* buf, size_t len, uint16_t portno)
{
  if(portno == 0)
    return len;
  serv_addr.sin_port = htons(portno);
  ssize_t tx(sendto(sockfd, buf, len, MSG_CONFIRM, (struct sockaddr*)&serv_addr,
                    sizeof(serv_addr)));
  if(tx > 0)
    tx_bytes += tx;
  return tx;
}

ssize_t udpsocket_t::send(const char* buf, size_t len, const endpoint_t& ep)
{
  ssize_t tx(
      sendto(sockfd, buf, len, MSG_CONFIRM, (struct sockaddr*)&ep, sizeof(ep)));
  if(tx > 0)
    tx_bytes += tx;
  return tx;
}

ssize_t udpsocket_t::recvfrom(char* buf, size_t len, endpoint_t& addr)
{
  memset(&addr, 0, sizeof(endpoint_t));
  addr.sin_family = AF_INET;
  socklen_t socklen(sizeof(endpoint_t));
  ssize_t rx(
      ::recvfrom(sockfd, buf, len, 0, (struct sockaddr*)&addr, &socklen));
  if(rx > 0)
    rx_bytes += rx;
  return rx;
}

std::string addr2str(const struct in_addr& addr)
{
  return std::to_string(addr.s_addr & 0xff) + "." +
         std::to_string((addr.s_addr >> 8) & 0xff) + "." +
         std::to_string((addr.s_addr >> 16) & 0xff) + "." +
         std::to_string((addr.s_addr >> 24) & 0xff);
}

std::string ep2str(const endpoint_t& ep)
{
  return addr2str(ep.sin_addr) + "/" + std::to_string(ntohs(ep.sin_port));
}

std::string ep2ipstr(const endpoint_t& ep)
{
  return addr2str(ep.sin_addr);
}

ovbox_udpsocket_t::ovbox_udpsocket_t(secret_t secret, stage_device_id_t cid)
    : secret(secret), callerid(cid)
{
  t_start = std::chrono::high_resolution_clock::now();
  // create key pair:
  crypto_box_keypair(recipient_public, recipient_secret);
  TASCAR::console_log("My public key is \"" +
                      bin2base64(recipient_public, crypto_box_PUBLICKEYBYTES) +
                      "\".");
}

void ovbox_udpsocket_t::set_secret(secret_t s)
{
  secret = s;
  crypto_box_keypair(recipient_public, recipient_secret);
  TASCAR::console_log("My public key is \"" +
                      bin2base64(recipient_public, crypto_box_PUBLICKEYBYTES) +
                      "\".");
}

double ovbox_udpsocket_t::time_since_start() const
{
  std::chrono::duration<double> time_span =
      std::chrono::duration_cast<std::chrono::duration<double>>(
          std::chrono::high_resolution_clock::now() - t_start);
  return time_span.count();
}

void ovbox_udpsocket_t::send_ping(const endpoint_t& ep,
                                  stage_device_id_t destid, port_t proto)
{
  char buffer[pingbufsize];
  size_t n(0);
  n = packmsg(buffer, pingbufsize, proto, "", 0);
  if(proto == PORT_PING_SRV)
    n = addmsg(buffer, pingbufsize, n, (char*)(&destid), sizeof(destid));
  double t1 = time_since_start();
  n = addmsg(buffer, pingbufsize, n, (const char*)(&t1), sizeof(t1));
  n = addmsg(buffer, pingbufsize, n, (char*)(&ep), sizeof(ep));
  send(buffer, n, ep);
}

double ovbox_udpsocket_t::get_pingtime(char*& msg, size_t& msglen)
{
  if(msglen >= sizeof(double)) {
    double t_since_start = time_since_start();
    double t_send = 0;
    memcpy(&t_send, msg, sizeof(t_send));
    msglen -= sizeof(double);
    msg += sizeof(double);
    auto dt = t_since_start - t_send;
    return (1000.0 * dt);
  }
  return -1;
}

void ovbox_udpsocket_t::send_registration(epmode_t mode, port_t port,
                                          const endpoint_t& localep)
{
  std::string rver(OVBOXVERSION);
  {
    size_t buflen(HEADERLEN + rver.size() + 1);
    char buffer[buflen];
    // here we are not using the internal packing method for backward
    // compatibility. This should be no problem because this type of
    // message is handled only by the server, not by peers
    size_t n(::packmsg(buffer, buflen, secret, callerid, PORT_REGISTER, mode,
                       rver.c_str(), rver.size() + 1));
    send(buffer, n, port);
  }
  {
    // send local IP address:
    size_t buflen(HEADERLEN + sizeof(endpoint_t));
    char buffer[buflen];
    size_t n(packmsg(buffer, buflen, PORT_SETLOCALIP, (const char*)(&localep),
                     sizeof(endpoint_t)));
    send(buffer, n, port);
  }
  send_pubkey(port);
}

void ovbox_udpsocket_t::send_pubkey(port_t port)
{
  // send public key:
  size_t buflen(HEADERLEN + crypto_box_PUBLICKEYBYTES);
  char buffer[buflen];
  size_t n(packmsg(buffer, buflen, PORT_PUBKEY, (const char*)recipient_public,
                   crypto_box_PUBLICKEYBYTES));
  send(buffer, n, port);
}

void ovbox_udpsocket_t::send_pubkey(const endpoint_t& ep)
{
  // send public key:
  size_t buflen(HEADERLEN + crypto_box_PUBLICKEYBYTES);
  char buffer[buflen];
  size_t n(packmsg(buffer, buflen, PORT_PUBKEY, (const char*)recipient_public,
                   crypto_box_PUBLICKEYBYTES));
  send(buffer, n, ep);
}

size_t ovbox_udpsocket_t::packmsg(char* destbuf, size_t maxlen, port_t destport,
                                  const char* msg, size_t msglen)
{
  sequence_t& seq(seqmap[destport]);
  if(destport >= MAXSPECIALPORT) {
    seq++;
    return ::packmsg(destbuf, maxlen, secret, callerid, destport, seq, msg,
                     msglen);
  }
  return ::packmsg(destbuf, maxlen, secret, callerid, destport, 0, msg, msglen);
}

bool ovbox_udpsocket_t::pack_and_send(port_t destport, const char* msg,
                                      size_t msglen, port_t remoteport)
{
  char buffer[BUFSIZE];
  size_t len(packmsg(buffer, BUFSIZE, destport, msg, msglen));
  if(len == 0)
    return false;
  ssize_t sendlen(send(buffer, len, remoteport));
  if(sendlen == -1)
    return false;
  return true;
}

char* ovbox_udpsocket_t::recv_sec_msg(char* inputbuf, size_t& ilen, size_t& len,
                                      stage_device_id_t& cid, port_t& destport,
                                      sequence_t& seq, endpoint_t& addr)
{
  ssize_t ilens = recvfrom(inputbuf, ilen, addr);
  if(ilens < 0) {
    ilen = 0;
    return NULL;
  }
  ilen = ilens;
  if(ilen < HEADERLEN)
    return NULL;
  // check secret:
  if(msg_secret(inputbuf) != secret) {
    return NULL;
  }
  cid = msg_callerid(inputbuf);
  destport = msg_port(inputbuf);
  seq = msg_seq(inputbuf);
  len = ilen - HEADERLEN;
  return &(inputbuf[HEADERLEN]);
}

bool ovbox_udpsocket_t::recv_sec_msg(msgbuf_t& msg)
{
  msg.valid = false;
  ssize_t ilens = recvfrom(msg.rawbuffer, BUFSIZE, msg.sender);
  if(ilens < 0)
    return false;
  size_t ilen(ilens);
  if(ilen < HEADERLEN)
    return false;
  // check secret:
  if(msg_secret(msg.rawbuffer) != secret)
    return false;
  msg.unpack(ilen);
  return msg.valid;
}

#if defined(__linux__)
std::string getmacaddr()
{
  std::string retv;
  struct ifreq ifr;
  struct ifconf ifc;
  char buf[1024];
  int success = 0;
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if(sock == -1) { /* handle error*/
    return retv;
  };
  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;
  if(ioctl(sock, SIOCGIFCONF, &ifc) == -1) { /* handle error */
    return retv;
  }
  struct ifreq* it = ifc.ifc_req;
  const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));
  for(; it != end; ++it) {
    strcpy(ifr.ifr_name, it->ifr_name);
    // ioctl(sock, SIOCGIWNAME, &ifr);
    if(ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
      if(!(ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
        if(ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
          // exclude virtual docker devices:
          if((ifr.ifr_hwaddr.sa_data[0] != 0x02) ||
             (ifr.ifr_hwaddr.sa_data[1] != 0x42)) {
            success = 1;
            break;
          }
        }
      }
    } else { /* handle error */
      return retv;
    }
  }
  unsigned char mac_address[6];
  if(success) {
    memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
    char ctmp[1024];
    sprintf(ctmp, "%02x%02x%02x%02x%02x%02x", mac_address[0], mac_address[1],
            mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
    retv = ctmp;
  }
  return retv;
}
#else
std::string getmacaddr()
{
  std::string retv;
  unsigned char mac_address[6];
  char ctmp[1024];
  if(MACAddressUtility::GetMACAddress(mac_address) == 0) {
    sprintf(ctmp, "%02x%02x%02x%02x%02x%02x", mac_address[0], mac_address[1],
            mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
    retv = ctmp;
  }
  return retv;
}
#endif

endpoint_t getipaddr()
{
  endpoint_t my_addr;
  memset(&my_addr, 0, sizeof(endpoint_t));
#if defined(WIN32) || defined(UNDER_CE)
  // DWORD rv, size;
  // PIP_ADAPTER_ADDRESSES adapter_addresses, aa;
  // PIP_ADAPTER_UNICAST_ADDRESS ua;
  //
  // rv = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, NULL,
  //                           &size);
  // if(rv != ERROR_BUFFER_OVERFLOW) {
  //   fprintf(stderr, "GetAdaptersAddresses() failed...");
  //   return my_addr;
  // }
  // adapter_addresses = (PIP_ADAPTER_ADDRESSES)malloc(size);
  //
  // rv = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL,
  //                           adapter_addresses, &size);
  // if(rv == ERROR_SUCCESS) {
  //   for(aa = adapter_addresses; aa != NULL; aa = aa->Next) {
  //     for(ua = aa->FirstUnicastAddress; ua != NULL; ua = ua->Next) {
  //       // my_addr = ua->Address.lpSockaddr;
  //       memcpy(&my_addr, ua->Address.lpSockaddr, sizeof(endpoint_t));
  //       free(adapter_addresses);
  //       return my_addr;
  //     }
  //   }
  // }
  // free(adapter_addresses);

  char hostname[256];
  if(gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR)
    throw ErrMsg("Failed to get host name");
  return ovgethostbyname(hostname);

#else
  struct ifaddrs* addrs;
  getifaddrs(&addrs);
  struct ifaddrs* tmp = addrs;
  while(tmp) {
    if(tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET &&
       (!(tmp->ifa_flags & IFF_LOOPBACK))) {
      memcpy(&my_addr, tmp->ifa_addr, sizeof(endpoint_t));
      freeifaddrs(addrs);
      return my_addr;
    }
    tmp = tmp->ifa_next;
  }
  freeifaddrs(addrs);
#endif
  return my_addr;
}

std::vector<std::string> getnetworkdevices()
{
  std::vector<std::string> devices;
#ifndef WIN32
  struct ifaddrs* addrs;
  getifaddrs(&addrs);
  struct ifaddrs* tmp = addrs;
  while(tmp) {
    if(tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
      devices.push_back(tmp->ifa_name);
    tmp = tmp->ifa_next;
  }
  freeifaddrs(addrs);
#endif
  return devices;
}

msgbuf_t::msgbuf_t()
    : valid(false), cid(0), destport(0), seq(0), size(0),
      rawbuffer(new char[BUFSIZE]), msg(rawbuffer)
{
  memset(rawbuffer, 0, BUFSIZE);
}

void msgbuf_t::copy(const msgbuf_t& src)
{
  valid = src.valid;
  cid = src.cid;
  destport = src.destport;
  seq = src.seq;
  size = src.size;
  memcpy(rawbuffer, src.rawbuffer, BUFSIZE);
  msg = &(rawbuffer[HEADERLEN]);
}

msgbuf_t::~msgbuf_t()
{
  delete[] rawbuffer;
}

void msgbuf_t::pack(secret_t secret, stage_device_id_t callerid,
                    port_t destport, sequence_t seq, const char* msg,
                    size_t msglen)
{
  size_t len(packmsg(rawbuffer, BUFSIZE, secret, callerid, destport, seq, msg,
                     msglen));
  unpack(len);
}

void msgbuf_t::unpack(size_t msglen)
{
  valid = false;
  if((msglen >= HEADERLEN) && (msglen <= BUFSIZE)) {
    cid = msg_callerid(rawbuffer);
    destport = msg_port(rawbuffer);
    seq = msg_seq(rawbuffer);
    size = msglen - HEADERLEN;
    msg = &(rawbuffer[HEADERLEN]);
    valid = true;
  }
}

void msgbuf_t::set_tick()
{
  t = std::chrono::high_resolution_clock::now();
}

double msgbuf_t::get_age()
{
  std::chrono::high_resolution_clock::time_point t2(
      std::chrono::high_resolution_clock::now());
  std::chrono::duration<double> time_span =
      std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t);
  return (1000.0 * time_span.count());
}

std::string bin2base64(uint8_t* data, size_t len)
{
  std::string b64;
  size_t b64_len =
      sodium_base64_encoded_len(len, sodium_base64_VARIANT_ORIGINAL);
  b64.resize(b64_len);
  sodium_bin2base64(b64.data(), b64_len, data, len,
                    sodium_base64_VARIANT_ORIGINAL);
  return b64;
}

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
