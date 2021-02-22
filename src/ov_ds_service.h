#ifndef OV_DS_SERVICE
#define OV_DS_SERVICE

#include "ov_types.h"
#include "ov_ds_store.h"
#include <atomic>
#include <thread>

class ov_ds_service_t {
    struct three_dimensional_t {
        double volume;
        bool muted;
        double x;
        double y;
        double z;
        double rX;
        double rY;
        double rZ;
    };
    struct group_t {
        std::string &name;
        struct three_dimensional_t properties;
    };
    struct custom_group_t {
        struct three_dimensional_t properties;
    };

    struct soundcard_t {
        std::string &userId;
        std::string &name;
        std::string &driver; //'JACK' | 'ALSA' | 'ASIO' | 'WEBRTC'
        int numInputChannels;
        int numOutputChannels;
        std::string &trackPreset;
        double sampleRate;
        double periodSize;
        int numPeriods;
    };
    struct track_preset_t {
        std::string &userId;
        std::string &soundCardId;
        std::string &name;
        std::vector<std::string> outputChannels;
    };
    struct track_t {
        std::string &trackPresetId;
        int channel;
        bool online;
        double gain;
        std::string &directivity;
    };
    struct stage_member_ov_t {
        std::string stageMemberId;
        struct track_t track;
    };

    struct user_t {
        std::string name;
    };
    struct stage_member_t {
        std::string name;
    };

public:
    ov_ds_service_t(const std::string &api_url);

    ~ov_ds_service_t();

    void start(const std::string &token);

    void stop();

private:
    void service();

    bool running_;
    std::thread servicethread_;
    std::string api_url_;
    std::string token_;
    std::atomic<bool> quitrequest_;
    std::map<std::string, stage_member_t> stage_members_;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
