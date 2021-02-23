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
}

ov_ds_service_t::~ov_ds_service_t() {
}


void ov_ds_service_t::start(const std::string &token) {
    this->token_ = token;
    this->running_ = true;
    servicethread_ = std::thread(&ov_ds_service_t::service, this);
}

void ov_ds_service_t::stop() {
    this->running_ = false;
    tce.set();        // task completion event is set closing wss listening task
    wsclient.close(); // wss client is closed
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
                            //this->store.customGroups[_id] = i;
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
                            ucout << i.dump() << std::endl;
                        }
                    }
                    if (payload.contains("customOvTracks") && payload["customOvTracks"].is_array()) {
                        for (const nlohmann::json &i : payload["customOvTracks"]) {
                            ucout << i.dump() << std::endl;
                        }
                    }
                } else if (event == "stage-left") {
                    // Remove active stage related data
                    this->store.stageMembers.clear();
                    this->store.customStageMembers.clear();
                    this->store.customStageMemberByStageMember.clear();
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

                } else if (event == "custom-stage-member-changed") {

                } else if (event == "custom-stage-member-removed") {

                } else if (event == "stage-member-ov-added") {

                } else if (event == "stage-member-ov-changed") {

                } else if (event == "stage-member-ov-removed") {

                } else if (event == "custom-stage-member-ov-added") {

                } else if (event == "custom-stage-member-ov-changed") {

                } else if (event == "custom-stage-member-ov-removed") {

                } else if (event == "sound-card-added") {

                } else if (event == "sound-card-changed") {

                } else if (event == "sound-card-removed") {

                } else if (event == "track-preset-added") {

                } else if (event == "track-preset-changed") {

                } else if (event == "track-preset-removed") {

                } else if (event == "track-added") {

                } else if (event == "track-changed") {

                } else if (event == "track-removed") {

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

    // Read mac and ip address
    std::string macaddress(getmacaddr());
    std::cout << "MAC Address: " << macaddress << " IP: " << ep2ipstr(getipaddr()) << '\n';

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

    // this part is never reached as receive_task.wait() is blocking the thread
    // until ov_client_digitalstage_t::stop_service() is called
    // this part is for reference only ! @Giso we can delete the while loop
    while (running_) {
        std::cerr << "Error: not yet implemented." << std::endl;
        quitrequest_ = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void ov_ds_service_t::syncStageDevices() {
    for(const nlohmann::json &tracks : this->store.tracks) {
        //TODO: Use all related entities to calculate and compare ov_stage_device_t
        //TODO: If something changed, set it again
    }
}
