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
#include "../tascar/libtascar/include/tscconfig.h"
#include <string.h>
#include <strings.h>

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
  if(statusthread.joinable())
    statusthread.join();
}

void endpoint_list_t::cid_register(stage_device_id_t cid, char* data,
                                   epmode_t mode, const std::string& rver)
{
  if(cid < MAX_STAGE_ID) {
    if(mstat.try_lock()) {
      size_t scid = (size_t)cid;
      memcpy(&(endpoints[scid].ep), &data, sizeof(endpoint_t));
      if(mode != endpoints[cid].mode)
        endpoints[scid].announced = false;
      endpoints[scid].mode = mode;
      endpoints[scid].timeout = CALLERLIST_TIMEOUT;
      endpoints[scid].version = rver;
      mstat.unlock();
    }
  }
}

void endpoint_list_t::cid_setlocalip(stage_device_id_t cid, char* data)
{
  if(cid < MAX_STAGE_ID) {
    memcpy(&(endpoints[cid].localep), data, sizeof(endpoint_t));
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

void endpoint_list_t::cid_set_pubkey(stage_device_id_t cid, char* data,
                                     size_t len)
{
  if(cid < MAX_STAGE_ID) {
    if(len == crypto_box_PUBLICKEYBYTES) {
      if(bcmp(data, endpoints[cid].pubkey, len) != 0) {
        memcpy(endpoints[cid].pubkey, data, len);
        TASCAR::console_log(
            "Updated public key for " + std::to_string((int)cid) + ": \"" +
            bin2base64((uint8_t*)data, crypto_box_PUBLICKEYBYTES) + "\".");
      }
      endpoints[cid].has_pubkey = true;
    } else {
      endpoints[cid].has_pubkey = false;
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
  uint32_t statlogcnt(60000 / pingperiodms);
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
      statlogcnt = 60000 / pingperiodms;
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
  for(const auto& ep : endpoints)
    c += (ep.timeout > 0);
  return c;
}

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
