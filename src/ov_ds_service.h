#ifndef OV_DS_SERVICE
#define OV_DS_SERVICE

#include "ds_types.h"
#include "ov_types.h"
#include <atomic>
#include <thread>
#include <nlohmann/json.hpp>
#include <soundio/soundio.h>

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
        int layout_count;
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

    void watch_sound_devices();

    static std::vector<SoundIoDevice> get_input_sound_devices(SoundIo *soundio);

    static std::vector<SoundIoDevice> get_output_sound_devices(SoundIo *soundio);

    static void on_sound_devices_change(SoundIo *soundio);

    //TODO: REMOVE
    static void print_channel_layout(const SoundIoChannelLayout *layout);
    static void print_device(SoundIoDevice device);
    // END OF TODO

    bool running_;
    std::thread servicethread_;
    std::thread devicewatcherthread_;
    std::string api_url_;
    std::string token_;
    std::atomic<bool> quitrequest_;

    SoundIo* soundio;

    ov_ds_store_t store;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
