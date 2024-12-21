#include "ovtcpsocket.h"
#include "errmsg.h"
#include <cstring>

ovtcpsocket_t::ovtcpsocket_t(port_t udpresponseport_)
    : udpresponseport(udpresponseport_)
{
  memset((char*)&serv_addr, 0, sizeof(serv_addr));
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0)
    throw ErrMsg("Opening socket failed: ", errno);
  set_netpriority(6);
  set_timeout_usec(10000);
}

ovtcpsocket_t::~ovtcpsocket_t()
{
  close();
  if(mainthread.joinable())
    mainthread.join();
  for(auto& thr : handlethreads)
    if(thr.second.joinable())
      thr.second.join();
  if(clienthandler.joinable())
    clienthandler.join();
}

port_t ovtcpsocket_t::connect(endpoint_t ep, port_t targetport_)
{
  run_server = true;
  int retv = ::connect(sockfd, (const struct sockaddr*)(&ep), sizeof(ep));
  if(retv != 0)
    throw ErrMsg("Connection to " + ep2str(ep) + " failed: ", errno);
  binding = true;
  if(retv == 0) {
    targetport = targetport_;
    if(clienthandler.joinable())
      clienthandler.join();
    clienthandler =
        std::thread(&ovtcpsocket_t::handleconnection, this, sockfd, ep);
    // std::cerr << "Connected to " << ep2str(ep) << ", new target port is "
    //         << targetport << std::endl;
  }
  while(binding)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  return connect_receiveport;
}

port_t ovtcpsocket_t::bind(port_t port, bool loopback)
{
  int optval = 1;
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
  if(listen(sockfd, 256) < 0)
    throw ErrMsg("Unable to listen to socket: ", errno);
  run_server = true;
  if(mainthread.joinable())
    mainthread.join();
  mainthread = std::thread(&ovtcpsocket_t::acceptor, this);
#ifdef WIN32
  int addrlen(sizeof(endpoint_t));
#else
  socklen_t addrlen(sizeof(endpoint_t));
#endif
  getsockname(sockfd, (struct sockaddr*)&my_addr, &addrlen);
  targetport = ntohs(my_addr.sin_port);
  return targetport;
}

void ovtcpsocket_t::close()
{
  if(isopen)
#if defined(WIN32) || defined(UNDER_CE)
    ::closesocket(sockfd);
#else
    ::close(sockfd);
#endif
  isopen = false;
  run_server = false;
}

void ovtcpsocket_t::acceptor()
{
  std::cerr << "TCP server ready to accept connections.\n";
  while(run_server) {
    try {
      endpoint_t ep;
#ifdef WIN32
      int len = sizeof(ep);
#else
      socklen_t len = sizeof(ep);
#endif
      int clientfd = accept(sockfd, (struct sockaddr*)(&ep), &len);
      if(clientfd >= 0) {
        if(handlethreads.count(clientfd)) {
          if(handlethreads[clientfd].joinable())
            handlethreads[clientfd].join();
        }
        handlethreads[clientfd] =
            std::thread(&ovtcpsocket_t::handleconnection, this, clientfd, ep);
      }
      usleep(50000);
    }
    catch(const std::exception& e) {
      std::cerr << "Error in TCP acceptor: " << e.what() << std::endl;
      usleep(500000);
    }
  }
  std::cerr << "TCP server closed\n";
}

ssize_t ovtcpsocket_t::nbread(int fd, uint8_t* buf, size_t cnt)
{
  int rcnt = 0;
  while(run_server && (cnt > 0)) {
    int lrcnt = read(fd, buf, cnt);
    if(lrcnt < 0) {
      if(!((errno == EAGAIN) || (errno == EWOULDBLOCK)))
        return lrcnt;
    } else {
      if(lrcnt > 0) {
        cnt -= lrcnt;
        buf += lrcnt;
        rcnt += lrcnt;
      } else {
        return -2;
      }
    }
  }
  return rcnt;
}

ssize_t ovtcpsocket_t::nbwrite(int fd, uint8_t* buf, size_t cnt)
{
  size_t rcnt = 0;
  while(run_server && (cnt > 0)) {
    // attempt to write cnt bytes:
    ssize_t lrcnt = write(fd, buf, cnt);
    if(lrcnt == -1) {
      // an error occurred.
      // ignore (EAGAIN or EWOULDBLOCK), otherwise return error:
      if(!((errno == EAGAIN) || (errno == EWOULDBLOCK)))
        return lrcnt;
    } else {
      // no error:
      if(lrcnt > 0) {
        // some bytes were written.
        if(lrcnt != (ssize_t)cnt) {
          DEBUG(lrcnt);
          DEBUG(cnt);
        }
        cnt -= lrcnt;
        buf += lrcnt;
        rcnt += lrcnt;
      } else {
        // probably EOF:
        return -2;
      }
    }
  }
  // return total number of written bytes:
  return rcnt;
}

ssize_t ovtcpsocket_t::send(int fd, const char* buf, size_t len)
{
  uint8_t csize[4];
  csize[0] = len & 0xff;
  csize[1] = (len >> 8) & 0xff;
  csize[2] = (len >> 16) & 0xff;
  csize[3] = (len >> 24) & 0xff;
  auto wcnt = nbwrite(fd, csize, 4);
  if(wcnt < 4) {
    DEBUG(wcnt);
    return -4;
  }
  return nbwrite(fd, (uint8_t*)buf, len);
}

void udpreceive(udpsocket_t* udp, std::atomic_bool* runthread,
                ovtcpsocket_t* tcp, int fd)
{
  try {
    char buf[BUFSIZE];
    endpoint_t eptmp;
    while(*runthread) {
      ssize_t len = 0;
      if((len = udp->recvfrom(buf, BUFSIZE, eptmp)) > 0) {
        ssize_t slen = tcp->send(fd, buf, len);
        if(slen != len) {
          DEBUG(len);
          DEBUG(slen);
        }
      }
    }
  }
  catch(const std::exception& e) {
    std::cerr << "Error in TCP tunnel UDP receiver: " << e.what() << std::endl;
    usleep(500000);
  }
}

void ovtcpsocket_t::handleconnection(int fd, endpoint_t ep)
{
  port_t targetport = get_target_port();
  udpsocket_t udp;
  port_t udpport = udp.bind(udpresponseport);
  connect_receiveport = udpport;
  binding = false;
  udp.set_timeout_usec(10000);
  std::cerr << "connection from " << ep2str(ep)
            << " established, UDP listening on port " << udpport
            << ", sending to " << targetport << "\n";
  udp.set_destination("127.0.0.1");
  uint8_t buf[BUFSIZE];
  std::atomic_bool runthread = true;
  std::thread udphandlethread(&udpreceive, &udp, &runthread, this, fd);
  try {
    while(run_server) {
      uint8_t csize[4] = {0, 0, 0, 0};
      ssize_t cnt = nbread(fd, csize, 4);
      if(cnt < 4) {
        DEBUG(cnt);
        break;
      }
      if(cnt == 4) {
        size_t size =
            csize[0] + (csize[1] << 8) + (csize[2] << 16) + (csize[3] << 24);
        // read package:
        if(size > BUFSIZE) {
          std::cerr << "Message is too large to fit into buffer (" << size << "/" << BUFSIZE << " Bytes).\n";
          break;
        }
        cnt = nbread(fd, buf, size);
        buf[cnt] = 0;
        if(cnt == (ssize_t)size) {
          udp.send((char*)buf, cnt, targetport);
        } else {
          DEBUG(cnt);
        }
      } else {
        DEBUG(cnt);
      }
    }
  }
  catch(const std::exception& e) {
    std::cerr << "Error in connection handler: " << e.what() << std::endl;
  }
  runthread = false;
  if(udphandlethread.joinable())
    udphandlethread.join();
  std::cerr << "closing connection from " << ep2str(ep) << "\n";
  ::close(fd);
}

void ovtcpsocket_t::set_timeout_usec(int usec, int fd)
{
  if(fd == -1)
    fd = sockfd;
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

void ovtcpsocket_t::set_netpriority(int priority)
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

void ovtcpsocket_t::set_expedited_forwarding_PHB()
{
#ifndef __APPLE__
#ifndef WIN32
  int iptos = IPTOS_DSCP_EF;
  setsockopt(sockfd, IPPROTO_IP, IP_TOS, &iptos, sizeof(iptos));
#endif
#endif
  set_netpriority(6);
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
