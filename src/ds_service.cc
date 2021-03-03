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
    this->wsclient.close(); // wss client is closed
    this->servicethread_.join(); // thread is joined
}

void ds::ds_service_t::service() {
    wsclient.connect(U(this->api_url_)).wait();

    auto receive_task = create_task(tce);

    // handler for incoming d-s messages
    wsclient.set_message_handler([&](const websocket_incoming_message &ret_msg) {
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
                                    {"label",             sound_device.name},
                                    {"numInputChannels",  sound_device.num_input_channels},
                                    {"numOutputChannels", sound_device.num_output_channels},
                                    {"sampleRate",        sound_device.sample_rate},
                                    {"sampleRates",       sound_device.sample_rates},
                                    {"isDefault",         sound_device.is_default}
                            };
                            this->sendAsync("set-sound-card", soundcard.dump());
                        }
                        this->sendAsync("update-device", deviceUpdate.dump());
                    } else {
                        std::cerr << "WARNING: No soundcards available!" << std::endl;
                    }
                } else if (event == "device-changed") {
                    boost::optional<const ds::json::device_t> localDevice = this->store_->getLocalDevice();
                    if (localDevice && (*localDevice)._id == payload["_id"]) {
                        // The events refers this device
                        std::cout << "DEVICE CHANGED" << std::endl;
                        if (payload.contains("soundCardName") &&
                            payload["soundCardName"] != (*localDevice).soundCardName) {
                            // Soundcard changes
                            //TODO: Restart JACK with soundCardName as identifier
                        }

                        if (payload.contains("sendAudio") &&
                            payload["sendAudio"] != (*localDevice).sendAudio) {
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
                            const std::string _id = i["_id"].get<std::string>();
                            this->store_->createStage(i);
                        }
                    }
                    /*
                    if (payload.contains("groups") && payload["groups"].is_array()) {
                        for (const nlohmann::json &i : payload["groups"]) {
                            const std::string _id = i["_id"].get<std::string>();
                            this->store.groups[_id] = i;
                        }
                    }
                    if (payload.contains("customGroups") && payload["customGroups"].is_array()) {
                        for (const nlohmann::json &i : payload["customGroups"]) {
                            const std::string _id = i["_id"].get<std::string>();
                            this->store.customGroups[_id] = i;
                        }
                    }
                    if (payload.contains("stageMembers") && payload["stageMembers"].is_array()) {
                        for (const nlohmann::json &i : payload["stageMembers"]) {
                            const std::string _id = i["_id"].get<std::string>();
                            this->store.stageMembers[_id] = i;
                        }
                    }
                    if (payload.contains("customStageMembers") && payload["customStageMembers"].is_array()) {
                        for (const nlohmann::json &i : payload["customStageMembers"]) {
                            const std::string _id = i["_id"].get<std::string>();
                            this->store.customStageMembers[_id] = i;
                            this->store.customStageMemberByStageMember[_id] = i["stageMemberId"];
                        }
                    }
                    if (payload.contains("ovTracks") && payload["ovTracks"].is_array()) {
                        for (const nlohmann::json &i : payload["ovTracks"]) {
                            const std::string _id = i["_id"].get<std::string>();
                            this->store.stageMemberTracks[_id] = i;
                        }
                    }
                    if (payload.contains("customOvTracks") && payload["customOvTracks"].is_array()) {
                        for (const nlohmann::json &i : payload["customOvTracks"]) {
                            const std::string _id = i["_id"].get<std::string>();
                            this->store.customStageMemberTracks[_id] = i;
                        }
                    }
                    if (this->is_sending_audio()) {
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
                    /*
                    this->store.customGroups.clear();
                    this->store.stageMembers.clear();
                    this->store.customStageMembers.clear();
                    this->store.customStageMemberByStageMember.clear();
                    this->store.stageMemberTracks.clear();
                    this->store.customStageMemberTracks.clear();
                     */
                } else if (event == "user-ready") {

                } else if (event == "stage-added") {
                    this->store_->createStage(payload);
                } else if (event == "stage-changed") {
                    const std::string stageId = payload["_id"].get<std::string>();
                    const std::string currentStageId = this->store_->getCurrentStageId();
                    this->store_->updateStage(stageId, payload);
                    if (currentStageId == stageId) {
                        //TODO: Verify, if just setting again works
                        boost::optional<const ds::json::stage_t> stage = this->store_->readStage(currentStageId);
                        if( stage ) {
                            this->backend_.set_relay_server((*stage).ovServer.ipv4, (*stage).ovServer.port,
                                                            (*stage).ovServer.pin);
                            //TODO: Update stage render settings
                            //this->backend_.set_render_settings()
                        }
                    }
                } else if (event == "stage-removed") {
                    this->store_->removeStage(payload.get<std::string>());
                    /*
                } else if (event == "group-added") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->storegroups[_id] = payload;
                    if (this->is_sending_audio()) {
                        //TODO: Change volume and position of all tracks of the related stage member
                    }
                } else if (event == "group-changed") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.groups[_id].merge_patch(payload);
                    if (this->is_sending_audio()) {
                        //TODO: Change volume and position of all tracks of the related stage member
                    }
                } else if (event == "group-removed") {
                    const std::string _id = payload.get<std::string>();
                    this->store.groups.erase(_id);
                    if (this->is_sending_audio()) {
                        //TODO: Change volume and position of all tracks of the related stage member
                    }
                } else if (event == "stage-member-added") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.stageMembers[_id] = payload;
                    if (this->is_sending_audio()) {
                        //TODO: Change volume and position of all tracks of the related stage member
                    }
                } else if (event == "stage-member-changed") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.stageMembers[_id].merge_patch(payload);
                    if (this->is_sending_audio()) {
                        //TODO: Change volume and position of all tracks of the related stage member
                    }
                } else if (event == "stage-member-removed") {
                    const std::string _id = payload.get<std::string>();
                    this->store.stageMembers.erase(_id);
                    if (this->is_sending_audio()) {
                        //TODO: Change volume and position of all tracks of the related stage member
                    }
                } else if (event == "custom-stage-member-added") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.customStageMembers[_id] = payload;
                    if (this->is_sending_audio()) {
                        //TODO: Change volume and position of all tracks of the related stage member
                    }
                } else if (event == "custom-stage-member-changed") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.customStageMembers[_id].merge_patch(payload);
                    if (this->is_sending_audio()) {
                        //TODO: Change volume and position of all tracks of the related stage member
                        std::cout << "TODO: Change volume and position of all tracks of the related stage member"
                                  << std::endl;
                    }
                } else if (event == "custom-stage-member-removed") {
                    const std::string _id = payload.get<std::string>();
                    this->store.customStageMembers.erase(_id);
                    if (this->is_sending_audio()) {
                        //TODO: Change volume and position of all tracks of the related stage member
                        std::cout << "TODO: Change volume and position of all tracks of the related stage member"
                                  << std::endl;
                    }
                } else if (event == "stage-member-ov-added") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.stageMemberTracks[_id] = payload;
                    if (this->is_sending_audio()) {
                        //TODO: Add track as stage device to backend
                        std::cout << "TODO: Add track " << _id << " as stage device " << payload["ovId"]
                                  << " to backend" << std::endl;
                    }
                } else if (event == "stage-member-ov-changed") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.stageMemberTracks[_id].merge_patch(payload);
                    if (this->is_sending_audio()) {
                        //TODO: Change stage device of backend
                        //TODO: This means removal and addition
                        ds::json::stage_member_ov_t ovTrack = this->store.stageMemberTracks[_id].get<ds::json::stage_member_ov_t>();
                        std::cout << "TODO: Remove and add stage device " << ovTrack.ovId << std::endl;
                    }
                } else if (event == "stage-member-ov-removed") {
                    const std::string _id = payload.get<std::string>();
                    this->store.stageMemberTracks.erase(_id);
                    if (this->is_sending_audio()) {
                        //TODO: Remove stage device of backend
                        ds::json::stage_member_ov_t ovTrack = this->store.stageMemberTracks[_id].get<ds::json::stage_member_ov_t>();
                        std::cout << "TODO: Remove stage device " << ovTrack.ovId << std::endl;
                    }
                } else if (event == "custom-stage-member-ov-added") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.customStageMemberTracks[_id] = payload;
                    if (this->is_sending_audio()) {
                        //TODO: Change volume and position of the assigned track (and its stage device)
                        std::cout << "TODO: Change volume and position of the assigned track (and its stage device)" << std::endl;
                    }
                } else if (event == "custom-stage-member-ov-changed") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.customStageMemberTracks[_id].merge_patch(payload);
                    if (this->is_sending_audio()) {
                        //TODO: Change volume and position of the assigned track (and its stage device)
                        std::cout << "TODO: Change volume and position of the assigned track (and its stage device)" << std::endl;
                    }
                } else if (event == "custom-stage-member-ov-removed") {
                    const std::string _id = payload.get<std::string>();
                    this->store.customStageMemberTracks.erase(_id);
                    if (this->is_sending_audio()) {
                        //TODO: Change volume and position of the assigned track (and its stage device)
                        std::cout << "TODO: Change volume and position of the assigned track (and its stage device)" << std::endl;
                    }
                } else if (event == "sound-card-added") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.soundCards[_id] = payload;
                } else if (event == "sound-card-changed") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.soundCards[_id].merge_patch(payload);
                    if (this->store.localDevice
                        && this->store.localDevice["sendAudio"]
                        && this->store.localDevice["sendAudio"] == true
                        && this->store.localDevice.contains("soundCardId")
                        && this->store.localDevice["soundCardId"] == _id) {
                        //TODO: Change sound card settings, presets etc.
                    }
                } else if (event == "sound-card-removed") {
                    const std::string _id = payload.get<std::string>();
                    this->store.soundCards.erase(_id);
                } else if (event == "track-preset-added") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.trackPresets[_id] = payload;
                } else if (event == "track-preset-changed") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.trackPresets[_id].merge_patch(payload);
                } else if (event == "track-preset-removed") {
                    const std::string _id = payload.get<std::string>();
                    this->store.trackPresets.erase(_id);
                    //TODO: DISCUESS IF WE NEED THE FOLLOWING TRACK-HANDLING
                } else if (event == "track-added") {
                    const std::string _id = payload["_id"].get<std::string>();
                    //this->store.stageMemberTracks[_id] = payload;
                } else if (event == "track-changed") {
                    const std::string _id = payload["_id"].get<std::string>();
                    // this->store.tracks[_id].merge_patch(payload);
                } else if (event == "track-removed") {
                    const std::string _id = payload.get<std::string>();
                    //this->store.tracks.erase(_id);*/
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
    wsclient.set_close_handler([](websocket_close_status status,
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
    wsclient.send(msg).wait();
}

void ds::ds_service_t::sendAsync(const std::string &event, const std::string &message) {
    websocket_outgoing_message msg;
    std::string body_str(R"({"type":0,"data":[")" + event + "\"," + message + "]}");
    msg.set_utf8_message(body_str);
    ucout << std::endl << "[SENDING] " << body_str << std::endl << std::endl;
    wsclient.send(msg);
}