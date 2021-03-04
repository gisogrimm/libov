#include "ds_service.h"
#include <pplx/pplxtasks.h>
#include "udpsocket.h"
#include <nlohmann/json.hpp>
#include <utility>

using namespace utility;
using namespace web;
using namespace web::http;
using namespace utility::conversions;
using namespace pplx;
using namespace concurrency::streams;

task_completion_event<void> tce; // used to terminate async PPLX listening task

ds::ds_service_t::ds_service_t(ov_render_base_t &backend, std::string api_url) : backend_(backend),
                                                                                 api_url_(std::move(api_url)) {
  this->sound_card_tools_ = new sound_card_tools_t();
  std::cout << this->backend_.get_deviceid() << std::endl;
  this->store_ = new ds_store_t();
}

ds::ds_service_t::~ds_service_t() {
  delete this->sound_card_tools_;
  delete this->store_;
}

void ds::ds_service_t::start(const std::string &token) {
  this->token_ = token;
  this->servicethread_ = std::thread(&ds::ds_service_t::service, this);
}

void ds::ds_service_t::stop() {
  tce.set();        // task completion event is set closing wss listening task
  this->wsclient_.close(); // wss client is closed
  this->servicethread_.join(); // thread is joined
}

void ds::ds_service_t::service() {
  wsclient_.connect(U(this->api_url_)).wait();

  auto receive_task = create_task(tce);

  // handler for incoming d-s messages
  wsclient_.set_message_handler([&](const websocket_incoming_message &ret_msg) {
    // -----------------------------------------------
    // -----    parse incoming message events    -----
    // -----------------------------------------------

    auto ret_str = ret_msg.extract_string().get();

    // hey is our ping, so exlude it
    if (ret_str != "hey") {
      try {
        nlohmann::json j = nlohmann::json::parse(ret_str);

        const std::string &event = j["data"][0];
        const nlohmann::json payload = j["data"][1];

        ucout << "[" << event << "] " << payload.dump() << std::endl;

        if (event == "local-device-ready") {
          this->store_->setLocalDevice(payload);
          // UPDATE SOUND CARDS
          const std::vector<sound_card_t> sound_devices = this->sound_card_tools_->get_sound_devices();
          if (!sound_devices.empty()) {
            nlohmann::json deviceUpdate;
            deviceUpdate["_id"] = payload["_id"];
            for (const auto &sound_device : sound_devices) {
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
                  {"isDefault", sound_device.is_default}
              };
              this->sendAsync("set-sound-card", soundcard.dump());
            }
            this->sendAsync("update-device", deviceUpdate.dump());
          } else {
            std::cerr << "WARNING: No soundcards available!" << std::endl;
          }
        } else if (event == "device-changed") {
          boost::optional<ds::json::device_t> localDevice = this->store_->getLocalDevice();
          if (localDevice && localDevice->_id == payload["_id"]) {
            // The events refers this device
            std::cout << "DEVICE CHANGED" << std::endl;
            if (payload.contains("soundCardId") &&
                payload["soundCardId"] != localDevice->soundCardId) {
              // Soundcard changes
              boost::optional<ds::json::soundcard_t> soundCard = this->store_->readSoundCard(payload["soundCardId"]);
              if (soundCard) {
                // Use sound card
                this->backend_.configure_audio_backend({
                                                           soundCard->driver,
                                                           soundCard->deviceId,
                                                           soundCard->sampleRate,
                                                           soundCard->periodSize,
                                                           soundCard->numPeriods
                                                       });
                // We have to remove all existing tracks and propagate new tracks
                std::vector<ds::json::ov_track_t> existingTracks = this->store_->readOvTracks();
                for (const auto &track : existingTracks) {
                  this->sendAsync("remove-track", track._id);
                }
                for(const auto &channel : soundCard->inputChannels) {
                  this->createTrack(soundCard->_id, channel);
                }
              } else {
                std::cerr << "Race condition or logical error: could not find sound-card propagated by device update"
                          << std::endl;
              }
            }

            if (payload.contains("sendAudio") &&
                payload["sendAudio"] != localDevice->sendAudio) {
              if (payload["sendAudio"]) {
                //TODO: Connect ov
                std::cout << "[TODO] Start sending and receiving" << std::endl;
              } else {
                //TODO: Disconnect ov
                std::cout << "[TODO] Stop sending and receiving" << std::endl;
              }
            }
            this->store_->updateLocalDevice(payload);
          }
        } else if (event == "stage-joined") {
          this->store_->setCurrentStageId(payload["stageId"]);
          if (payload.contains("stages") && payload["stages"].is_array()) {
            for (const nlohmann::json &i : payload["stages"]) {
              this->store_->createStage(i);
            }
          }
          if (payload.contains("groups") && payload["groups"].is_array()) {
            for (const nlohmann::json &i : payload["groups"]) {
              this->store_->createGroup(i);
            }
          }
          if (payload.contains("customGroups") && payload["customGroups"].is_array()) {
            for (const nlohmann::json &i : payload["customGroups"]) {
              this->store_->createCustomGroup(i);
            }
          }
          if (payload.contains("stageMembers") && payload["stageMembers"].is_array()) {
            for (const nlohmann::json &i : payload["stageMembers"]) {
              this->store_->createStageMember(i);
            }
          }
          if (payload.contains("customStageMembers") && payload["customStageMembers"].is_array()) {
            for (const nlohmann::json &i : payload["customStageMembers"]) {
              this->store_->createCustomStageMember(i);
            }
          }
          if (payload.contains("ovTracks") && payload["ovTracks"].is_array()) {
            for (const nlohmann::json &i : payload["ovTracks"]) {
              this->store_->createOvTrack(i);
              // Also consume track
            }
          }
          if (payload.contains("remoteOvTracks") && payload["remoteOvTracks"].is_array()) {
            for (const nlohmann::json &i : payload["remoteOvTracks"]) {
              this->store_->createRemoteOvTrack(i);
            }
          }
          if (payload.contains("customRemoteOvTracks") && payload["customRemoteOvTracks"].is_array()) {
            for (const nlohmann::json &i : payload["customRemoteOvTracks"]) {
              this->store_->createCustomRemoteOvTrack(i);
            }
          }
          /*
          if (this->isSendingAudio()) {
              if (this->store.stages.count(this->store.currentStageId) > 0) {
                  nlohmann::json stage = this->store.stages[this->store.currentStageId];
                  std::cout << stage.dump() << std::endl;
                  //TODO: ovserver can be null...
                  //this->backend_.set_relay_server(stage.ovServer.ipv4, stage.ovServer.port, stage.ovServer.pin);
                  // TODO: START SENDING
                  std::cout << "[TODO] Start sending and receiving" << std::endl;
              } else {
                  std::cerr << "Could not find current stage" << std::endl;
              }
          }*/
        } else if (event == "stage-left") {
          std::cout << "[TODO] Stop sending and receiving" << std::endl;
          // TODO: check: STOP SENDING
          this->backend_.clear_stage();
          this->backend_.stop_audiobackend();

          // Remove active stage related data
          this->store_->removeCustomGroups();
          this->store_->removeStageMembers();
          this->store_->removeCustomStageMembers();
          this->store_->removeRemoteOvTracks();
          this->store_->removeCustomRemoteOvTracks();
        } else if (event == "user-ready") {

        } else if (event == "stage-added") {
          this->store_->createStage(payload);
        } else if (event == "stage-changed") {
          const std::string stageId = payload["_id"].get<std::string>();
          const std::string currentStageId = this->store_->getCurrentStageId();
          this->store_->updateStage(stageId, payload);
          if (currentStageId == stageId) {
            //TODO: Verify, if just setting again works
            boost::optional<ds::json::stage_t> stage = this->store_->readStage(currentStageId);
            if (stage) {
              this->backend_.set_relay_server(stage->ovServer.ipv4, stage->ovServer.port,
                                              stage->ovServer.pin);
              //TODO: Update stage render settings
              //this->backend_.set_render_settings()
            }
          }
        } else if (event == "stage-removed") {
          this->store_->removeStage(payload.get<std::string>());
        } else if (event == "group-added") {
          this->store_->createGroup(payload);
          if (this->backend_.is_session_active()) {
            //TODO: Change volume and position of all tracks of the related stage member
          }
        } else if (event == "group-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store_->updateGroup(_id, payload);
          if (this->backend_.is_session_active()) {
            //TODO: Change volume and position of all tracks of the related stage member
          }
        } else if (event == "group-removed") {
          const std::string _id = payload.get<std::string>();
          this->store_->removeGroup(_id);
          if (this->backend_.is_session_active()) {
            //TODO: Change volume and position of all tracks of the related stage member
          }
        } else if (event == "stage-member-added") {
          this->store_->createStageMember(payload);
          if (this->backend_.is_session_active()) {
            //TODO: Change volume and position of all tracks of the related stage member
          }
        } else if (event == "stage-member-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store_->updateStageMember(_id, payload);
          if (this->backend_.is_session_active()) {
            //TODO: Change volume and position of all tracks of the related stage member
          }
        } else if (event == "stage-member-removed") {
          const std::string _id = payload.get<std::string>();
          this->store_->removeStageMember(_id);
          if (this->backend_.is_session_active()) {
            //TODO: Change volume and position of all tracks of the related stage member
          }
        } else if (event == "custom-stage-member-added") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store_->createCustomStageMember(payload);
          if (this->isSendingAudio()) {
            //TODO: Change volume and position of all tracks of the related stage member
          }
        } else if (event == "custom-stage-member-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store_->updateCustomStageMember(_id, payload);
          if (this->isSendingAudio()) {
            //TODO: Change volume and position of all tracks of the related stage member
            std::cout << "TODO: Change volume and position of all tracks of the related stage member"
                      << std::endl;
          }
        } else if (event == "custom-stage-member-removed") {
          const std::string _id = payload.get<std::string>();
          this->store_->removeCustomStageMember(_id);
          if (this->isSendingAudio()) {
            //TODO: Change volume and position of all tracks of the related stage member
            std::cout << "TODO: Change volume and position of all tracks of the related stage member"
                      << std::endl;
          }
        } else if (event == "stage-member-ov-added") {
          this->store_->createRemoteOvTrack(payload);
          if (this->isSendingAudio()) {
            //TODO: Add track as stage device to backend
            std::cout << "TODO: Add track " << payload["_id"] << " as stage device " << payload["ovId"]
                      << " to backend" << std::endl;
          }
        } else if (event == "stage-member-ov-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store_->updateRemoteOvTrack(_id, payload);
          auto ovTrack = this->store_->readRemoteOvTrack(_id);
          if (this->isSendingAudio()) {
            //TODO: Change stage device of backend
            //TODO: This means removal and addition
            std::cout << "TODO: Remove and add stage device " << ovTrack->ovTrackId << std::endl;
          }
        } else if (event == "stage-member-ov-removed") {
          const std::string _id = payload.get<std::string>();
          auto ovTrack = this->store_->readRemoteOvTrack(_id);
          this->store_->removeRemoteOvTrack(_id);
          if (this->isSendingAudio()) {
            //TODO: Remove stage device of backend
            std::cout << "TODO: Remove stage device " << ovTrack->ovTrackId << std::endl;
          }
        } else if (event == "custom-stage-member-ov-added") {
          this->store_->createCustomRemoteOvTrack(payload);
          if (this->isSendingAudio()) {
            //TODO: Change volume and position of the assigned track (and its stage device)
            std::cout << "TODO: Change volume and position of the assigned track (and its stage device)"
                      << std::endl;
          }
        } else if (event == "custom-stage-member-ov-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store_->updateCustomRemoteOvTrack(_id, payload);
          if (this->isSendingAudio()) {
            //TODO: Change volume and position of the assigned track (and its stage device)
            std::cout << "TODO: Change volume and position of the assigned track (and its stage device)"
                      << std::endl;
          }
        } else if (event == "custom-stage-member-ov-removed") {
          const std::string _id = payload.get<std::string>();
          this->store_->removeCustomRemoteOvTrack(_id);
          if (this->isSendingAudio()) {
            //TODO: Change volume and position of the assigned track (and its stage device)
            std::cout << "TODO: Change volume and position of the assigned track (and its stage device)"
                      << std::endl;
          }
        } else if (event == "sound-card-added") {
          this->store_->createSoundCard(payload);
        } else if (event == "sound-card-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store_->updateSoundCard(_id, payload);
          auto localDevice = this->store_->getLocalDevice();
          if (localDevice
              && localDevice->sendAudio == true
              && localDevice->soundCardId == _id) {
            //TODO: Change sound card settings, presets etc.
          }
        } else if (event == "sound-card-removed") {
          const std::string _id = payload.get<std::string>();
          this->store_->removeSoundCard(_id);
        } else if (event == "track-added") {
          this->store_->createOvTrack(payload);
        } else if (event == "track-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store_->updateOvTrack(_id, payload);
        } else if (event == "track-removed") {
          const std::string _id = payload.get<std::string>();
          this->store_->removeOvTrack(_id);
        } else {
          ucout << "Not supported: [" << event << "] " << payload.dump() << std::endl;
        }
      } catch (const std::exception &e) {
        std::cerr << "std::exception: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "error parsing" << std::endl;
      }
    }
  });

  //utility::string_t close_reason;
  wsclient_.set_close_handler([](websocket_close_status status,
                                 const utility::string_t &reason,
                                 const std::error_code &code) {
    ucout << " closing reason..." << reason << "\n";
    ucout << "connection closed, reason: " << reason
          << " close status: " << int(status) << " error code " << code
          << std::endl;
  });

  // Register sound card handler
  //this->soundio->on_devices_change = this->on_sound_devices_change;

  // Get mac address and local ip
  std::string localIpAddress(ep2ipstr(getipaddr()));
  std::string mac(backend_.get_deviceid());
  std::cout << "MAC Address: " << mac << " IP: " << localIpAddress << '\n';

  // Initial call with device
  nlohmann::json deviceJson;
  deviceJson["mac"] = mac;    // MAC = device id (!)
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

void ds::ds_service_t::on_sound_devices_change() {
  ucout << "SOUNDCARD CHANGED" << std::endl;
  ucout << "Have now " << this->sound_card_tools_->get_input_sound_devices().size() << " input soundcards"
        << std::endl;
}

void ds::ds_service_t::send(const std::string &event, const std::string &message) {
  websocket_outgoing_message msg;
  std::string body_str(R"({"type":0,"data":[")" + event + "\"," + message + "]}");
  msg.set_utf8_message(body_str);
  ucout << std::endl << "[SENDING] " << body_str << std::endl << std::endl;
  wsclient_.send(msg).wait();
}

void ds::ds_service_t::sendAsync(const std::string &event, const std::string &message) {
  websocket_outgoing_message msg;
  std::string body_str(R"({"type":0,"data":[")" + event + "\"," + message + "]}");
  msg.set_utf8_message(body_str);
  ucout << std::endl << "[SENDING] " << body_str << std::endl << std::endl;
  wsclient_.send(msg);
}

bool ds::ds_service_t::isSendingAudio() {
  return this->backend_.is_session_active();
}

void ds::ds_service_t::createTrack(const std::string &soundCardId, int channel) {
  nlohmann::json trackJson = {
      {"soundCardId", soundCardId},
      {"channel", channel}
  };
  this->sendAsync("add-track", trackJson);
}

void ds::ds_service_t::removeTrack(const std::string &id) {
  this->sendAsync("remove-track", id);
}
