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

ds::ds_service_t::ds_service_t(ov_render_base_t &backend, std::string api_url)
    : backend_(backend), api_url_(std::move(api_url)) {
  this->sound_card_tools_ = new sound_card_tools_t();
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
  tce.set(); // task completion event is set closing wss listening task
  this->wsclient_.close();     // wss client is closed
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
          const auto localDevice = this->store_->getLocalDevice();
          // UPDATE SOUND CARDS
          const std::vector<sound_card_t> sound_devices =
              this->sound_card_tools_->get_sound_devices();
          if (!sound_devices.empty()) {
            nlohmann::json deviceUpdate;
            deviceUpdate["_id"] = payload["_id"];
            std::string defaultSoundCardName;
            for (const auto &sound_device : sound_devices) {
              deviceUpdate["soundCardNames"].push_back(sound_device.id);
              if (sound_device.is_default) {
                defaultSoundCardName = sound_device.id;
              }
              nlohmann::json soundCard;
              soundCard["name"] = sound_device.id;
              soundCard["initial"] = {
                  {"label", sound_device.name},
                  {"numInputChannels", sound_device.num_input_channels},
                  {"numOutputChannels", sound_device.num_output_channels},
                  {"sampleRate", sound_device.sample_rate},
                  {"sampleRates", sound_device.sample_rates},
                  {"isDefault", sound_device.is_default}};
              this->sendAsync("set-sound-card", soundCard.dump());
            }
            if (localDevice->soundCardName.empty() &&
                !defaultSoundCardName.empty()) {
              deviceUpdate["soundCardName"] = defaultSoundCardName;
            }
            this->sendAsync("update-device", deviceUpdate.dump());
          } else {
            std::cerr << "WARNING: No soundcards available!" << std::endl;
          }
        } else if (event == "device-changed") {
          boost::optional<const ds::json::device_t> localDevice =
              this->store_->getLocalDevice();
          if (localDevice && localDevice->_id == payload["_id"]) {
            // The events refers this device
            if (payload.contains("soundCardName") &&
                payload["soundCardName"] != localDevice->soundCardName) {
              // Soundcard changes
              boost::optional<const ds::json::soundcard_t> soundCard =
                  this->store_->readSoundCardByName(payload["soundCardName"]);
              if (soundCard) {
                // Use sound card
                this->backend_.configure_audio_backend(
                    {soundCard->driver, soundCard->deviceId,
                     soundCard->sampleRate, soundCard->periodSize,
                     soundCard->numPeriods});
                // We have to remove all existing tracks and propagate new
                // tracks
                std::vector<ds::json::ov_track_t> existingTracks =
                    this->store_->readOvTracks();
                for (const auto &track : existingTracks) {
                  this->sendAsync("remove-track", track._id);
                }
                for (const auto &channel : soundCard->inputChannels) {
                  this->createTrack(soundCard->_id, channel);
                }
              } else {
                std::cerr << "Race condition or logical error: could not find "
                             "sound-card propagated by device update"
                          << std::endl;
              }
            }

            if (payload.contains("sendAudio") &&
                payload["sendAudio"] != localDevice->sendAudio) {
              if (payload["sendAudio"]) {
                // TODO: Connect ov
                std::cout << "[TODO] Start sending and receiving" << std::endl;
              } else {
                // TODO: Disconnect ov
                std::cout << "[TODO] Stop sending and receiving" << std::endl;
              }
            }
            this->store_->updateLocalDevice(payload);
          }
        } else if (event == "stage-joined") {
          if (payload.contains("users") && payload["users"].is_array()) {
            for (const nlohmann::json &i : payload["users"]) {
              this->store_->createUser(i);
            }
          }
          if (payload.contains("stages") && payload["stages"].is_array()) {
            for (const nlohmann::json &i : payload["stages"]) {
              this->store_->createStage(i);
            }
          }
          this->store_->setCurrentStageId(payload["stageId"]);
          if (payload.contains("groups") && payload["groups"].is_array()) {
            for (const nlohmann::json &i : payload["groups"]) {
              this->store_->createGroup(i);
            }
          }
          if (payload.contains("customGroups") &&
              payload["customGroups"].is_array()) {
            for (const nlohmann::json &i : payload["customGroups"]) {
              this->store_->createCustomGroup(i);
            }
          }
          if (payload.contains("stageMembers") &&
              payload["stageMembers"].is_array()) {
            for (const nlohmann::json &i : payload["stageMembers"]) {
              this->store_->createStageMember(i);
            }
          }
          if (payload.contains("customStageMembers") &&
              payload["customStageMembers"].is_array()) {
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
          if (payload.contains("remoteOvTracks") &&
              payload["remoteOvTracks"].is_array()) {
            for (const nlohmann::json &i : payload["remoteOvTracks"]) {
              this->store_->createRemoteOvTrack(i);
            }
          }
          if (payload.contains("customRemoteOvTracks") &&
              payload["customRemoteOvTracks"].is_array()) {
            for (const nlohmann::json &i : payload["customRemoteOvTracks"]) {
              this->store_->createCustomRemoteOvTrack(i);
            }
          }

          this->backend_.start_audiobackend();
          this->backend_.start_session();

          this->syncLocalStageMember();
          this->syncRemoteStageMembers();
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
          this->store_->setLocalUser(payload);
        } else if (event == "stage-added") {
          this->store_->createStage(payload);
        } else if (event == "stage-changed") {
          const std::string stageId = payload["_id"].get<std::string>();
          const std::string currentStageId = this->store_->getCurrentStageId();
          this->store_->updateStage(stageId, payload);
          if (currentStageId == stageId) {
            // TODO: Verify, if just setting again works
            boost::optional<const ds::json::stage_t> stage =
                this->store_->readStage(currentStageId);
            if (stage) {
              this->backend_.set_relay_server(stage->ovServer.ipv4,
                                              stage->ovServer.port,
                                              stage->ovServer.pin);
              // TODO: Update stage render settings
              // this->backend_.set_render_settings()
            }
          }
        } else if (event == "stage-removed") {
          this->store_->removeStage(payload.get<std::string>());
        } else if (event == "user-added") {
          this->store_->createUser(payload);
        } else if (event == "user-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store_->updateUser(_id, payload);
        } else if (event == "user-removed") {
          const std::string _id = payload.get<std::string>();
          this->store_->removeUser(_id);
        } else if (event == "group-added") {
          this->store_->createGroup(payload);
          if (this->isSendingAudio()) {
            // TODO: Change volume and position of all tracks of the related
            // stage member
          }
        } else if (event == "group-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store_->updateGroup(_id, payload);
          if (this->isSendingAudio()) {
            // TODO: Change volume and position of all tracks of the related
            // stage member
          }
        } else if (event == "group-removed") {
          const std::string _id = payload.get<std::string>();
          this->store_->removeGroup(_id);
          if (this->isSendingAudio()) {
            // TODO: Change volume and position of all tracks of the related
            // stage member
          }
        } else if (event == "stage-member-added") {
          this->store_->createStageMember(payload);
          if (this->isSendingAudio()) {
            this->syncRemoteStageMembers();
          }
        } else if (event == "stage-member-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store_->updateStageMember(_id, payload);
          if (this->isSendingAudio()) {
            // stage member
            if (payload.count("volume") != 0 /* || payload.count("muted") != 0 */ ) {
              this->syncStageMemberVolume(_id);
            }
            if (payload.count("x") != 0
                || payload.count("z") != 0
                || payload.count("y") != 0
                || payload.count("rX") != 0
                || payload.count("rY") != 0
                || payload.count("yZ") != 0
                ) {
              this->syncStageMemberPosition(_id);
            }
          }
        } else if (event == "stage-member-removed") {
          const std::string _id = payload.get<std::string>();
          this->store_->removeStageMember(_id);
          if (this->isSendingAudio()) {
            this->syncRemoteStageMembers();
          }
        } else if (event == "custom-stage-member-added") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store_->createCustomStageMember(payload);
          if (this->isSendingAudio()) {
            this->syncStageMemberVolume(payload["stageMemberId"]);
            this->syncStageMemberPosition(payload["stageMemberId"]);
          }
        } else if (event == "custom-stage-member-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store_->updateCustomStageMember(_id, payload);
          if (this->isSendingAudio()) {
            const auto customStageMember = this->store_->readCustomStageMember(_id);
            if (customStageMember) {
              if (payload.count("volume") != 0 /* || payload.count("muted") != 0 */ ) {
                this->syncStageMemberVolume(customStageMember->stageId);
              }
              if (payload.count("x") != 0
                  || payload.count("z") != 0
                  || payload.count("y") != 0
                  || payload.count("rX") != 0
                  || payload.count("rY") != 0
                  || payload.count("yZ") != 0
                  ) {
                this->syncStageMemberPosition(customStageMember->stageId);
              }
            } else {
              std::cerr << "Error: custom stage member not available" << std::endl;
            }
          }
        } else if (event == "custom-stage-member-removed") {
          const std::string _id = payload.get<std::string>();
          const auto customStageMember = this->store_->readCustomStageMember(_id);
          this->store_->removeCustomStageMember(_id);
          if (this->isSendingAudio()) {
            if (customStageMember) {
              if (payload.count("volume") != 0 /* || payload.count("muted") != 0 */ ) {
                this->syncStageMemberVolume(customStageMember->stageId);
              }
              if (payload.count("x") != 0
                  || payload.count("z") != 0
                  || payload.count("y") != 0
                  || payload.count("rX") != 0
                  || payload.count("rY") != 0
                  || payload.count("yZ") != 0
                  ) {
                this->syncStageMemberPosition(customStageMember->stageId);
              }
            } else {
              std::cerr << "Error: custom stage member not available" << std::endl;
            }
          }
        } else if (event == "stage-member-ov-added") {
          this->store_->createRemoteOvTrack(payload);
          if (this->isSendingAudio()) {
            this->syncRemoteStageMembers();
          }
        } else if (event == "stage-member-ov-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store_->updateRemoteOvTrack(_id, payload);
          auto ovTrack = this->store_->readRemoteOvTrack(_id);
          if (this->isSendingAudio()) {
            // TODO: Change stage device of backend
            // TODO: This means removal and addition
            std::cout << "TODO: Remove and add stage device "
                      << ovTrack->ovTrackId << std::endl;
          }
        } else if (event == "stage-member-ov-removed") {
          const std::string _id = payload.get<std::string>();
          auto ovTrack = this->store_->readRemoteOvTrack(_id);
          this->store_->removeRemoteOvTrack(_id);
          if (this->isSendingAudio()) {
            this->syncRemoteStageMembers();
          }
        } else if (event == "custom-stage-member-ov-added") {
          this->store_->createCustomRemoteOvTrack(payload);
          if (this->isSendingAudio()) {
            // TODO: Change volume and position of the assigned track (and its
            // stage device)
            std::cout << "TODO: Change volume and position of the assigned "
                         "track (and its stage device)"
                      << std::endl;
          }
        } else if (event == "custom-stage-member-ov-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store_->updateCustomRemoteOvTrack(_id, payload);
          if (this->isSendingAudio()) {
            // TODO: Change volume and position of the assigned track (and its
            // stage device)
            std::cout << "TODO: Change volume and position of the assigned "
                         "track (and its stage device)"
                      << std::endl;
          }
        } else if (event == "custom-stage-member-ov-removed") {
          const std::string _id = payload.get<std::string>();
          this->store_->removeCustomRemoteOvTrack(_id);
          if (this->isSendingAudio()) {
            // TODO: Change volume and position of the assigned track (and its
            // stage device)
            std::cout << "TODO: Change volume and position of the assigned "
                         "track (and its stage device)"
                      << std::endl;
          }
        } else if (event == "sound-card-added") {
          this->store_->createSoundCard(payload);
        } else if (event == "sound-card-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store_->updateSoundCard(_id, payload);
          const auto soundCard = this->store_->readSoundCard(_id);
          const auto localDevice = this->store_->getLocalDevice();
          if (soundCard && localDevice) {
            if (localDevice && localDevice->sendAudio == true &&
                localDevice->soundCardName == soundCard->name) {
              bool isChannelConfigurationDifferent = false;
              std::vector<ds::json::ov_track_t> existingTracks =
                  this->store_->readOvTracks();
              // Remove obsolete tracks (channel has been removed from sound
              // card for these tracks)
              for (const auto &track : existingTracks) {
                auto it =
                    std::find(soundCard->inputChannels.begin(),
                              soundCard->inputChannels.end(), track.channel);
                if (it == soundCard->inputChannels.end()) {
                  isChannelConfigurationDifferent = true;
                  this->sendAsync("remove-track", track._id);
                }
              }
              // Add missing tracks (channel has been activated on sound card)
              for (const unsigned int &channel : soundCard->inputChannels) {
                auto it = std::find_if(
                    existingTracks.begin(), existingTracks.end(),
                    [channel](const ds::json::ov_track_t &existingTrack) {
                      return existingTrack.channel == channel;
                    });
                if (it == existingTracks.end()) {
                  isChannelConfigurationDifferent = true;
                  this->createTrack(soundCard->_id, channel);
                }
              }
              // TODO: We could do this also when add-track is called (see
              // bellow)
              if (isChannelConfigurationDifferent) {
                this->syncLocalStageMember();
              }
            }
          } else {
            std::cerr << "Logical error: updated sound card is NOT available";
          }
        } else if (event == "sound-card-removed") {
          const std::string _id = payload.get<std::string>();
          this->store_->removeSoundCard(_id);
        } else if (event == "track-added") {
          this->store_->createOvTrack(payload);
          // TODO: Device, if we should add tracks here or when the sound card
          // has been updated / switched (see above)
        } else if (event == "track-changed") {
          const std::string _id = payload["_id"].get<std::string>();
          this->store_->updateOvTrack(_id, payload);
        } else if (event == "track-removed") {
          const std::string _id = payload.get<std::string>();
          this->store_->removeOvTrack(_id);
        } else {
          ucout << "Not supported: [" << event << "] " << payload.dump()
                << std::endl;
        }
      }
      catch (const std::exception &e) {
        std::cerr << "std::exception: " << e.what() << std::endl;
      }
      catch (...) {
        std::cerr << "error parsing" << std::endl;
      }
    }
  });

  // utility::string_t close_reason;
  wsclient_.set_close_handler([](websocket_close_status status,
                                 const utility::string_t &reason,
                                 const std::error_code &code) {
    ucout << " closing reason..." << reason << "\n";
    ucout << "connection closed, reason: " << reason
          << " close status: " << int(status) << " error code " << code
          << std::endl;
  });

  // Register sound card handler
  // this->soundio->on_devices_change = this->on_sound_devices_change;

  // Get mac address and local ip
  std::string mac(backend_.get_deviceid());

  // Initial call with device
  nlohmann::json deviceJson;
  deviceJson["mac"] = mac; // MAC = device id (!)
  deviceJson["canVideo"] = false;
  deviceJson["canAudio"] = true;
  deviceJson["canOv"] = true;
  deviceJson["sendAudio"] = true;
  deviceJson["sendVideo"] = false;
  deviceJson["receiveAudio"] = true;
  deviceJson["receiveVideo"] = false;

  nlohmann::json identificationJson;
  identificationJson["token"] = this->token_;
  identificationJson["device"] = deviceJson;
  this->sendAsync("token", identificationJson.dump());

  // RECEIVE TILL END
  receive_task.wait();
}

void ds::ds_service_t::on_sound_devices_change() {
  ucout << "SOUNDCARD CHANGED" << std::endl;
  ucout << "Have now "
        << this->sound_card_tools_->get_input_sound_devices().size()
        << " input soundcards" << std::endl;
}

void ds::ds_service_t::send(const std::string &event,
                            const std::string &message) {
  websocket_outgoing_message msg;
  std::string body_str(R"({"type":0,"data":[")" + event + "\"," + message +
      "]}");
  msg.set_utf8_message(body_str);
  ucout << std::endl << "[SENDING] " << body_str << std::endl << std::endl;
  wsclient_.send(msg).wait();
}

void ds::ds_service_t::sendAsync(const std::string &event,
                                 const std::string &message) {
  websocket_outgoing_message msg;
  std::string body_str(R"({"type":0,"data":[")" + event + "\"," + message +
      "]}");
  msg.set_utf8_message(body_str);
  ucout << std::endl << "[SENDING] " << body_str << std::endl << std::endl;
  wsclient_.send(msg);
}

bool ds::ds_service_t::isSendingAudio() {
  return this->backend_.is_session_active();
}

void ds::ds_service_t::createTrack(const std::string &soundCardId,
                                   unsigned int channel) {
  nlohmann::json trackJson = {{"soundCardId", soundCardId},
                              {"channel", channel}};
  this->sendAsync("add-track", trackJson);
}

void ds::ds_service_t::syncLocalStageMember() {
  std::cout << "SYNC Local Stage Member" << std::endl;
  boost::optional<const ds::json::device_t> localDevice =
      this->store_->getLocalDevice();
  boost::optional<const ds::json::user_t> localUser =
      this->store_->getLocalUser();

  if (!localDevice) {
    std::cerr << "Local device not set yet - server side error?" << std::endl;
    return;
  }
  if (!localUser) {
    std::cerr << "Local user not set yet - server side error?" << std::endl;
    return;
  }

  boost::optional<const ds::json::stage_member_t> currentStageMember =
      this->store_->getCurrentStageMember();

  std::cout << "LOCAL DEVICE: " << localDevice.get()._id << std::endl;
  std::cout << "STAGE MEMBER: " << currentStageMember.get()._id << std::endl;

  if (currentStageMember) {
    // we have to create the tracks, maybe the decorating events has been
    // thrown...
    boost::optional<const ds::json::group_t> group =
        this->store_->readGroup(currentStageMember->groupId);
    boost::optional<const ds::json::custom_stage_member_t> customStageMember =
        this->store_->getCustomStageMemberByStageMemberId(
            currentStageMember->_id);
    boost::optional<const ds::json::custom_group_t> customGroup =
        this->store_->getCustomGroupByGroupId(group->_id);

    const std::vector<const ds::json::remote_ov_track_t> tracks =
        this->store_->getRemoteOvTracksByStageMemberId(currentStageMember->_id);

    std::vector<device_channel_t> deviceChannels;
    // Now for all tracks
    for (const auto &track : tracks) {
      std::cout << "Adding local track " << track.channel << std::endl;
      // Look for custom track
      boost::optional<const ds::json::custom_remote_ov_track_t> customTrack =
          this->store_->getCustomOvTrackByOvTrackId(track._id);
      if (customTrack) {
        deviceChannels.push_back(
            {track._id,
             "", // TODO: What is sourceport
             customTrack->volume,
             {customTrack->x, customTrack->y, customTrack->z},
             customTrack->directivity});
      } else {
        deviceChannels.push_back({track._id,
                                  "", // TODO: What is sourceport
                                  track.volume,
                                  {track.x, track.y, track.z},
                                  track.directivity});
      }
    }

    // Calculate stage member volume
    double gain = customStageMember ? customStageMember->volume
                                    : currentStageMember->volume;
    gain = customGroup ? (customGroup->volume * gain) : (group->volume * gain);

    bool muted;
    pos_t position{};
    zyx_euler_t orientation{};
    if (customStageMember) {
      muted = customStageMember->muted;
      position = {
          customStageMember->x,
          customStageMember->y,
          customStageMember->z,
      };
      orientation = {
          customStageMember->rZ,
          customStageMember->rY,
          customStageMember->rX,
      };
    } else {
      muted = currentStageMember->muted;
      position = {
          currentStageMember->x,
          currentStageMember->y,
          currentStageMember->z,
      };
      orientation = {
          currentStageMember->rZ,
          currentStageMember->rY,
          currentStageMember->rX,
      };
    }
    if (customGroup) {
      muted = customGroup->muted || muted;
      position = {position.x * customGroup->x, position.y * customGroup->y,
                  position.z * customGroup->z};
      orientation = {
          orientation.z * customGroup->rZ,
          orientation.y * customGroup->rY,
          orientation.x * customGroup->rX,
      };
    } else {
      muted = group->muted || muted;
      position = {position.x * group->x, position.y * group->y,
                  position.z * group->z};
      orientation = {
          orientation.z * group->rZ,
          orientation.y * group->rY,
          orientation.x * group->rX,
      };
    }

    this->backend_.set_thisdev({
                                   currentStageMember->ovStageDeviceId, localUser->name, deviceChannels,
                                   position, orientation, gain, muted, localDevice->senderJitter,
                                   localDevice->receiverJitter,
                                   true // sendlocal always true?
                               });
  } else {
    std::cout << "Nothing to do here - not on a stage" << std::endl;
  }
}

void ds::ds_service_t::syncRemoteStageMembers() {
  boost::optional<const ds::json::stage_t> stage = this->store_->getCurrentStage();
  boost::optional<const ds::json::device_t> localDevice =
      this->store_->getLocalDevice();
  if (localDevice && stage) {
    std::map<stage_device_id_t, stage_device_t> stageDeviceMap;

    const std::vector<const ds::json::stage_member_t> stageMembers = this->store_->readStageMembersByStage(stage->_id);

    for (const auto &stageMember : stageMembers) {
      boost::optional<const ds::json::user_t> user =
          this->store_->readUser(stageMember.userId);
      boost::optional<const ds::json::group_t> group =
          this->store_->readGroup(stageMember.groupId);
      boost::optional<const ds::json::custom_stage_member_t> customStageMember =
          this->store_->getCustomStageMemberByStageMemberId(
              stageMember._id);
      boost::optional<const ds::json::custom_group_t> customGroup =
          this->store_->getCustomGroupByGroupId(group->_id);
      const std::vector<const ds::json::remote_ov_track_t> tracks =
          this->store_->getRemoteOvTracksByStageMemberId(stageMember._id);

      std::vector<device_channel_t> deviceChannels;
      for (const auto &track : tracks) {
        // Look for custom track
        boost::optional<const ds::json::custom_remote_ov_track_t> customTrack =
            this->store_->getCustomOvTrackByOvTrackId(track._id);
        if (customTrack) {
          deviceChannels.push_back(
              {track._id,
               "", // TODO: What is sourceport
               customTrack->volume,
               {customTrack->x, customTrack->y, customTrack->z},
               customTrack->directivity});
        } else {
          deviceChannels.push_back({track._id,
                                    "", // TODO: What is sourceport
                                    track.volume,
                                    {track.x, track.y, track.z},
                                    track.directivity});
        }
      }

      // Calculate stage member volume
      double gain = customStageMember ? customStageMember->volume
                                      : stageMember.volume;
      gain = customGroup ? (customGroup->volume * gain) : (group->volume * gain);

      bool muted;
      pos_t position{};
      zyx_euler_t orientation{};
      if (customStageMember) {
        muted = customStageMember->muted;
        position = {
            customStageMember->x,
            customStageMember->y,
            customStageMember->z,
        };
        orientation = {
            customStageMember->rZ,
            customStageMember->rY,
            customStageMember->rX,
        };
      } else {
        muted = stageMember.muted;
        position = {
            stageMember.x,
            stageMember.y,
            stageMember.z,
        };
        orientation = {
            stageMember.rZ,
            stageMember.rY,
            stageMember.rX,
        };
      }
      if (customGroup) {
        muted = customGroup->muted || muted;
        position = {position.x * customGroup->x, position.y * customGroup->y,
                    position.z * customGroup->z};
        orientation = {
            orientation.z * customGroup->rZ,
            orientation.y * customGroup->rY,
            orientation.x * customGroup->rX,
        };
      } else {
        muted = group->muted || muted;
        position = {position.x * group->x, position.y * group->y,
                    position.z * group->z};
        orientation = {
            orientation.z * group->rZ,
            orientation.y * group->rY,
            orientation.x * group->rX,
        };
      }

      stage_device_t stageDevice = {
          stageMember.ovStageDeviceId,
          user ? user->name : stageMember._id,
          deviceChannels,
          position,
          orientation,
          gain,
          muted,
          localDevice->senderJitter,
          localDevice->receiverJitter,
          stageMember.sendlocal
      };
      stageDeviceMap[stageDevice.id] = stageDevice;
    }
    this->backend_.set_stage(stageDeviceMap);
  } else {
    std::cout << "Nothing to do here - not on a stage" << std::endl;
  }
}

void ds::ds_service_t::syncStageMemberPosition(const std::string &stageMemberId) {
  boost::optional<const ds::json::stage_member_t> stageMember = this->store_->readStageMember(stageMemberId);

  if (stageMember) {
    boost::optional<const ds::json::group_t> group =
        this->store_->readGroup(stageMember->groupId);
    boost::optional<const ds::json::custom_stage_member_t> customStageMember =
        this->store_->getCustomStageMemberByStageMemberId(
            stageMember->_id);
    boost::optional<const ds::json::custom_group_t> customGroup =
        this->store_->getCustomGroupByGroupId(group->_id);
    pos_t position{};
    zyx_euler_t orientation{};
    if (customStageMember) {
      position = {
          customStageMember->x,
          customStageMember->y,
          customStageMember->z,
      };
      orientation = {
          customStageMember->rZ,
          customStageMember->rY,
          customStageMember->rX,
      };
    } else {
      position = {
          stageMember->x,
          stageMember->y,
          stageMember->z,
      };
      orientation = {
          stageMember->rZ,
          stageMember->rY,
          stageMember->rX,
      };
    }
    if (customGroup) {
      position = {position.x * customGroup->x, position.y * customGroup->y,
                  position.z * customGroup->z};
      orientation = {
          orientation.z * customGroup->rZ,
          orientation.y * customGroup->rY,
          orientation.x * customGroup->rX,
      };
    } else {
      position = {position.x * group->x, position.y * group->y,
                  position.z * group->z};
      orientation = {
          orientation.z * group->rZ,
          orientation.y * group->rY,
          orientation.x * group->rX,
      };
    }
    this->backend_.set_stage_device_position(stageMember->ovStageDeviceId, position, orientation);
  }
}

void ds::ds_service_t::syncStageMemberVolume(const std::string &stageMemberId) {
  boost::optional<const ds::json::stage_member_t> stageMember = this->store_->readStageMember(stageMemberId);

  if (stageMember) {
    boost::optional<const ds::json::group_t> group =
        this->store_->readGroup(stageMember->groupId);
    boost::optional<const ds::json::custom_stage_member_t> customStageMember =
        this->store_->getCustomStageMemberByStageMemberId(
            stageMember->_id);
    boost::optional<const ds::json::custom_group_t> customGroup =
        this->store_->getCustomGroupByGroupId(group->_id);
    double gain = customStageMember ? customStageMember->volume
                                    : stageMember->volume;
    gain = customGroup ? (customGroup->volume * gain) : (group->volume * gain);
    bool muted = (customStageMember ? customStageMember->muted : stageMember->muted)
        || (customGroup ? customGroup->muted : group->muted);
    //TODO: mute is MISSING!!!
    this->backend_.set_stage_device_gain(stageMember->ovStageDeviceId, gain);
  }
}