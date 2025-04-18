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

#include "ovboxclient.h"
#include "../tascar/libtascar/include/tscconfig.h"
#include <algorithm>
#include <condition_variable>
#include <string.h>
#include <strings.h>
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

#include "errmsg.h"
#include <cmath>
#include <errno.h>

/**
 * @defgroup proxymode Proxy mode
 *
 * Reduce external network load in case of multiple devices in the
 * same network.
 *
 * Devices which use the proxy mode (flag
 * ov_render_tascar_t::use_proxy is true) and which have a proxy
 * device (flag ov_render_tascar_t::is_proxy) in the same network and
 * session receive data only from the proxy, as well as from other
 * devices in the local network.
 *
 * A list of proxy clients is provided by the configuration server,
 * and is stored in ovboxclient_t::proxyclients using the function
 * ovboxclient_t::add_proxy_client(). Incoming unencoded messages are
 * processed in function ovboxclient_t::recsrv(). Here they are
 * forwarded to all peer-to-peer mode devices which don't have sending
 * deactivated, which are not using proxy mode or are in the same
 * network. In a proxy device, incoming encoded messages are sent
 * locally as unencoded messages, and forwarded as unencoded messages
 * to the proxy clients if the message arrived from outside the local
 * network, see ovboxclient_t::process_msg().
 */

/**
 * @defgroup secondarydev Secondary client on same device
 *
 * Start ov-client as secondary client on a device.
 *
 * Creating a secondary client can be useful for routing signals from
 * one session into another session. Starting multiple instances of
 * ov-client requires to shift all UDP ports by an offset. This needs
 * to be compensated for in ovboxclient_t, to allow communication with
 * other clients. Specifically, the offset is added to the port number
 * for all incoming messages, and subtracted for all outgoing
 * messages.
 */

ovboxclient_t::ovboxclient_t(std::string desthost, port_t destport,
                             port_t recport, port_t portoffset, int prio,
                             secret_t secret, stage_device_id_t callerid,
                             bool peer2peer_, bool donotsend_,
                             bool receivedownmix_, bool sendlocal_,
                             double deadline, bool senddownmix, bool usingproxy,
                             bool use_tcp_tunnel, bool encryption)
    : prio(prio), remote_server(secret, callerid), toport(destport),
      recport(recport), portoffset(portoffset), callerid(callerid),
      runsession(true), mode(0), sendlocal(sendlocal_), last_tx(0), last_rx(0),
      t_bitrate(std::chrono::high_resolution_clock::now()), cb_seqerr(nullptr),
      cb_seqerr_data(nullptr)
{
  if(peer2peer_)
    mode |= B_PEER2PEER;
  if(receivedownmix_)
    mode |= B_RECEIVEDOWNMIX;
  if(donotsend_)
    mode |= B_DONOTSEND;
  if(senddownmix)
    mode |= B_SENDDOWNMIX;
  if(usingproxy)
    mode |= B_USINGPROXY;
  if(encryption)
    mode |= B_ENCRYPTION;
  DEBUG(encryption);
  local_server.set_timeout_usec(10000);
  local_server.set_destination("localhost");
  local_server.bind(recport, true);
  // setup connection to relay server:
  endpoint_t ep_tcptunnel = ovgethostbyname(desthost);
  ep_tcptunnel.sin_port = htons(destport);
  if(use_tcp_tunnel && (!peer2peer_))
    desthost = "127.0.0.1";
  remote_server.set_destination(desthost.c_str());
  if(deadline > 0.0)
    remote_server.set_timeout_usec((int)(1000.0 * std::min(1e3, deadline)));
  port_t local_relay_port = remote_server.bind(0, false);
  if(use_tcp_tunnel && (!peer2peer_)) {
    tcp_tunnel = new ovtcpsocket_t(0);
    try {
      toport = tcp_tunnel->connect(ep_tcptunnel, local_relay_port);
    }
    catch(...) {
      delete tcp_tunnel;
      throw;
    }
  }
  localep = getipaddr();
  localep.sin_port = remote_server.getsockep().sin_port;
  sendthread = std::thread(&ovboxclient_t::sendsrv, this);
  recthread = std::thread(&ovboxclient_t::recsrv, this);
  pingthread = std::thread(&ovboxclient_t::pingservice, this);
}

ovboxclient_t::~ovboxclient_t()
{
  runsession = false;
  if(sendthread.joinable())
    sendthread.join();
  if(recthread.joinable())
    recthread.join();
  if(pingthread.joinable())
    pingthread.join();
  if(!xrecthread.empty()) {
    for(auto& th : xrecthread) {
      if(th.joinable())
        th.join();
    }
  }
  if(tcp_tunnel) {
    delete tcp_tunnel;
  }
}

void ovboxclient_t::set_expedited_forwarding_PHB()
{
  remote_server.set_expedited_forwarding_PHB();
}

void ovboxclient_t::set_reorder_deadline(double t_ms)
{
  if(t_ms > 0) {
    remote_server.set_timeout_usec((int)(1000.0 * std::min(1e3, t_ms)));
    DEBUG(t_ms);
  }
}

void ovboxclient_t::getbitrate(double& txrate, double& rxrate)
{
  std::chrono::high_resolution_clock::time_point t2(
      std::chrono::high_resolution_clock::now());
  std::chrono::duration<double> time_span(
      std::chrono::duration_cast<std::chrono::duration<double>>(t2 -
                                                                t_bitrate));
  double sc(8.0 / std::max(1e-6, time_span.count()));
  txrate = sc * ((double)(remote_server.tx_bytes) - (double)last_tx);
  rxrate = sc * ((double)(remote_server.rx_bytes) - (double)last_rx);
  t_bitrate = t2;
  last_tx = remote_server.tx_bytes;
  last_rx = remote_server.rx_bytes;
}

void ovboxclient_t::set_ping_callback(
    std::function<void(stage_device_id_t, port_t, double, const endpoint_t&,
                       void*)>
        f,
    void* d)
{
  cb_ping = f;
  cb_ping_data = d;
}

void ovboxclient_t::set_latreport_callback(latreport_cb_t f, void* d)
{
  cb_latreport = f;
  cb_latreport_data = d;
}

void ovboxclient_t::set_seqerr_callback(
    std::function<void(stage_device_id_t, sequence_t, sequence_t, port_t,
                       void*)>
        f,
    void* d)
{
  cb_seqerr = f;
  cb_seqerr_data = d;
}

void ovboxclient_t::add_receiverport(port_t srcxport, port_t destxport)
{
  xrecthread.emplace_back(
      std::thread(&ovboxclient_t::xrecsrv, this, srcxport, destxport));
}

void ovboxclient_t::add_extraport(port_t dest)
{
  xdest.push_back(dest);
}

void ovboxclient_t::add_proxy_client(stage_device_id_t cid,
                                     const std::string& host)
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
  proxyclients[cid] = serv_addr;
}

void ovboxclient_t::announce_new_connection(stage_device_id_t cid,
                                            const ep_desc_t& ep)
{
  if(cid == callerid)
    return;
  log(recport, "new connection for " + std::to_string(cid) + " from " +
                   ep2str(ep.ep) + " [" + ep2str(ep.localep) + "] in " +
                   ((ep.mode & B_PEER2PEER) ? "p2p" : "srv") + "-mode" +
                   ((ep.mode & B_RECEIVEDOWNMIX) ? " receivedownmix" : "") +
                   ((ep.mode & B_DONOTSEND) ? " donotsend" : "") + " v" +
                   ep.version);
}

void ovboxclient_t::announce_connection_lost(stage_device_id_t cid)
{
  if(cid == callerid)
    return;
  log(recport, "connection for " + std::to_string(cid) + " lost.");
}

void ovboxclient_t::announce_latency(stage_device_id_t cid, double, double,
                                     double, uint32_t, uint32_t)
{
  if(cid == callerid)
    return;
  update_client_stats(cid, client_stats_announce[cid]);
  log(recport, "packages " + std::to_string(cid) + " " +
                   to_string(client_stats_announce[cid].packages));
  if(client_stats_announce[cid].ping_p2p.received) {
    log(recport, "lat-p2p " + std::to_string(cid) + " " +
                     to_string(client_stats_announce[cid].ping_p2p));
    if(cb_latreport)
      cb_latreport(cid, "p2p", client_stats_announce[cid].ping_p2p,
                   cb_latreport_data);
  }
  if(client_stats_announce[cid].ping_srv.received) {
    log(recport, "lat-srv " + std::to_string(cid) + " " +
                     to_string(client_stats_announce[cid].ping_srv));
    if(cb_latreport)
      cb_latreport(cid, "srv", client_stats_announce[cid].ping_srv,
                   cb_latreport_data);
  }
  if(client_stats_announce[cid].ping_loc.received) {
    log(recport, "lat-loc " + std::to_string(cid) + " " +
                     to_string(client_stats_announce[cid].ping_loc));
    if(cb_latreport)
      cb_latreport(cid, "loc", client_stats_announce[cid].ping_loc,
                   cb_latreport_data);
  }
  double data[6];
  data[0] = cid;
  data[1] = client_stats_announce[cid].ping_p2p.t_min;
  data[2] = client_stats_announce[cid].ping_p2p.t_med;
  data[3] = client_stats_announce[cid].ping_p2p.t_p99;
  data[4] = (double)(client_stats_announce[cid].ping_p2p.received);
  data[5] = (double)(client_stats_announce[cid].ping_p2p.lost);
  remote_server.pack_and_send(PORT_PEERLATREP, (const char*)data,
                              6 * sizeof(double), toport);
}

void ovboxclient_t::update_client_stats(stage_device_id_t cid,
                                        client_stats_t& stats)
{
  stats.packages = sorter.get_stat(cid);
  message_stat_t ostat(stats.state_packages);
  stats.state_packages = stats.packages;
  stats.packages -= ostat;
  if(stats.packages.lost > (1 << 30))
    stats.packages.lost = 0;
  ping_stat_collecors_p2p[cid].update_ping_stat(stats.ping_p2p);
  ping_stat_collecors_srv[cid].update_ping_stat(stats.ping_srv);
  ping_stat_collecors_local[cid].update_ping_stat(stats.ping_loc);
}

void ovboxclient_t::handle_endpoint_list_update(stage_device_id_t cid,
                                                const endpoint_t& ep)
{
  DEBUG(cid);
  DEBUG(ep2str(ep));
}

// ping service
void ovboxclient_t::pingservice()
{
  while(runsession) {
    std::this_thread::sleep_for(std::chrono::milliseconds(pingperiodms));
    // send registration to relay server:
    remote_server.send_registration(mode, toport, localep);
    // send ping to other peers:
    uint8_t ocid(0);
    for(auto& ep : endpoints) {
      if(ep.timeout && (ocid != callerid)) {
        remote_server.send_ping(ep.ep, ocid);
        ++ping_stat_collecors_p2p[ocid].sent;
        remote_server.send_ping(remote_server.get_destination(), ocid,
                                PORT_PING_SRV);
        ++ping_stat_collecors_srv[ocid].sent;
        // test if peer is in same network:
        if((endpoints[callerid].ep.sin_addr.s_addr == ep.ep.sin_addr.s_addr) &&
           (ep.localep.sin_addr.s_addr != 0)) {
          remote_server.send_ping(ep.localep, ocid, PORT_PING_LOCAL);
          ++ping_stat_collecors_local[ocid].sent;
        }
      }
      ++ocid;
    }
  }
}

// this thread receives messages from the session server and the peers:
void ovboxclient_t::sendsrv()
{
  try {
    set_thread_prio(prio);
    msgbuf_t msg;
    while(runsession) {
      bool can_process = remote_server.recv_sec_msg(msg);
      if(can_process) {
        msgbuf_t* pmsg(&msg);
        while(sorter.process(&pmsg))
          process_msg(*pmsg);
      }
    }
  }
  catch(const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    runsession = false;
  }
}

void ovboxclient_t::process_ping_msg(msgbuf_t& msg)
{
  stage_device_id_t cid(msg.cid);
  msg_callerid(msg.rawbuffer) = callerid;
  switch(msg.destport) {
  case PORT_PING:
    // we received a ping message, so we just send it back as a pong
    // message and with our own stage device id:
    msg_port(msg.rawbuffer) = PORT_PONG;
    break;
  case PORT_PING_SRV:
    // we received a ping message via server so we just send it back as a pong
    // message and with our own stage device id:
    msg_port(msg.rawbuffer) = PORT_PONG_SRV;
    *((stage_device_id_t*)(msg.msg)) = cid;
    break;
  case PORT_PING_LOCAL:
    // we received a ping message, so we just send it back as a pong
    // message and with our own stage device id:
    msg_port(msg.rawbuffer) = PORT_PONG_LOCAL;
    break;
  }
  remote_server.send(msg.rawbuffer, msg.size + HEADERLEN, msg.sender);
}

void ovboxclient_t::process_pong_msg(msgbuf_t& msg)
{
  char* tbuf(msg.msg);
  size_t tsize(msg.size);
  if(msg.destport == PORT_PONG_SRV) {
    tbuf += sizeof(stage_device_id_t);
    tsize -= sizeof(stage_device_id_t);
  }
  double tms(remote_server.get_pingtime(tbuf, tsize));
  if(tms > 0) {
    if(cb_ping)
      cb_ping(msg.cid, msg.destport, tms, msg.sender, cb_ping_data);
    switch(msg.destport) {
    case PORT_PONG:
      ping_stat_collecors_p2p[msg.cid].add_value((float)tms);
      break;
    case PORT_PONG_SRV:
      ping_stat_collecors_srv[msg.cid].add_value((float)tms);
      break;
    case PORT_PONG_LOCAL:
      ping_stat_collecors_local[msg.cid].add_value((float)tms);
      break;
    }
  }
}

void ovboxclient_t::process_msg(msgbuf_t& msg)
{
  msg.valid = false;
  // avoid handling of loopback messages:
  if((msg.cid == callerid) && (msg.destport != PORT_LISTCID) &&
     (msg.destport != PORT_PUBKEY))
    return;
  // not a special port, thus we forward data to localhost and proxy
  // clients:
  if(msg.destport > MAXSPECIALPORT) {
    char* send_msg = msg.msg;
    size_t send_len = msg.size;
    if((msg.cid < MAX_STAGE_ID) && (endpoints[msg.cid].mode & B_ENCRYPTION) &&
       (mode & B_ENCRYPTION)) {
      if(crypto_box_seal_open((uint8_t*)(decrypted_msg.msg),
                              (uint8_t*)(msg.msg), msg.size,
                              remote_server.recipient_public,
                              remote_server.recipient_secret) == 0) {
        // success:
        send_msg = decrypted_msg.msg;
        send_len = msg.size - crypto_box_SEALBYTES;
      }
    }
    if(msg.destport + portoffset != recport)
      // forward to local UDP receivers (zita etc.), add portoffset:
      local_server.send(send_msg, send_len,
                        (uint16_t)(msg.destport + portoffset));
    for(auto xd : xdest)
      if(msg.destport + xd != recport)
        local_server.send(send_msg, send_len, (uint16_t)(msg.destport + xd));
    // is this message from same network?
    if(!is_same_network(msg.sender, localep)) {
      // now send to proxy clients:
      for(auto& client : proxyclients) {
        if(msg.cid != client.first) {
          client.second.sin_port = htons((unsigned short)msg.destport);
          remote_server.send(send_msg, send_len, client.second);
        }
      }
    }
    return;
  }
  switch(msg.destport) {
  case PORT_PING:
  case PORT_PING_SRV:
  case PORT_PING_LOCAL:
    process_ping_msg(msg);
    break;
  case PORT_PONG:
  case PORT_PONG_SRV:
  case PORT_PONG_LOCAL:
    process_pong_msg(msg);
    break;
  case PORT_SETLOCALIP:
    // we received the local IP address of a peer:
    if(msg.size == sizeof(endpoint_t)) {
      cid_setlocalip(msg.cid, msg.msg);
    }
    break;
  case PORT_LISTCID:
    if(msg.size == sizeof(endpoint_t)) {
      // seq is peer2peer flag:
      if((msg.seq >= 0) && (msg.seq < 256))
        cid_register(msg.cid, msg.msg, (uint8_t)(msg.seq), "");
    }
    break;
  case PORT_PUBKEY:
    cid_set_pubkey(msg.cid, msg.msg, msg.size);
    break;
  }
}

// this thread receives local UDP messages and handles them:
void ovboxclient_t::recsrv()
{
  try {
    set_thread_prio(prio);
    char buffer[BUFSIZE];
    char msg[BUFSIZE];
    char cmsg[BUFSIZE + crypto_box_SEALBYTES];
    endpoint_t sender_endpoint;
    log(recport, "listening");
    while(runsession) {
      ssize_t n = local_server.recvfrom(buffer, BUFSIZE, sender_endpoint);
      if(n > 0) {
        // subtract port offset before forwarding to remote peers:
        size_t msglen_packed = remote_server.packmsg(
            msg, BUFSIZE, (uint16_t)(recport - portoffset), buffer, n);
        bool sendtoserver(!(mode & B_PEER2PEER));
        uint32_t peers_total = 0;
        uint32_t peers_encrypted = 0;
        if(mode & B_PEER2PEER) {
          // we are in peer-to-peer mode.
          size_t ocid(0);
          for(auto& ep : endpoints) {
            if(ep.timeout) {
              // target endpoint is active.
              if(ocid != callerid) {
                // not sending to ourself.
                if(ep.mode & B_PEER2PEER) {
                  // target/other end is in peer-to-peer mode.
                  char* send_msg = msg;
                  size_t send_len = msglen_packed;
                  ++peers_total;
                  // now check for encryption:
                  if((mode & B_ENCRYPTION) && (ep.mode & B_ENCRYPTION) &&
                     ep.has_pubkey) {
                    send_len = encryptmsg(cmsg, BUFSIZE, msg, msglen_packed,
                                          ep.pubkey);
                    send_msg = cmsg;
                    ++peers_encrypted;
                  }
                  bool target_in_same_network(
                      (endpoints[callerid].ep.sin_addr.s_addr ==
                       ep.ep.sin_addr.s_addr) &&
                      (ep.localep.sin_addr.s_addr != 0));
                  if((!(bool)(ep.mode & B_DONOTSEND)) ||
                     ((bool)(ep.mode & B_USINGPROXY) &&
                      target_in_same_network)) {
                    // sending is not deactivated.
                    if((bool)(ep.mode & B_RECEIVEDOWNMIX) ==
                       (bool)(mode & B_SENDDOWNMIX)) {
                      // remote is receiving downmix and this is downmixer
                      if(sendlocal && target_in_same_network) {
                        // same network.
                        remote_server.send(send_msg, send_len, ep.localep);
                      } else
                        remote_server.send(send_msg, send_len, ep.ep);
                    }
                  }
                } else {
                  // ep is not B_PEER2PEER
                  sendtoserver = true;
                }
              } // ocid != callerid
            }   // ep.timeout
            ++ocid;
          } // for( ep : endpoints )
        }   // this is not B_PEER2PEER
        if(sendtoserver) {
          // encrypt if needed:
          char* send_msg = msg;
          size_t send_len = msglen_packed;
          ++peers_total;
          // now check for encryption:
          if((mode & B_ENCRYPTION) && srv_has_pubkey) {
            send_len = encryptmsg(cmsg, BUFSIZE, msg, msglen_packed,
                                  srv_pubkey);
            send_msg = cmsg;
            ++peers_encrypted;
          }
          remote_server.send(send_msg, send_len, toport);
        }
        send_encrypt_any = (peers_encrypted > 0);
        send_encrypt_all = send_encrypt_any && (peers_encrypted == peers_total);
      }
    }
  }
  catch(const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    runsession = false;
  }
}

// this thread receives local UDP messages and handles them:
void ovboxclient_t::xrecsrv(port_t srcport, port_t destport)
{
  try {
    udpsocket_t xlocal_server;
    xlocal_server.set_timeout_usec(100000);
    xlocal_server.set_destination("localhost");
    // xlocal_server.bind(srcport, true);
    xlocal_server.bind(srcport, false);
    set_thread_prio(prio);
    char buffer[BUFSIZE];
    char msg[BUFSIZE];
    char cmsg[BUFSIZE + crypto_box_SEALBYTES];
    endpoint_t sender_endpoint;
    log(recport, "listening");
    while(runsession) {
      ssize_t n = xlocal_server.recvfrom(buffer, BUFSIZE, sender_endpoint);
      if(n > 0) {
        size_t msglen_packed =
            remote_server.packmsg(msg, BUFSIZE, destport, buffer, n);
        bool sendtoserver(!(mode & B_PEER2PEER));
        if(mode & B_PEER2PEER) {
          // we are in peer-to-peer mode.
          size_t ocid(0);
          for(auto& ep : endpoints) {
            if(ep.timeout) {
              // target endpoint is active.
              if(ocid != callerid) {
                // not sending to ourself.
                if(ep.mode & B_PEER2PEER) {
                  // target/other end is in peer-to-peer mode.
                  char* send_msg = msg;
                  size_t send_len = msglen_packed;
                  // now check for encryption:
                  if((mode & B_ENCRYPTION) && (ep.mode & B_ENCRYPTION) &&
                     ep.has_pubkey) {
                    send_len = encryptmsg(cmsg, BUFSIZE, msg, msglen_packed,
                                          ep.pubkey);
                    send_msg = cmsg;
                  }
                  bool target_in_same_network(
                      (endpoints[callerid].ep.sin_addr.s_addr ==
                       ep.ep.sin_addr.s_addr) &&
                      (ep.localep.sin_addr.s_addr != 0));
                  if((!(bool)(ep.mode & B_DONOTSEND)) ||
                     ((bool)(ep.mode & B_USINGPROXY) &&
                      target_in_same_network)) {
                    if(sendlocal && target_in_same_network)
                      // same network.
                      remote_server.send(send_msg, send_len, ep.localep);
                    else
                      remote_server.send(send_msg, send_len, ep.ep);
                  } else {
                    sendtoserver = true;
                  }
                }
              }
            }
            ++ocid;
          }
        }
        if(sendtoserver) {
          remote_server.send(msg, msglen_packed, toport);
        }
      }
    }
  }
  catch(const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    runsession = false;
  }
}

bool message_sorter_t::process(msgbuf_t** ppmsg)
{
  if((*ppmsg)->valid) {
    msgbuf_t* pmsg(*ppmsg);
    // handle special ports separately:
    if(pmsg->destport <= MAXSPECIALPORT) {
      pmsg->valid = false;
      return true;
    }
    // we received a message, check for sequence order
    ++stat[pmsg->cid].received;
    bool notfirst(seq_in[pmsg->cid].find(pmsg->destport) !=
                  seq_in[pmsg->cid].end());
    // get input sequence difference:
    sequence_t dseq_in(deltaseq(seq_in, *pmsg));
    sequence_t dseq_io(deltaseq_const(seq_out, *pmsg));
    if((dseq_in != 0) && notfirst)
      stat[pmsg->cid].lost += dseq_in - 1;
    // dropout:
    if((dseq_in > 1) && (dseq_io > 1)) {
      buf1.copy(*pmsg);
      (*ppmsg)->valid = false;
      return false;
    }
    stat[pmsg->cid].seqerr_in += (dseq_in < 0);
    if((dseq_in < -1) || ((dseq_io > 1) && (dseq_in > 0))) {
      if(buf1.valid && (buf1.cid == pmsg->cid) &&
         (buf1.destport == pmsg->destport) && (buf1.seq < pmsg->seq)) {
        buf2.copy(*pmsg);
        *ppmsg = &buf1;
        buf1.valid = false;
        sequence_t dseq_out(deltaseq(seq_out, buf1));
        stat[pmsg->cid].seqerr_out += (dseq_out < 0);
        return true;
      }
    }
    sequence_t dseq_out(deltaseq(seq_out, *pmsg));
    pmsg->valid = false;
    stat[pmsg->cid].seqerr_out += (dseq_out < 0);
    return true;
  }
  if(buf1.valid) {
    *ppmsg = &buf1;
    sequence_t dseq_out(deltaseq(seq_out, buf1));
    buf1.valid = false;
    stat[buf1.cid].seqerr_out += (dseq_out < 0);
    return true;
  }
  if(buf2.valid) {
    sequence_t dseq_out(deltaseq(seq_out, buf2));
    *ppmsg = &buf2;
    buf2.valid = false;
    stat[buf2.cid].seqerr_out += (dseq_out < 0);
    return true;
  }
  return false;
}

message_stat_t message_sorter_t::get_stat(stage_device_id_t id)
{
  return stat[id];
}

ping_stat_collecor_t::ping_stat_collecor_t(size_t N)
    : sent(0), received(0), data(N, 0.0), idx(0), filled(0), sum(0.0)
{
}

void ping_stat_collecor_t::add_value(float pt)
{
  ++received;
  sum -= data[idx];
  data[idx] = pt;
  sum += pt;
  ++idx;
  if(idx >= data.size())
    idx = 0;
  if(filled < data.size())
    ++filled;
}

void ping_stat_collecor_t::update_ping_stat(ping_stat_t& ps) const
{
  ps.t_min = -1.0;
  ps.t_med = -1.0;
  ps.t_p99 = -1.0;
  ps.t_mean = -1.0;
  ps.received = received - ps.state_received;
  ps.lost = sent - ps.state_sent;
  ps.lost -= std::min(ps.received, ps.lost);
  ps.state_sent = sent;
  ps.state_received = received;
  if(!filled)
    return;
  std::vector<float> sb(data);
  sb.resize(filled);
  std::sort(sb.begin(), sb.end());
  size_t idx_med((size_t)(std::round(0.5f * ((float)filled - 1.0))));
  size_t idx_99((size_t)(std::round(0.99f * ((float)filled - 1.0))));
  ps.t_med = sb[idx_med];
  if((filled & 1) == 0) {
    // even number of samples, median is mean of two neighbours
    if(idx_med)
      ps.t_med += sb[idx_med - 1];
    else
      ps.t_med += sb[idx_med + 1];
    ps.t_med *= 0.5f;
  }
  ps.t_min = sb[0];
  ps.t_p99 = sb[idx_99];
  ps.t_mean = sum / (float)filled;
  return;
}

std::string to_string(const ping_stat_t& ps)
{
  char ctmp[1024];
  sprintf(ctmp, "min=%1.2fms median=%1.2fms p99=%1.2fms mean=%1.2fms received=",
          ps.t_min, ps.t_med, ps.t_p99, ps.t_mean);
  return ctmp + std::to_string(ps.received) +
         " lost=" + std::to_string(ps.lost);
}

std::string to_string(const message_stat_t& ms)
{
  char ctmp[1024];
  sprintf(ctmp, " (%1.2f%%) seqerr=",
          100.0 * (double)ms.lost /
              (double)(std::max((size_t)1, ms.received + ms.lost)));
  return "received=" + std::to_string(ms.received) +
         " lost=" + std::to_string(ms.lost) + ctmp +
         std::to_string(ms.seqerr_in) +
         " recovered=" + std::to_string(ms.seqerr_in - ms.seqerr_out);
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
