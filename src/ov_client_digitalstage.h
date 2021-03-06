/*
 * This file is part of the ovbox software tool, see <http://orlandoviols.com/>.
 *
 * Copyright (c) 2021 delude88, Giso Grimm, Tobias Hegemann
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

#ifndef DS_SERVICE
#define DS_SERVICE

#include "ds_store.h"
#include "ds_types.h"
#include "ov_types.h"
#include "soundcardtools.h"
#include <atomic>
#include <cpprest/ws_client.h>
#include <mutex>
#include <string>
#include <thread>

using namespace web::websockets::client;

class ov_client_digitalstage_t : public ov_client_base_t {

public:
  ov_client_digitalstage_t(ov_render_base_t& backend, std::string api_url);

  ~ov_client_digitalstage_t();

  void set_token(const std::string& token);

  const std::string get_token();

  void start_service();

  void stop_service();

  bool is_going_to_stop() const { return quitrequest_; };

protected:
  ov_render_base_t& backend_;

private:
  void service();

  void on_sound_devices_change();

  void send(const std::string& event, const std::string& message);

  void sendAsync(const std::string& event, const std::string& message);

  void configureConnection(const ds::stage_ov_server_t server);

  void configureAudio(const ds::soundcard_t soundcard);

  void startOv();

  void stopOv();

  void createTrack(const std::string& soundCardId, unsigned int channel);

  // TODO: Replace this later with a more detailes TASCAR control
  void syncLocalStageMember();
  void syncRemoteStageMembers();
  void syncGroupVolume(const std::string& groupId);
  void syncGroupPosition(const std::string& groupId);
  void syncStageMemberVolume(const std::string& stageMemberId);
  void syncStageMemberPosition(const std::string& stageMemberId);
  void syncRemoteOvTrackVolume(const std::string& remoteOvTrackId);
  void syncRemoteOvTrackPosition(const std::string& remoteOvTrackId);

  bool isInsideStage();

  // Threading
  std::thread servicethread_;
  std::recursive_mutex backend_mutex_;

  // Connection related
  websocket_callback_client wsclient_;
  std::string api_url_;
  std::string token_;

#ifndef LINUX
  sound_card_tools_t* sound_card_tools_;
#endif
  ds::ds_store_t* store_;

  std::atomic<bool> ready_;
  std::atomic<bool> quitrequest_;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
