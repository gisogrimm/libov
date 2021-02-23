#include "ov_ds_service.h"
#include <pplx/pplxtasks.h>
#include "udpsocket.h"

using namespace utility;
using namespace web;
using namespace web::http;
using namespace utility::conversions;
using namespace pplx;
using namespace concurrency::streams;

task_completion_event<void> tce; // used to terminate async PPLX listening task


ov_ds_service_t::ov_ds_service_t(const std::string &api_url) : api_url_(api_url) {
    this->soundio = soundio_create();
    soundio_connect(this->soundio); // use default driver (not always jack)
}

ov_ds_service_t::~ov_ds_service_t() {
    soundio_disconnect(this->soundio);
    delete this->soundio;
}


void ov_ds_service_t::start(const std::string &token) {
    this->token_ = token;
    this->running_ = true;
    this->servicethread_ = std::thread(&ov_ds_service_t::service, this);
}

void ov_ds_service_t::stop() {
    this->running_ = false;
    tce.set();        // task completion event is set closing wss listening task
    this->wsclient.close(); // wss client is closed
    this->servicethread_.join(); // thread is joined
}

void ov_ds_service_t::service() {
    wsclient.connect(U(this->api_url_)).wait();

    auto receive_task = create_task(tce);

    // handler for incoming d-s messages
    wsclient.set_message_handler([&](websocket_incoming_message ret_msg) {
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

                } else if (event == "stage-joined") {

                    if (payload.contains("stages") && payload["stages"].is_array()) {
                        for (const nlohmann::json &i : payload["stages"]) {
                            const std::string _id = i["_id"].get<std::string>();
                            this->store.stages[_id] = i;
                        }
                    }
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
                } else if (event == "stage-left") {
                    // Remove active stage related data
                    this->store.customGroups.clear();
                    this->store.stageMembers.clear();
                    this->store.customStageMembers.clear();
                    this->store.customStageMemberByStageMember.clear();
                    this->store.stageMemberTracks.clear();
                    this->store.customStageMemberTracks.clear();
                } else if (event == "user-ready") {

                } else if (event == "stage-added") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.stages[_id] = payload;
                    ucout << store.stages << std::endl;
                } else if (event == "stage-changed") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.stages[_id].merge_patch(payload);
                } else if (event == "stage-removed") {
                    const std::string _id = payload.get<std::string>();
                    this->store.stages.erase(_id);
                } else if (event == "group-added") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.groups[_id] = payload;
                } else if (event == "group-changed") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.groups[_id].merge_patch(payload);
                } else if (event == "group-removed") {
                    const std::string _id = payload.get<std::string>();
                    this->store.groups.erase(_id);
                } else if (event == "stage-member-added") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.stageMembers[_id] = payload;
                } else if (event == "stage-member-changed") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.stageMembers[_id].merge_patch(payload);
                    ucout << store.stageMembers << std::endl;
                } else if (event == "stage-member-removed") {
                    const std::string _id = payload.get<std::string>();
                    this->store.stageMembers.erase(_id);
                    //TODO: Remove related entities
                } else if (event == "custom-stage-member-added") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.customStageMembers[_id] = payload;
                } else if (event == "custom-stage-member-changed") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.customStageMembers[_id].merge_patch(payload);
                } else if (event == "custom-stage-member-removed") {
                    const std::string _id = payload.get<std::string>();
                    this->store.customStageMembers.erase(_id);
                } else if (event == "stage-member-ov-added") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.stageMemberTracks[_id] = payload;
                } else if (event == "stage-member-ov-changed") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.stageMemberTracks[_id].merge_patch(payload);
                } else if (event == "stage-member-ov-removed") {
                    const std::string _id = payload.get<std::string>();
                    this->store.stageMemberTracks.erase(_id);
                } else if (event == "custom-stage-member-ov-added") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.customStageMemberTracks[_id] = payload;
                } else if (event == "custom-stage-member-ov-changed") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.customStageMemberTracks[_id].merge_patch(payload);
                } else if (event == "custom-stage-member-ov-removed") {
                    const std::string _id = payload.get<std::string>();
                    this->store.customStageMemberTracks.erase(_id);
                } else if (event == "sound-card-added") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.soundCards[_id] = payload;
                } else if (event == "sound-card-changed") {
                    const std::string _id = payload["_id"].get<std::string>();
                    this->store.soundCards[_id].merge_patch(payload);
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
                } else if (event == "track-added") {
                    const std::string _id = payload["_id"].get<std::string>();
                    //this->store.stageMemberTracks[_id] = payload;
                } else if (event == "track-changed") {
                    const std::string _id = payload["_id"].get<std::string>();
                    // this->store.tracks[_id].merge_patch(payload);
                } else if (event == "track-removed") {
                    const std::string _id = payload.get<std::string>();
                    //this->store.tracks.erase(_id);
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

    utility::string_t close_reason;
    wsclient.set_close_handler([&close_reason](websocket_close_status status,
                                               const utility::string_t &reason,
                                               const std::error_code &code) {
        ucout << " closing reason..." << reason << "\n";
        ucout << "connection closed, reason: " << reason
              << " close status: " << int(status) << " error code " << code
              << std::endl;
    });

    // Register sound card handler
    //this->soundio->on_devices_change = this->on_sound_devices_change;

    const std::vector<ds::soundcard> input_sound_devices = this->get_input_sound_devices();
    //const std::vector<ds::soundcard> output_sound_devices = this->get_output_sound_devices();

    // Get mac address and local ip
    std::string macaddress(getmacaddr());
    std::string localIpAddress(ep2ipstr(getipaddr()));
    std::cout << "MAC Address: " << macaddress << " IP: " << localIpAddress << '\n';

    // Initial call with device
    nlohmann::json deviceJson;
    deviceJson["mac"] = macaddress;
    deviceJson["canVideo"] = false;
    deviceJson["canAudio"] = true;
    deviceJson["canOv"] = "true";
    deviceJson["sendAudio"] = "true";
    deviceJson["sendVideo"] = "false";
    deviceJson["receiveAudio"] = "true";
    deviceJson["receiveVideo"] = "false";
    deviceJson["inputVideoDevices"] = nlohmann::json::array();
    deviceJson["inputAudioDevices"] = nlohmann::json::array();
    deviceJson["outputAudioDevices"] = nlohmann::json::array();
    for(int i = 0; i < input_sound_devices.size(); i++) {
        deviceJson["soundCardIds"].push_back(input_sound_devices[i].id);
    }

    nlohmann::json identificationJson;
    identificationJson["token"] = this->token_;
    identificationJson["device"] = deviceJson;
    this->sendAsync("token", identificationJson.dump());

    // Now send sound cards
    for(int i = 0; i < input_sound_devices.size(); i++) {
        deviceJson["soundCardIds"].push_back(input_sound_devices[i].id);

        nlohmann::json soundcard;
        soundcard["name"] = input_sound_devices[i].id;
        soundcard["initial"] = {
                {"name", input_sound_devices[i].name},
                {"label", input_sound_devices[i].name},
                {"numInputChannels", input_sound_devices[i].channel_count},
                {"numOutputChannels", 2}, // Not more supported
                {"sampleRate", input_sound_devices[i].sample_rate_current}
        };
        this->sendAsync("add-sound-card", soundcard.dump());
    }

    receive_task.wait();
}

std::vector<ds::soundcard> ov_ds_service_t::get_input_sound_devices() {
    soundio_flush_events(this->soundio); // Flush the sound devices
    int input_count = soundio_input_device_count(this->soundio);
    int default_input = soundio_default_input_device_index(this->soundio);
    std::vector<ds::soundcard> input_sound_devices;
    for (int i = 0; i < input_count; i += 1) {
        SoundIoDevice *device = soundio_get_input_device(this->soundio, i);
        ucout << "Processing " << device->name << std::endl;
        ds::soundcard soundcard;
        soundcard.id = std::string(device->id);
        soundcard.name = std::string(device->name);
        soundcard.channel_count = device->current_layout.channel_count;
        soundcard.is_default = default_input == i;
        soundcard.sample_rate_current = device->sample_rate_current;
        for (int j = 0; j < device->sample_rate_count; j += 1) {
            struct SoundIoSampleRateRange *range = &device->sample_rates[i];
            soundcard.supported_sample_rates.push_back(range->min);
        }
        soundcard.software_latency_current = device->software_latency_current;
        input_sound_devices.push_back(soundcard);
        soundio_device_unref(device);
    }
    return input_sound_devices;
}

std::vector<ds::soundcard> ov_ds_service_t::get_output_sound_devices() {
    soundio_flush_events(this->soundio); // Flush the sound devices
    int output_count = soundio_output_device_count(this->soundio);
    int default_output = soundio_default_output_device_index(this->soundio);
    std::vector<ds::soundcard> output_sound_devices;
    for (int i = 0; i < output_count; i += 1) {
        SoundIoDevice *device = soundio_get_input_device(this->soundio, i);
        ucout << "Processing " << device->name << std::endl;
        ds::soundcard soundcard;
        soundcard.id = std::string(device->id);
        soundcard.name = std::string(device->name);
        soundcard.channel_count = device->current_layout.channel_count;
        soundcard.is_default = default_output == i;
        soundcard.sample_rate_current = device->sample_rate_current;
        for (int j = 0; j < device->sample_rate_count; j += 1) {
            struct SoundIoSampleRateRange *range = &device->sample_rates[i];
            soundcard.supported_sample_rates.push_back(range->min);
        }
        soundcard.software_latency_current = device->software_latency_current;
        output_sound_devices.push_back(soundcard);
        soundio_device_unref(device);
    }
    return output_sound_devices;
}

void ov_ds_service_t::on_sound_devices_change() {
    ucout << "SOUNDCARD CHANGED" << std::endl;
    ucout << "Have now " << this->get_input_sound_devices().size() << " input soundcards" << std::endl;
}

void ov_ds_service_t::send(const std::string &event, const std::string &message) {
    websocket_outgoing_message msg;
    std::string body_str("{\"type\":0,\"data\":[\"" + event + "\"," + message + "]}");
    msg.set_utf8_message(body_str);
    ucout << "[SENDING] " << body_str << std::endl;
    wsclient.send(msg).wait();
}

void ov_ds_service_t::sendAsync(const std::string &event, const std::string &message) {
    websocket_outgoing_message msg;
    std::string body_str("{\"type\":0,\"data\":[\"" + event + "\"," + message + "]}");
    msg.set_utf8_message(body_str);
    ucout << "[SENDING] " << body_str << std::endl;
    wsclient.send(msg);
}
