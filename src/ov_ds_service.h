#ifndef OV_DS_SERVICE
#define OV_DS_SERVICE

#include "ds_types.h"
#include "ov_types.h"
#include <atomic>
#include <thread>
#include <nlohmann/json.hpp>


struct ov_ds_store_t {
    // Digital Stage, from detailed to outer scope
    std::map<std::string, nlohmann::json> customTracks;
    std::map<std::string, nlohmann::json> tracks;
    std::map<std::string, nlohmann::json> customStageMembers;
    std::map<std::string, nlohmann::json> stageMembers;
    std::map<std::string, nlohmann::json> customGroups;
    std::map<std::string, nlohmann::json> groups;
    std::map<std::string, nlohmann::json> stages;
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


class ov_ds_service_t {



public:
    ov_ds_service_t(const std::string &api_url);

    ~ov_ds_service_t();

    void start(const std::string &token);

    void stop();

private:
    void service();

    void syncStageDevices();

    bool running_;
    std::thread servicethread_;
    std::string api_url_;
    std::string token_;
    std::atomic<bool> quitrequest_;

    ov_ds_store_t store;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
