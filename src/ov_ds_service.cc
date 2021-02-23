#include "ov_ds_service.h"
#include <pplx/pplxtasks.h>
#include <cpprest/ws_client.h>
#include "udpsocket.h"

using namespace utility;
using namespace web;
using namespace web::http;
using namespace utility::conversions;
using namespace web::websockets::client;
using namespace pplx;
using namespace concurrency::streams;


task_completion_event<void> tce; // used to terminate async PPLX listening task
websocket_callback_client wsclient;

ov_ds_service_t::ov_ds_service_t(const std::string &api_url) : api_url_(api_url) {
    this->soundio = soundio_create();
    soundio_connect(this->soundio); // use default driver (not always jack)
}

ov_ds_service_t::~ov_ds_service_t() {
    delete this->soundio;
}


void ov_ds_service_t::start(const std::string &token) {
    this->token_ = token;
    this->running_ = true;
    this->servicethread_ = std::thread(&ov_ds_service_t::service, this);
    this->devicewatcherthread_ = std::thread(&ov_ds_service_t::watch_sound_devices, this);
}

void ov_ds_service_t::stop() {
    this->running_ = false;
    tce.set();        // task completion event is set closing wss listening task
    wsclient.close(); // wss client is closed
    this->servicethread_.join(); // thread is joined
}

void ov_ds_service_t::service() {
    // Get audio devices
    std::vector<SoundIoDevice> input_sound_devices = get_input_sound_devices(this->soundio);
    std::vector<SoundIoDevice> output_sound_devices = get_output_sound_devices(this->soundio);

    for (auto &input_sound_device : input_sound_devices) {
        print_device(input_sound_device);
    }
    for (auto &output_sound_device : output_sound_devices) {
        print_device(output_sound_device);
    }

    // Get mac address and local ip
    std::string macaddress(getmacaddr());
    std::string localIpAddress(ep2ipstr(getipaddr()));
    std::cout << "MAC Address: " << macaddress << " IP: " << localIpAddress << '\n';

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

    nlohmann::json token_json;

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
    deviceJson["soundCardIds"] = nlohmann::json::array();

    ucout << "Print device json string: \n" + deviceJson.dump() << std::endl;
    ucout << "PrettyPrint device json: \n" + deviceJson.dump(4) << std::endl;

    std::string body_str("{\"type\":0,\"data\":[\"token\",{\"token\":\"" + this->token_ +
                         "\"}]}");

    ucout << "token / device msg: " << body_str << std::endl;
    websocket_outgoing_message msg;
    msg.set_utf8_message(body_str);
    wsclient.send(msg).wait();
    receive_task.wait();

    /*
    // this part is never reached as receive_task.wait() is blocking the thread
    // until ov_client_digitalstage_t::stop_service() is called
    // this part is for reference only ! @Giso we can delete the while loop
    while (running_) {
        std::cerr << "Error: not yet implemented." << std::endl;
        quitrequest_ = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }*/
}

void ov_ds_service_t::watch_sound_devices() {
    // Start sound card watcher
    this->soundio->on_devices_change = on_sound_devices_change;
    while (this->running_) {
        soundio_wait_events(this->soundio);
    }
}

std::vector<SoundIoDevice> ov_ds_service_t::get_input_sound_devices(SoundIo *soundio) {
    int input_count = soundio_input_device_count(soundio);
    std::vector<SoundIoDevice> devices = std::vector<SoundIoDevice>();
    for (int i = 0; i < input_count; i += 1) {
        struct SoundIoDevice *device = soundio_get_input_device(soundio, i);
        devices.push_back(*device);
        soundio_device_unref(device);
    }
    ucout << "Have " << devices.size() << " device" << std::endl;
    return devices;
}

std::vector<SoundIoDevice> ov_ds_service_t::get_output_sound_devices(SoundIo *soundio) {
    int output_count = soundio_output_device_count(soundio);
    std::vector<SoundIoDevice> devices = std::vector<SoundIoDevice>();
    for (int i = 0; i < output_count; i += 1) {
        struct SoundIoDevice *device = soundio_get_output_device(soundio, i);
        devices.push_back(*device);
        soundio_device_unref(device);
    }
    return devices;
}

void ov_ds_service_t::on_sound_devices_change(struct SoundIo *soundio) {
    ucout << "SOUNDCARD CHANGED" << std::endl;

    std::vector<SoundIoDevice> input_sound_devices = get_input_sound_devices(soundio);
    std::vector<SoundIoDevice> output_sound_devices = get_output_sound_devices(soundio);

    for (auto &input_sound_device : input_sound_devices) {
        print_device(input_sound_device);
    }
    for (auto &output_sound_device : output_sound_devices) {
        print_device(output_sound_device);
    }
    //TODO: Update device and create soundcard entities for all available devices
}

void ov_ds_service_t::print_channel_layout(const struct SoundIoChannelLayout *layout) {
    if (layout->name) {
        fprintf(stderr, "%s", layout->name);
    } else {
        fprintf(stderr, "%s", soundio_get_channel_name(layout->channels[0]));
        for (int i = 1; i < layout->channel_count; i += 1) {
            fprintf(stderr, ", %s", soundio_get_channel_name(layout->channels[i]));
        }
    }
}

void ov_ds_service_t::print_device(SoundIoDevice device) {
    const char *raw_str = device.is_raw ? " (raw)" : "";
    fprintf(stderr, "%s%s\n", device.name, raw_str);
    fprintf(stderr, "  id: %s\n", device.id);

    if (device.probe_error) {
        fprintf(stderr, "  probe error: %s\n", soundio_strerror(device.probe_error));
    } else {
        fprintf(stderr, "  channel layouts:\n");
        for (int i = 0; i < device.layout_count; i += 1) {
            fprintf(stderr, "    ");
            print_channel_layout(&device.layouts[i]);
            fprintf(stderr, "\n");
        }
        if (device.current_layout.channel_count > 0) {
            fprintf(stderr, "  current layout: ");
            print_channel_layout(&device.current_layout);
            fprintf(stderr, "\n");
        }

        fprintf(stderr, "  sample rates:\n");
        for (int i = 0; i < device.sample_rate_count; i += 1) {
            struct SoundIoSampleRateRange *range = &device.sample_rates[i];
            fprintf(stderr, "    %d - %d\n", range->min, range->max);

        }
        if (device.sample_rate_current)
            fprintf(stderr, "  current sample rate: %d\n", device.sample_rate_current);
        fprintf(stderr, "  formats: ");
        for (int i = 0; i < device.format_count; i += 1) {
            const char *comma = (i == device.format_count - 1) ? "" : ", ";
            fprintf(stderr, "%s%s", soundio_format_string(device.formats[i]), comma);
        }
        fprintf(stderr, "\n");
        if (device.current_format != SoundIoFormatInvalid)
            fprintf(stderr, "  current format: %s\n", soundio_format_string(device.current_format));

        fprintf(stderr, "  min software latency: %0.8f sec\n", device.software_latency_min);
        fprintf(stderr, "  max software latency: %0.8f sec\n", device.software_latency_max);
        if (device.software_latency_current != 0.0)
            fprintf(stderr, "  current software latency: %0.8f sec\n", device.software_latency_current);

    }
    fprintf(stderr, "\n");
}
