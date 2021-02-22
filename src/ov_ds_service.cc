#include "ov_ds_service.h"
#include <pplx/pplxtasks.h>
#include <cpprest/ws_client.h>
#include <nlohmann/json.hpp>
#include "udpsocket.h"

using namespace utility;
using namespace web;
using namespace web::http;
using namespace utility::conversions;
using namespace web::websockets::client;
using namespace pplx;
using namespace concurrency::streams;

using json = nlohmann::json;

task_completion_event<void> tce; // used to terminate async PPLX listening task
websocket_callback_client wsclient;

nlohmann::json user; // jsonObject for user data

nlohmann::json stage;         // jsonObject for joined d-s stage
nlohmann::json stage_members; // jsonObject containing joined d-s stage-members
nlohmann::json stages;        // jsonObject containing d-s stages
nlohmann::json groups;        // jsonObject containing d-s groups
nlohmann::json track_presets; // jsonObject containing d-s track presets
nlohmann::json sound_cards;   // jsonObject array with sound_cards

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
                const json payload = j["data"][1];

                ucout << "[" << event << "] " << payload.dump() << std::endl;
                if (event == "local-device-ready") {

                } else if (event == "stage-joined") {
                    const json customGroups = payload["customGroups"].get<std::vector<custom_group_t>>();
                } else if (event == "stage-left") {

                } else if (event == "user-ready") {

                } else if (event == "group-added") {

                } else if (event == "group-changed") {

                } else if (event == "group-removed") {

                } else if (event == "stage-member-added") {

                } else if (event == "stage-member-changed") {

                } else if (event == "stage-member-removed") {

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
