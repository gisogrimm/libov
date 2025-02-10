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

#ifndef CALLERLIST_H
#define CALLERLIST_H

#include "udpsocket.h"
#include <thread>

// timeout of caller actvity, in ping periods:
#define CALLERLIST_TIMEOUT 120

class ep_desc_t {
public:
  ep_desc_t();
  endpoint_t ep;      // public IP address
  endpoint_t localep; // local IP address
  uint32_t timeout = 0;
  bool announced = false;
  epmode_t mode = B_PEER2PEER; // endpoint operation mode
  double pingt_min = 10000.0;
  double pingt_max = 0.0;
  double pingt_sum = 0.0;
  uint32_t pingt_n = 0;
  uint32_t num_received = 0;
  uint32_t num_lost = 0;
  std::string version;
  bool has_pubkey = false;
  uint8_t pubkey[crypto_box_PUBLICKEYBYTES];
  uint64_t padding = 0;
};

class endpoint_list_t {
public:
  endpoint_list_t();
  ~endpoint_list_t();
  void add_endpoint(const endpoint_t& ep);
  void set_hiresping(bool hr);

protected:
  virtual void announce_new_connection(stage_device_id_t cid,
                                       const ep_desc_t& ep){};
  virtual void announce_connection_lost(stage_device_id_t cid){};
  virtual void announce_latency(stage_device_id_t cid, double lmin,
                                double lmean, double lmax, uint32_t received,
                                uint32_t lost){};
  void cid_setpingtime(stage_device_id_t cid, double pingtime);
  void cid_register(stage_device_id_t cid, char* data, epmode_t mode,
                    const std::string& rver);
  void cid_setlocalip(stage_device_id_t cid, char* data);
  void cid_set_pubkey(stage_device_id_t cid, char* data, size_t len);
  uint32_t get_num_clients();
  std::vector<ep_desc_t> endpoints;
  // ping period time in milliseconds:
  int pingperiodms = 2000;
  bool srv_has_pubkey = false;
  uint8_t srv_pubkey[crypto_box_PUBLICKEYBYTES];

private:
  void checkstatus();
  bool runthread;
  std::thread statusthread;
  std::mutex mstat;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
