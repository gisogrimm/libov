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
#include <mutex>
#include <nlohmann/json.hpp>
#include <thread>

#ifdef WIN32

#include <winsock2.h>

#endif

class ov_client_orlandoviols_t : public ov_client_base_t {
public:
  ov_client_orlandoviols_t(ov_render_base_t& backend, const std::string& lobby);
  ~ov_client_orlandoviols_t();
  void start_service();
  void stop_service();
  bool download_file(const std::string& url, const std::string& dest,
                     bool use_pw = true);
  bool is_going_to_stop() const { return quitrequest_; };
  std::string get_owner() const { return owner; };
  void upload_plugin_settings();
  void upload_session_gains();
  void upload_objmix();

private:
  void service();
  // void register_device(std::string url, const std::string& device);
  std::string device_update(std::string url, const std::string& device,
                            std::string& hash, bool show_errors);
  bool report_error(std::string url, const std::string& device,
                    const std::string& msg);
  void parse_config();

  bool runservice;
  std::thread servicethread;
  std::string lobby;
  std::atomic<bool> quitrequest_;
  bool isovbox;
  std::string owner;
  std::string refplugcfg;
  std::mutex curlmtx;
  // system information:
  std::string uname_sysname;
  std::string uname_release;
  std::string uname_machine;
  // device configuration:
  nlohmann::json devcfg;
  // start in mixer mode:
  bool start_mixer = false;

#ifdef WIN32
  WSADATA WSAData; // Structure to store details of the Windows Sockets
                   // implementation
#endif
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
