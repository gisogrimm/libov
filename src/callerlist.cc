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

#include "callerlist.h"
#include <string.h>

ep_desc_t::ep_desc_t()
{
  memset(&ep, 0, sizeof(ep));
  memset(&localep, 0, sizeof(ep));
  timeout = 0;
  announced = false;
  pingt_min = 10000;
  pingt_max = 0;
  pingt_sum = 0;
  pingt_n = 0;
  num_lost = 0;
  num_received = 0;
  mode = B_PEER2PEER;
  version = "";
}

endpoint_list_t::endpoint_list_t() : runthread(true)
{
  endpoints.resize(MAX_STAGE_ID);
  statusthread = std::thread(&endpoint_list_t::checkstatus, this);
}

endpoint_list_t::~endpoint_list_t()
{
  runthread = false;
  statusthread.join();
}

void endpoint_list_t::cid_register(stage_device_id_t cid, const endpoint_t& ep,
                                   epmode_t mode, const std::string& rver)
{
  if(cid < MAX_STAGE_ID) {
    size_t scid = (size_t)cid;
    DEBUG(scid);
    DEBUG(ep2str(ep));
    DEBUG((int)mode);
    DEBUG(endpoints.size());
    endpoints[scid].ep = ep;
    if(mode != endpoints[cid].mode)
      endpoints[scid].announced = false;
    endpoints[scid].mode = mode;
    endpoints[scid].timeout = CALLERLIST_TIMEOUT;
    endpoints[scid].version = rver;
  }
}

void endpoint_list_t::cid_setlocalip(stage_device_id_t cid,
                                     const endpoint_t& localep)
{
  if(cid < MAX_STAGE_ID) {
    endpoints[cid].localep = localep;
    // workaround for invalidly packed sockaddr structures when
    // receiving from some systems:
    endpoints[cid].localep.sin_family = AF_INET;
  }
}

void endpoint_list_t::cid_setpingtime(stage_device_id_t cid, double pingtime)
{
  if(cid < MAX_STAGE_ID) {
    endpoints[cid].timeout = CALLERLIST_TIMEOUT;
    if(pingtime > 0) {
      if(mstat.try_lock()) {
        ++endpoints[cid].pingt_n;
        endpoints[cid].pingt_sum += pingtime;
        endpoints[cid].pingt_max = std::max(pingtime, endpoints[cid].pingt_max);
        endpoints[cid].pingt_min = std::min(pingtime, endpoints[cid].pingt_min);
        mstat.unlock();
      }
    }
  }
}

void endpoint_list_t::set_hiresping(bool hr)
{
  if(hr)
    pingperiodms = 50;
  else
    pingperiodms = 500;
}

void endpoint_list_t::checkstatus()
{
  uint32_t statlogcnt(60000/pingperiodms);
  while(runthread) {
    std::this_thread::sleep_for(std::chrono::milliseconds(pingperiodms));
    for(stage_device_id_t ep = 0; ep != MAX_STAGE_ID; ++ep) {
      if(endpoints[ep].timeout) {
        // bookkeeping of connected endpoints:
        if(!endpoints[ep].announced) {
          announce_new_connection(ep, endpoints[ep]);
          endpoints[ep].announced = true;
        }
        --endpoints[ep].timeout;
      } else {
        // bookkeeping of disconnected endpoints:
        if(endpoints[ep].announced) {
          announce_connection_lost(ep);
          endpoints[ep] = ep_desc_t();
        }
      }
    }
    if(!statlogcnt) {
      // logging of ping statistics:
      statlogcnt = 60000/pingperiodms;
      std::lock_guard<std::mutex> lk(mstat);
      for(stage_device_id_t ep = 0; ep != MAX_STAGE_ID; ++ep) {
        if(endpoints[ep].timeout) {
          announce_latency(ep, endpoints[ep].pingt_min,
                           endpoints[ep].pingt_sum /
                               std::max(1u, endpoints[ep].pingt_n),
                           endpoints[ep].pingt_max, endpoints[ep].num_received,
                           endpoints[ep].num_lost);
          endpoints[ep].pingt_n = 0;
          endpoints[ep].pingt_min = 1000;
          endpoints[ep].pingt_max = 0;
          endpoints[ep].pingt_sum = 0.0;
          endpoints[ep].num_received = 0;
          endpoints[ep].num_lost = 0;
        }
      }
    }
    --statlogcnt;
  }
}

uint32_t endpoint_list_t::get_num_clients()
{
  uint32_t c(0);
  for(auto ep : endpoints)
    c += (ep.timeout > 0);
  return c;
}

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
