/*
 * This file is part of the ovbox software tool, see <http://orlandoviols.com/>.
 *
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

#ifndef OV_CLIENT_ORLANDOVIOLS
#define OV_CLIENT_ORLANDOVIOLS

#include "ov_types.h"
#include <atomic>
#include <thread>

class ov_client_orlandoviols_t : public ov_client_base_t {
public:
  ov_client_orlandoviols_t(ov_render_base_t& backend, const std::string& lobby);
  void start_service();
  void stop_service();
  bool download_file(const std::string& url, const std::string& dest);
  bool is_going_to_stop() const { return quitrequest_; };
  std::string get_owner() const { return owner; };
  void upload_plugin_settings();

private:
  void service();
  void register_device(std::string url, const std::string& device);
  std::string device_update(std::string url, const std::string& device,
                            std::string& hash);
  bool report_error(std::string url, const std::string& device,
                    const std::string& msg);

  bool runservice;
  std::thread servicethread;
  std::string lobby;
  std::atomic<bool> quitrequest_;
  bool isovbox;
  std::string owner;
  std::string refplugcfg;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
