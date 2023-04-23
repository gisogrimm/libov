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

// ping period time in milliseconds:
//static int pingperiodms = 100;
// period time of ping statistic loggin, in ping periods:
//#define STATLOGPERIOD 600
// timeout of caller actvity, in ping periods:
#define CALLERLIST_TIMEOUT 120

class ep_desc_t {
public:
  ep_desc_t();
  endpoint_t ep;
  endpoint_t localep;
  uint32_t timeout;
  bool announced;
  epmode_t mode;
  double pingt_min;
  double pingt_max;
  double pingt_sum;
  uint32_t pingt_n;
  uint32_t num_received;
  uint32_t num_lost;
  std::string version;
};

class endpoint_list_t {
public:
  endpoint_list_t();
  ~endpoint_list_t();
  void add_endpoint(const endpoint_t& ep);

protected:
  virtual void announce_new_connection(stage_device_id_t cid,
                                       const ep_desc_t& ep){};
  virtual void announce_connection_lost(stage_device_id_t cid){};
  virtual void announce_latency(stage_device_id_t cid, double lmin,
                                double lmean, double lmax, uint32_t received,
                                uint32_t lost){};
  void cid_setpingtime(stage_device_id_t cid, double pingtime);
  void cid_register(stage_device_id_t cid, const endpoint_t& ep, epmode_t mode,
                    const std::string& rver);
  void cid_setlocalip(stage_device_id_t cid, const endpoint_t& ep);
  uint32_t get_num_clients();
  void set_hiresping(bool hr);
  std::vector<ep_desc_t> endpoints;
  int pingperiodms = 2000;

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
