#ifndef OVBOXCLIENT
#define OVBOXCLIENT

#include "callerlist.h"
#include <functional>

class message_stat_t {
public:
  message_stat_t();
  void reset();
  void operator+=(const message_stat_t&);
  void operator-=(const message_stat_t&);
  size_t received;
  size_t lost;
  size_t seqerr_in;
  size_t seqerr_out;
};

class ping_stat_t {
public:
  ping_stat_t(size_t N = 2048);
  void add_value(double pt);
  std::vector<double> get_min_med_99_mean_lost() const;
  size_t sent;
  size_t received;

private:
  std::vector<double> data;
  size_t idx;
  size_t filled;
  double sum;
};

/**
 * Sort out-of-order messages.
 *
 * This class tries to sort out-of-order messages. It can re-order
 * swapped messages (e.g., series like 1-2-4-3-5), if the missing
 * message is arriving within a certain time.
 */
class message_sorter_t {
public:
  bool process(msgbuf_t** msg);
  message_stat_t get_stat(stage_device_id_t id);

private:
  inline sequence_t deltaseq(std::map<stage_device_id_t, sequence_map_t>& seq,
                             const msgbuf_t& msg)
  {
    sequence_t dseq_(msg.seq - seq[msg.cid][msg.destport]);
    seq[msg.cid][msg.destport] = msg.seq;
    return dseq_;
  };
  inline sequence_t
  deltaseq_const(std::map<stage_device_id_t, sequence_map_t>& seq,
                 const msgbuf_t& msg)
  {
    return msg.seq - seq[msg.cid][msg.destport];
  };
  std::map<stage_device_id_t, sequence_map_t> seq_in;
  std::map<stage_device_id_t, sequence_map_t> seq_out;
  msgbuf_t buf1;
  msgbuf_t buf2;
  std::map<stage_device_id_t, message_stat_t> stat;
};

/**
   Main communication between ovboxclient and relay server.

   This class implements the communication protocol between relay
   server and ov-clients. Its counterpart on the server side is
   ov_server_t from package ov-server.
 */
class ovboxclient_t : public endpoint_list_t {
public:
  /**

     \param desthost hostname or IP address of relay server
     \param destport port number of relay server
     \param recport port of local receiver
     \param portoffset offset added to recport for sending
     \param prio thread priority of sending and receiving threads
     \param secret access code of relay server session
     \param callerid ID of caller (unique number of device in stage, max 30)
     \param peer2peer use peer-to-peer mode for this client
     \param donotsend do not request data messages from server or peers (e.g.,
     in proxy mode) \param downmixonly send downmix only, not individual tracks
     (not yet fully implemented) \param sendlocal allow sending to local IP
     address if in same network
   */
  ovboxclient_t(const std::string& desthost, port_t destport, port_t recport,
                port_t portoffset, int prio, secret_t secret,
                stage_device_id_t callerid, bool peer2peer, bool donotsend,
                bool downmixonly, bool sendlocal);
  virtual ~ovboxclient_t();
  void announce_new_connection(stage_device_id_t cid, const ep_desc_t& ep);
  void announce_connection_lost(stage_device_id_t cid);
  void announce_latency(stage_device_id_t cid, double lmin, double lmean,
                        double lmax, uint32_t received, uint32_t lost);
  void add_extraport(port_t dest);
  /**
     \brief Add a proxy client

     \param cid Device ID of proxy client
     \param host Hostname or IP address of proxy client

     If proxy clients are added, then ovboxclient_t will send all data
     not only to localhost but also to the list of clients. Also the
     own audio will be forwarded to the proxy clients.
   */
  void add_proxy_client(stage_device_id_t cid, const std::string& host);
  void add_receiverport(port_t srcport_t, port_t destport_t);
  void set_ping_callback(
      std::function<void(stage_device_id_t, double, const endpoint_t&, void*)>
          f,
      void* d);
  void getbitrate(double& txrate, double& rxrate);
  void set_seqerr_callback(std::function<void(stage_device_id_t, sequence_t,
                                              sequence_t, port_t, void*)>
                               cb,
                           void* data);

private:
  void sendsrv();
  void recsrv();
  void xrecsrv(port_t srcport, port_t destport);
  void pingservice();
  void handle_endpoint_list_update(stage_device_id_t cid, const endpoint_t& ep);
  void process_msg(msgbuf_t& msg);
  // real time priority:
  const int prio;
  // PIN code to connect to server:
  secret_t secret;
  // data relay server address:
  ovbox_udpsocket_t remote_server;
  // local UDP receiver:
  udpsocket_t local_server;
  // additional port offsets to send data to locally:
  std::vector<port_t> xdest;
  // list of proxy clients:
  std::map<stage_device_id_t, endpoint_t> proxyclients;
  // destination port of relay server:
  port_t toport;
  // receiver ports:
  port_t recport;
  // port offset for primary port, added to nominal port, e.g., in case of local
  // setup:
  port_t portoffset;
  // client/caller identification (aka 'chair' in the lobby system):
  stage_device_id_t callerid;
  bool runsession;
  std::thread sendthread;
  std::thread recthread;
  std::thread pingthread;
  std::vector<std::thread> xrecthread;
  epmode_t mode;
  endpoint_t localep;
  std::function<void(stage_device_id_t, double, const endpoint_t&, void*)>
      cb_ping;
  void* cb_ping_data;
  bool sendlocal;
  size_t last_tx;
  size_t last_rx;
  std::chrono::high_resolution_clock::time_point t_bitrate;
  std::function<void(stage_device_id_t sender, sequence_t expected,
                     sequence_t received, port_t destport, void* data)>
      cb_seqerr;
  void* cb_seqerr_data;
  msgbuf_t* msgbuffers;
  message_sorter_t sorter;
  std::map<stage_device_id_t, message_stat_t> stats;
  std::map<stage_device_id_t, ping_stat_t> pingstats_p2p;
  std::map<stage_device_id_t, ping_stat_t> pingstats_srv;
  std::map<stage_device_id_t, ping_stat_t> pingstats_local;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
