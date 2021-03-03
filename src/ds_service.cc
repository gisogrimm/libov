#include "ds_service.h"
#include "udpsocket.h"
#include <nlohmann/json.hpp>
#include <pplx/pplxtasks.h>
#include <utility>

using namespace utility;
using namespace web;
using namespace web::http;
using namespace utility::conversions;
using namespace pplx;
using namespace concurrency::streams;

task_completion_event<void> tce; // used to terminate async PPLX listening task

ds::ds_service_t::ds_service_t(ov_render_base_t& backend, std::string api_url)
    : backend_(backend), api_url_(std::move(api_url)), running_(false),
      quitrequest_(false)
{
  this->sound_card_tools = new sound_card_tools_t();
  this->test();
}

ds::ds_service_t::~ds_service_t()
{
  delete this->sound_card_tools;
}

void ds::ds_service_t::start(const std::string& token)
{
  this->test();
  this->token_ = token;
  this->running_ = true;
  this->servicethread_ = std::thread(&ds::ds_service_t::service, this);
}

void ds::ds_service_t::stop()
{
  this->test();
  if(this->backend_.is_session_active()) {
    this->backend_.end_session();
  }
  if(this->backend_.is_audio_active()) {
    this->backend_.stop_audiobackend();
  }
  this->backend_.clear_stage();
  this->running_ = false;
  tce.set(); // task completion event is set closing wss listening task
  this->wsclient.close();      // wss client is closed
  this->servicethread_.join(); // thread is joined
}

void ds::ds_service_t::test() {
  const std::lock_guard<std::mutex> lock(this->store_mtx_);
  std::cout << "THREADTEST " << this->backend_.get_deviceid() << std::endl;
}

void ds::ds_service_t::service()
{
  wsclient.connect(U(this->api_url_)).wait();

  this->test();

  auto receive_task = create_task(tce);

  // handler for incoming d-s messages
  wsclient.set_message_handler([&](const websocket_incoming_message& ret_msg) {
    auto ret_str = ret_msg.extract_string().get();

    // "hey" is our ping, so exclude it
    if(ret_str != "hey") {
      try {
        nlohmann::json j = nlohmann::json::parse(ret_str);

        const std::string& event = j["data"][0];
        const nlohmann::json payload = j["data"][1];

        ucout << std::endl << "[" << event << "] " << payload.dump() << std::endl;

        if(event == "local-device-ready") {
          this->store.localDevice = payload;
          // UPDATE SOUND CARDS
          const std::vector<sound_card_t> sound_devices =
              this->sound_card_tools->get_sound_devices();
          if(!sound_devices.empty()) {
            nlohmann::json deviceUpdate;
            deviceUpdate["_id"] = this->store.localDevice["_id"];
            for(const auto& sound_device : sound_devices) {
              ucout << "[AUDIODEVICES] Have " << sound_device.id << std::endl;
              deviceUpdate["soundCardNames"].push_back(sound_device.id);
              nlohmann::json soundcard;
              soundcard["name"] = sound_device.id;
              soundcard["initial"] = {
                  {"label", sound_device.name},
                  {"numInputChannels", sound_device.num_input_channels},
                  {"numOutputChannels", sound_device.num_output_channels},
                  {"sampleRate", sound_device.sample_rate},
                  {"sampleRates", sound_device.sample_rates},
                  {"isDefault", sound_device.is_default}};
              this->sendAsync("set-sound-card", soundcard.dump());
            }
            this->sendAsync("update-device", deviceUpdate.dump());
          } else {
            std::cerr << "WARNING: No soundcards available!" << std::endl;
          }
        } else if(event == "device-changed") {
          if(!this->store.localDevice.is_null() &&
             this->store.localDevice["_id"] == payload["_id"]) {
            // The events refers this device
            std::cout << "DEVICE CHANGED" << std::endl;
            if(payload.contains("soundCardName") &&
               payload["soundCardName"] !=
                   this->store.localDevice["soundCardName"]) {
              // Soundcard changes
              // TODO: Restart JACK with soundCardName as identifier
            }

            if(payload.contains("sendAudio") &&
               payload["sendAudio"] != this->store.localDevice["sendAudio"]) {
              if(payload["sendAudio"]) {
                // TODO: Connect ov
                std::cout << "[TODO] Start sending and receiving" << std::endl;
              } else {
                // TODO: Disconnect ov
                std::cout << "[TODO] Stop sending and receiving" << std::endl;
              }
            }
            this->store.localDevice.merge_patch(payload);
          }
        } else if(event == "stage-joined") {
          this->store.currentStageId = payload["stageId"];
          if(payload.contains("stages") && payload["stages"].is_array()) {
            for(const nlohmann::json& i : payload["stages"]) {
              const std::string _id = i["_id"].get<std::string>();
              this->store.stages[_id] = i;
            }
          }
          if(payload.contains("groups") && payload["groups"].is_array()) {
            for(const nlohmann::json& i : payload["groups"]) {
              const std::string _id = i["_id"].get<std::string>();
              this->store.groups[_id] = i;
            }
          }
          if(payload.contains("customGroups") &&
             payload["customGroups"].is_array()) {
            for(const nlohmann::json& i : payload["customGroups"]) {
              const std::string _id = i["_id"].get<std::string>();
              this->store.customGroups[_id] = i;
            }
          }
          if(payload.contains("stageMembers") &&
             payload["stageMembers"].is_array()) {
            for(const nlohmann::json& i : payload["stageMembers"]) {
              const std::string _id = i["_id"].get<std::string>();
              this->store.stageMembers[_id] = i;
            }
          }
          if(payload.contains("customStageMembers") &&
             payload["customStageMembers"].is_array()) {
            for(const nlohmann::json& i : payload["customStageMembers"]) {
              const std::string _id = i["_id"].get<std::string>();
              this->store.customStageMembers[_id] = i;
            }
          }
          if(payload.contains("remoteOvTracks") && payload["remoteOvTracks"].is_array()) {
            for(const nlohmann::json& i : payload["remoteOvTracks"]) {
              const std::string _id = i["_id"].get<std::string>();
              this->store.stageMemberTracks[_id] = i;
            }
          }
          if(payload.contains("customRemoteOvTracks") &&
             payload["customRemoteOvTracks"].is_array()) {
            for(const nlohmann::json& i : payload["customRemoteOvTracks"]) {
              const std::string _id = i["_id"].get<std::string>();
              this->store.customStageMemberTracks[_id] = i;
            }
          }
          ucout << "PART FINISHED" << std::endl;
          if(this->is_sending_audio()) {
            if(this->store.stages.count(this->store.currentStageId) > 0) {
              nlohmann::json stage =
                  this->store.stages[this->store.currentStageId];
              std::cout << stage.dump() << std::endl;
              if(stage.contains("ovServer")) {
                ucout << "Stage supports ov" << std::endl;
                std::cout << this->backend_.get_deviceid() << std::endl;
                ucout << "Set relay server" << std::endl;
                this->backend_.set_relay_server(stage["ovServer"]["ipv4"],
                                                stage["ovServer"]["port"],
                                                stage["ovServer"]["pin"]);
                ucout << "Starting audio backend" << std::endl;
                this->backend_.start_audiobackend();
                ucout << "Starting session" << std::endl;
                this->backend_.start_session();
                std::cout << "[TODO] Start sending and receiving" << std::endl;
              }
            } else {
              std::cerr << "Could not find current stage" << std::endl;
            }
          }
          ucout << "FINISHED" << std::endl;
        } else if(event == "stage-left") {
          std::cout << "[TODO] Stop sending and receiving" << std::endl;
          // TODO: check: STOP SENDING
          this->backend_.clear_stage();
          this->backend_.stop_audiobackend();

          // Remove active stage related data
          this->store.customGroups.clear();
          this->store.stageMembers.clear();
          this->store.customStageMembers.clear();
          this->store.stageMemberTracks.clear();
          this->store.customStageMemberTracks.clear();
        } else if(event == "user-ready") {

        } else if(event == "stage-added") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store.stages[_id] = payload;
        } else if(event == "stage-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store.stages[_id].merge_patch(payload);
          if(this->store.currentStageId == _id) {
            // TODO: Verify, if just setting again works
            ds::json::stage_t stage =
                this->store.stages[this->store.currentStageId]
                    .get<ds::json::stage_t>();
            this->backend_.set_relay_server(
                stage.ovServer.ipv4, stage.ovServer.port, stage.ovServer.pin);
            // TODO: Update stage render settings
            // this->backend_.set_render_settings()
          }
        } else if(event == "stage-removed") {
          const std::string _id = payload.get<std::string>();
          this->store.stages.erase(_id);
        } else if(event == "group-added") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store.groups[_id] = payload;
          if(this->is_sending_audio()) {
            // TODO: Change volume and position of all tracks of the related
            // stage member
          }
        } else if(event == "group-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store.groups[_id].merge_patch(payload);
          std::vector<std::string> stageMemberIds =
              this->getStageMemberIdsByGroupId(_id);
          for(const auto& stageMemberId : stageMemberIds) {
            this->update_stage_member(stageMemberId);
          }
        } else if(event == "group-removed") {
          const std::string _id = payload.get<std::string>();
          this->store.groups.erase(_id);
        } else if(event == "stage-member-added") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store.stageMembers[_id] = payload;
          // TODO: CREATE STAGE DEVICE
        } else if(event == "stage-member-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store.stageMembers[_id].merge_patch(payload);
          this->update_stage_member(_id);
        } else if(event == "stage-member-removed") {
          const std::string _id = payload.get<std::string>();
          const stage_device_id_t stageDeviceId =
              this->store.stageMembers[_id]["ovStageDeviceId"];
          this->store.stageMembers.erase(_id);
          this->backend_.rm_stage_device(stageDeviceId);
        } else if(event == "custom-stage-member-added") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store.customStageMembers[_id] = payload;
          const std::string stageMemberId =
              this->store.customStageMembers[_id]["stageMemberId"];
          this->store.customStageMemberIdByStageMember[stageMemberId] = _id;
          this->update_stage_member(stageMemberId);
        } else if(event == "custom-stage-member-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store.customStageMembers[_id].merge_patch(payload);
          const std::string stageMemberId =
              this->store.customStageMembers[_id]["stageMemberId"];
        } else if(event == "custom-stage-member-removed") {
          const std::string _id = payload.get<std::string>();
          const std::string stageMemberId =
              this->store.customStageMembers[_id]["stageMemberId"];
          this->store.customStageMemberIdByStageMember.erase(stageMemberId);
          this->store.customStageMembers.erase(_id);
          this->update_stage_member(stageMemberId);
        } else if(event == "stage-member-ov-added") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store.stageMemberTracks[_id] = payload;
          if(this->is_sending_audio()) {
            // TODO: Add track as stage device to backend
            std::cout << "TODO: Add track " << _id << " as stage device "
                      << payload["ovId"] << " to backend" << std::endl;
          }
        } else if(event == "stage-member-ov-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store.stageMemberTracks[_id].merge_patch(payload);
          this->update_stage_member_track(_id);
        } else if(event == "stage-member-ov-removed") {
          const std::string _id = payload.get<std::string>();
          this->store.stageMemberTracks.erase(_id);
          if(this->is_sending_audio()) {
            // TODO: Remove stage device of backend
            ds::json::stage_member_ov_t ovTrack =
                this->store.stageMemberTracks[_id]
                    .get<ds::json::stage_member_ov_t>();
            std::cout << "TODO: Remove stage device " << ovTrack.ovId
                      << std::endl;
          }
        } else if(event == "custom-stage-member-ov-added") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store.customStageMemberTracks[_id] = payload;
          const std::string stageMemberId = payload["stageMemberId"];
          this->update_stage_member_track(stageMemberId);
        } else if(event == "custom-stage-member-ov-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store.customStageMemberTracks[_id].merge_patch(payload);
          const std::string stageMemberId =
              this->store.customStageMemberTracks[_id]["stageMemberId"];
          this->update_stage_member_track(stageMemberId);
        } else if(event == "custom-stage-member-ov-removed") {
          const std::string _id = payload.get<std::string>();
          const std::string stageMemberId =
              this->store.customStageMemberTracks[_id]["stageMemberId"];
          this->store.customStageMemberTracks.erase(_id);
          this->update_stage_member_track(stageMemberId);
        } else if(event == "sound-card-added") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store.soundCards[_id] = payload;
        } else if(event == "sound-card-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store.soundCards[_id].merge_patch(payload);
          if(this->store.localDevice &&
             this->store.localDevice.contains("soundCardId") &&
             this->store.localDevice["soundCardId"] == _id) {
            const nlohmann::json soundCard = this->store.soundCards[_id];
            audio_device_t audioDevice;
            audioDevice.drivername = soundCard["driver"];
            audioDevice.devicename = soundCard["name"];
            audioDevice.srate = soundCard["sampleRate"];
            audioDevice.periodsize = soundCard["periodSize"];
            audioDevice.numperiods = soundCard["numPeriods"];
            this->backend_.configure_audio_backend(audioDevice);
          }
        } else if(event == "sound-card-removed") {
          const std::string _id = payload.get<std::string>();
          this->store.soundCards.erase(_id);
        } else if(event == "track-preset-added") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store.trackPresets[_id] = payload;
        } else if(event == "track-preset-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store.trackPresets[_id].merge_patch(payload);
        } else if(event == "track-preset-removed") {
          const std::string _id = payload.get<std::string>();
          this->store.trackPresets.erase(_id);
          // TODO: DISCUESS IF WE NEED THE FOLLOWING TRACK-HANDLING
        } else if(event == "track-added") {
          const std::string _id = payload["_id"].get<std::string>();
          // this->store.stageMemberTracks[_id] = payload;
        } else if(event == "track-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          // this->store.tracks[_id].merge_patch(payload);
        } else if(event == "track-removed") {
          const std::string _id = payload.get<std::string>();
          // this->store.tracks.erase(_id);
        } else {
          ucout << "Not supported: [" << event << "] " << payload.dump()
                << std::endl;
        }
      }
      catch(const std::exception& e) {
        std::cerr << "std::exception: " << e.what() << std::endl;
      }
      catch(...) {
        std::cerr << "error parsing" << std::endl;
      }
    }
  });

  // utility::string_t close_reason;
  wsclient.set_close_handler([](websocket_close_status status,
                                const utility::string_t& reason,
                                const std::error_code& code) {
    ucout << " closing reason..." << reason << "\n";
    ucout << "connection closed, reason: " << reason
          << " close status: " << int(status) << " error code " << code
          << std::endl;
  });

  // Register sound card handler
  // this->soundio->on_devices_change = this->on_sound_devices_change;

  // Get mac address and local ip
  std::string localIpAddress(ep2ipstr(getipaddr()));
  std::string mac(backend_.get_deviceid());
  std::cout << "MAC Address: " << mac << " IP: " << localIpAddress << '\n';

  // Initial call with device
  nlohmann::json deviceJson;
  deviceJson["mac"] = mac; // MAC = device id (!)
  deviceJson["canVideo"] = false;
  deviceJson["canAudio"] = true;
  deviceJson["canOv"] = "true";
  deviceJson["sendAudio"] = "true";
  deviceJson["sendVideo"] = "false";
  deviceJson["receiveAudio"] = "true";
  deviceJson["receiveVideo"] = "false";

  nlohmann::json identificationJson;
  identificationJson["token"] = this->token_;
  identificationJson["device"] = deviceJson;
  this->sendAsync("token", identificationJson.dump());

  // RECEIVE TILL END
  receive_task.wait();
}

void ds::ds_service_t::on_sound_devices_change()
{
  ucout << "SOUNDCARD CHANGED" << std::endl;
  ucout << "Have now "
        << this->sound_card_tools->get_input_sound_devices().size()
        << " input soundcards" << std::endl;
}

void ds::ds_service_t::send(const std::string& event,
                            const std::string& message)
{
  websocket_outgoing_message msg;
  std::string body_str(R"({"type":0,"data":[")" + event + "\"," + message +
                       "]}");
  msg.set_utf8_message(body_str);
  ucout << std::endl << "[SENDING] " << body_str << std::endl << std::endl;
  wsclient.send(msg).wait();
}

void ds::ds_service_t::sendAsync(const std::string& event,
                                 const std::string& message)
{
  websocket_outgoing_message msg;
  std::string body_str(R"({"type":0,"data":[")" + event + "\"," + message +
                       "]}");
  msg.set_utf8_message(body_str);
  ucout << std::endl << "[SENDING] " << body_str << std::endl << std::endl;
  wsclient.send(msg);
}

bool ds::ds_service_t::is_sending_audio()
{
  return this->store.localDevice.is_object() &&
         this->store.localDevice.contains("sendAudio") &&
         this->store.localDevice["sendAudio"] == "true";
}

void ds::ds_service_t::update_stage_member(const std::string& stageMemberId)
{
  // Fetch all related entities
  const nlohmann::json stageMember = this->store.stageMembers[stageMemberId];
  const stage_device_id_t stageDeviceId = stageMember["ovStageDeviceId"];
  nlohmann::json customStageMember;
  if(this->store.customStageMemberIdByStageMember.count(stageMemberId) > 0) {
    customStageMember =
        this->store.customStageMembers
            [this->store.customStageMemberIdByStageMember[stageMemberId]];
  }
  const nlohmann::json group = this->store.groups[stageMember["groupId"]];
  // Calculate gain
  auto gain = stageMember["volume"].get<double>();
  if(customStageMember.is_object()) {
    gain = gain * customStageMember["volume"].get<double>();
  }
  gain = gain * group["volume"].get<double>();
  this->backend_.set_stage_device_gain(stageDeviceId, gain);
  // TODO: Calculate position, now just using the stage member position
  this->backend_.set_stage_device_position(
      stageDeviceId,
      {stageMember["x"].get<double>(), stageMember["y"].get<double>(),
       stageMember["z"].get<double>()},
      {stageMember["rZ"].get<double>(), stageMember["rY"].get<double>(),
       stageMember["rX"].get<double>()});
}

void ds::ds_service_t::update_stage_member_track(const std::string& trackId)
{
  const nlohmann::json track = this->store.stageMemberTracks[trackId];
  const nlohmann::json stageMember =
      this->store.stageMembers[track["stageMemberId"]];
  const stage_device_id_t stageDeviceId = stageMember["ovStageDeviceId"];
  this->backend_.set_stage_device_channel_gain(stageDeviceId, trackId,
                                               track["gain"]);
  this->backend_.set_stage_device_channel_position(
      stageDeviceId, trackId, {track["x"].get<double>(), track["y"].get<double>(), track["z"].get<double>()}, {track["rZ"].get<double>(), track["rY"].get<double>(), track["rX"].get<double>()});
}
/*
void ds::ds_service_t::add_stage_member(const std::string &stageMemberId) {
  if (this->store.trackIdsByStageMember.count(stageMemberId) > 0) {
    // Fetch all related entities
    const nlohmann::json stageMember = this->store.stageMembers[stageMemberId];
    nlohmann::json customStageMember;
    if (this->store.customStageMemberIdByStageMember.count(stageMemberId) > 0) {
      customStageMember =
this->store.customStageMembers[this->store.customStageMemberIdByStageMember[stageMemberId]];
    }
    const nlohmann::json group = this->store.groups[stageMember["groupId"]];
    const std::vector<std::string> trackIds =
this->store.trackIdsByStageMember[stageMemberId]; std::vector<nlohmann::json>
tracks; for (const auto &trackId : trackIds) {
      tracks.push_back(this->store.stageMemberTracks[trackId]);
    }
    stage_device_t stageDevice; // 1 Stage member = 1 stage device
    stage_device_id_t callerId = stageMember["ovStageDeviceId"];
    stageDevice.id = stageMember[callerId];
    for (const auto &track : tracks) {
      device_channel_t deviceChannel; // 3 Tracks von 1 StageMember = 3 device
channels deviceChannel.gain = 1.0; //TODO: Replace with direct manipulation (=
track["gain"])
      //deviceChannel.sourceport = track["sourceport"];
      deviceChannel.directivity = track["directivity"];
      deviceChannel.position = {track["x"], track["y"], track["z"]};
      stageDevice.channels.push_back(deviceChannel);
    }
    // I know what number
    if (stage_device_exists_already)
      //this->backend_.add_stage_device(callerId);


      // track gain = group gain * stage member gain * (custom stage member
gain) * track gain * (custom track gain)
      // DYNAMIC:
      this->backend_.set_stage_device_gain();

    // stage member position

    // stage_t (= Stage, Device)
  } else {
    // Member has no tracks
    std::cerr << "Member has no tracks" << std::endl;
  }
}*/

std::vector<std::string>
ds::ds_service_t::getStageMemberIdsByGroupId(const std::string& groupId)
{
  std::vector<std::string> stageMemberIds;
  for(auto const& x : this->store.stageMembers) {
    if(x.second["groupId"].get<std::string>() == groupId) {
      stageMemberIds.push_back(x.first);
    }
  }
  return stageMemberIds;
}