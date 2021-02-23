#ifndef OV_DS_SERVICE
#define OV_DS_SERVICE

#include "ds_types.h"
#include "ov_types.h"
#include <atomic>
#include <thread>
#include <nlohmann/json.hpp>
#include <soundio/soundio.h>
#include <cpprest/ws_client.h>

using namespace web::websockets::client;

struct ov_ds_store_t {
    // Digital Stage, from detailed to outer scope
    std::map<std::string, nlohmann::json> customStageMemberTracks;
    std::map<std::string, nlohmann::json> stageMemberTracks;
    std::map<std::string, nlohmann::json> customStageMembers;
    std::map<std::string, nlohmann::json> stageMembers;
    std::map<std::string, nlohmann::json> customGroups;
    std::map<std::string, nlohmann::json> groups;
    std::map<std::string, nlohmann::json> stages;

    std::map<std::string, nlohmann::json> soundCards;
    std::map<std::string, nlohmann::json> trackPresets;
    nlohmann::json localDevice;
    // Helper
    std::map<std::string, std::string> customTracksByTrack;
    std::map<std::string, std::string> trackByStageMember;
    std::map<std::string, std::string> customStageMemberByStageMember;
    std::map<std::string, std::string> stageMemberByGroup;
    std::map<std::string, std::string> groupByStage;

    // OV / Tascar
    std::vector<stage_device_t> stageDevices;
    std::map<std::string, int> stageDeviceByStageMember;
    stage_t stage;
};

namespace ds {
    struct soundcard {
        std::string id;
        std::string name;
        int channel_count;
        int sample_rate_current;
        std::vector<int> supported_sample_rates;
        double software_latency_current;
        bool is_default;
    };
}

class ov_ds_service_t {

public:
    ov_ds_service_t(const std::string &api_url);

    ~ov_ds_service_t();

    void start(const std::string &token);

    void stop();

private:
    void service();

    std::vector<ds::soundcard> get_input_sound_devices();

    std::vector<ds::soundcard> get_output_sound_devices();

    void on_sound_devices_change();


    // Threading
    bool running_;
    std::thread servicethread_;
    std::atomic<bool> quitrequest_;

    // Soundcard related
    struct SoundIo *soundio;

    // Connection related
    websocket_callback_client wsclient;
    std::string api_url_;
    std::string token_;

    ov_ds_store_t store;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
