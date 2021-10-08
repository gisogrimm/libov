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

ovboxclient_t::ovboxclient_t(const std::string& desthost, port_t destport,
                             port_t recport, port_t portoffset, int prio,
                             secret_t secret, stage_device_id_t callerid,
                             bool peer2peer_, bool donotsend_,
                             bool receivedownmix_, bool sendlocal_,
                             double deadline, bool senddownmix, bool usingproxy)
    : prio(prio), secret(secret), remote_server(secret, callerid),
      toport(destport), recport(recport), portoffset(portoffset),
      callerid(callerid), runsession(true), mode(0), cb_ping(nullptr),
      cb_ping_data(nullptr), sendlocal(sendlocal_), last_tx(0), last_rx(0),
      t_bitrate(std::chrono::high_resolution_clock::now()), cb_seqerr(nullptr),
      cb_seqerr_data(nullptr), msgbuffers(new msgbuf_t[MAX_STAGE_ID])
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
  local_server.set_timeout_usec(10000);
  local_server.set_destination("localhost");
  local_server.bind(recport, true);
  remote_server.set_destination(desthost.c_str());
  if(deadline > 0)
    remote_server.set_timeout_usec(1000 * deadline);
  remote_server.bind(0, false);
  localep = getipaddr();
  localep.sin_port = remote_server.getsockep().sin_port;
  sendthread = std::thread(&ovboxclient_t::sendsrv, this);
  recthread = std::thread(&ovboxclient_t::recsrv, this);
  pingthread = std::thread(&ovboxclient_t::pingservice, this);
}

ovboxclient_t::~ovboxclient_t()
{
  runsession = false;
  sendthread.join();
  recthread.join();
  pingthread.join();
  for(auto th = xrecthread.begin(); th != xrecthread.end(); ++th)
    th->join();
  delete[] msgbuffers;
}

void ovboxclient_t::set_expedited_forwarding_PHB()
{
  remote_server.set_expedited_forwarding_PHB();
}

void ovboxclient_t::set_reorder_deadline(double t_ms)
{
  if(t_ms > 0) {
    remote_server.set_timeout_usec(1000 * t_ms);
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
  txrate = sc * (remote_server.tx_bytes - last_tx);
  rxrate = sc * (remote_server.rx_bytes - last_rx);
  t_bitrate = t2;
  last_tx = remote_server.tx_bytes;
  last_rx = remote_server.rx_bytes;
}

void ovboxclient_t::set_ping_callback(
    std::function<void(stage_device_id_t, double, const endpoint_t&, void*)> f,
    void* d)
{
  cb_ping = f;
  cb_ping_data = d;
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
  log(recport,
      "new connection for " + std::to_string(cid) + " from " + ep2str(ep.ep) +
          " in " + ((ep.mode & B_PEER2PEER) ? "peer-to-peer" : "server") +
          "-mode" + ((ep.mode & B_RECEIVEDOWNMIX) ? " receivedownmix" : "") +
          ((ep.mode & B_DONOTSEND) ? " donotsend" : "") + " v" + ep.version);
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
  if(client_stats_announce[cid].ping_p2p.received)
    log(recport, "lat-p2p " + std::to_string(cid) + " " +
                     to_string(client_stats_announce[cid].ping_p2p));
  if(client_stats_announce[cid].ping_srv.received)
    log(recport, "lat-srv " + std::to_string(cid) + " " +
                     to_string(client_stats_announce[cid].ping_srv));
  if(client_stats_announce[cid].ping_loc.received)
    log(recport, "lat-loc " + std::to_string(cid) + " " +
                     to_string(client_stats_announce[cid].ping_loc));
  double data[6];
  data[0] = cid;
  data[1] = client_stats_announce[cid].ping_p2p.t_min;
  data[2] = client_stats_announce[cid].ping_p2p.t_med;
  data[3] = client_stats_announce[cid].ping_p2p.t_p99;
  data[4] = client_stats_announce[cid].ping_p2p.received;
  data[5] = client_stats_announce[cid].ping_p2p.lost;
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
    std::this_thread::sleep_for(std::chrono::milliseconds(PINGPERIODMS));
    // send registration to server:
    remote_server.send_registration(mode, toport, localep);
    // send ping to other peers:
    size_t ocid(0);
    for(auto ep : endpoints) {
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

// this thread receives messages from the server:
void ovboxclient_t::sendsrv()
{
  try {
    set_thread_prio(prio);
    msgbuf_t msg;
    while(runsession) {
      remote_server.recv_sec_msg(msg);
      msgbuf_t* pmsg(&msg);
      while(sorter.process(&pmsg))
        process_msg(*pmsg);
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
  // DEBUG(msg.destport);
  char* tbuf(msg.msg);
  size_t tsize(msg.size);
  if(msg.destport == PORT_PONG_SRV) {
    tbuf += sizeof(stage_device_id_t);
    tsize -= sizeof(stage_device_id_t);
  }
  double tms(get_pingtime(tbuf, tsize));
  // DEBUG(tms);
  if(tms > 0) {
    if(cb_ping)
      cb_ping(msg.cid, tms, msg.sender, cb_ping_data);
    switch(msg.destport) {
    case PORT_PONG:
      ping_stat_collecors_p2p[msg.cid].add_value(tms);
      break;
    case PORT_PONG_SRV:
      tbuf += sizeof(stage_device_id_t);
      tsize -= sizeof(stage_device_id_t);
      ping_stat_collecors_srv[msg.cid].add_value(tms);
      break;
    case PORT_PONG_LOCAL:
      ping_stat_collecors_local[msg.cid].add_value(tms);
      break;
    }
  }
}

void ovboxclient_t::process_msg(msgbuf_t& msg)
{
  msg.valid = false;
  // avoid handling of loopback messages:
  if((msg.cid == callerid) && (msg.destport != PORT_LISTCID))
    return;
  // not a special port, thus we forward data to localhost and proxy
  // clients:
  if(msg.destport > MAXSPECIALPORT) {
    if(msg.destport + portoffset != recport)
      local_server.send(msg.msg, msg.size, msg.destport + portoffset);
    for(auto xd : xdest)
      if(msg.destport + xd != recport)
        local_server.send(msg.msg, msg.size, msg.destport + xd);
    // is this message from same network?
    if(!is_same_network(msg.sender, localep)) {
      // now send to proxy clients:
      for(auto client : proxyclients) {
        if(msg.cid != client.first) {
          client.second.sin_port = htons((unsigned short)msg.destport);
          remote_server.send(msg.msg, msg.size, client.second);
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
      cid_setlocalip(msg.cid, *((endpoint_t*)(msg.msg)));
    }
    break;
  case PORT_LISTCID:
    if(msg.size == sizeof(endpoint_t)) {
      // seq is peer2peer flag:
      cid_register(msg.cid, *((endpoint_t*)(msg.msg)), msg.seq, "");
    }
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
    endpoint_t sender_endpoint;
    log(recport, "listening");
    while(runsession) {
      ssize_t n = local_server.recvfrom(buffer, BUFSIZE, sender_endpoint);
      if(n > 0) {
        size_t un = remote_server.packmsg(msg, BUFSIZE, recport, buffer, n);
        bool sendtoserver(!(mode & B_PEER2PEER));
        if(mode & B_PEER2PEER) {
          // we are in peer-to-peer mode.
          size_t ocid(0);
          for(auto ep : endpoints) {
            if(ep.timeout) {
              // endpoint is active.
              if(ocid != callerid) {
                // not sending to ourself.
                if(ep.mode & B_PEER2PEER) {
                  // other end is in peer-to-peer mode.
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
                      if(sendlocal && target_in_same_network)
                        // same network.
                        remote_server.send(msg, un, ep.localep);
                      else
                        remote_server.send(msg, un, ep.ep);
                    }
                  }
                } else {
                  sendtoserver = true;
                }
              }
            }
            ++ocid;
          }
          //// serve proxy clients:
          // for(auto client : proxyclients) {
          //  // send unencoded message:
          //  client.second.sin_port = htons((unsigned short)recport);
          //  remote_server.send(buffer, n, client.second);
          //}
        }
        if(sendtoserver) {
          remote_server.send(msg, un, toport);
        }
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
    //xlocal_server.bind(srcport, true);
    xlocal_server.bind(srcport, false);
    set_thread_prio(prio);
    char buffer[BUFSIZE];
    char msg[BUFSIZE];
    endpoint_t sender_endpoint;
    log(recport, "listening");
    while(runsession) {
      ssize_t n = xlocal_server.recvfrom(buffer, BUFSIZE, sender_endpoint);
      if(n > 0) {
        size_t un = remote_server.packmsg(msg, BUFSIZE, destport, buffer, n);
        bool sendtoserver(!(mode & B_PEER2PEER));
        if(mode & B_PEER2PEER) {
          // we are in peer-to-peer mode.
          size_t ocid(0);
          for(auto ep : endpoints) {
            if(ep.timeout) {
              // target is active.
              if(ocid != callerid){
                // not sending to ourselfs.
                if(ep.mode & B_PEER2PEER){
                  // target is in peer-to-peer mode.
                  bool target_in_same_network(
                      (endpoints[callerid].ep.sin_addr.s_addr ==
                       ep.ep.sin_addr.s_addr) &&
                      (ep.localep.sin_addr.s_addr != 0));
                  if((!(bool)(ep.mode & B_DONOTSEND)) ||
                     ((bool)(ep.mode & B_USINGPROXY) &&
                      target_in_same_network)) {
                    //if(!(ep.mode & B_DONOTSEND)) {
                    remote_server.send(msg, un, ep.ep);
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
          remote_server.send(msg, un, toport);
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

void ping_stat_collecor_t::add_value(double pt)
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
  std::vector<double> sb(data);
  sb.resize(filled);
  std::sort(sb.begin(), sb.end());
  size_t idx_med(std::round(0.5 * (filled - 1)));
  size_t idx_99(std::round(0.99 * (filled - 1)));
  ps.t_med = sb[idx_med];
  if((filled & 1) == 0) {
    // even number of samples, median is mean of two neighbours
    if(idx_med)
      ps.t_med += sb[idx_med - 1];
    else
      ps.t_med += sb[idx_med + 1];
    ps.t_med *= 0.5;
  }
  ps.t_min = sb[0];
  ps.t_p99 = sb[idx_99];
  ps.t_mean = sum / filled;
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
