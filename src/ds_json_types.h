#ifndef DS_TYPES
#define DS_TYPES

#include "ov_types.h"
#include <nlohmann/json.hpp>

namespace ds {
    namespace json {

        struct device_t {
            std::string _id;
            std::string userId;
            bool online;
            std::string mac;
            std::string name;
            bool canVideo;
            bool canAudio;
            bool canOv;
            bool sendVideo;
            bool sendAudio;
            bool receiveVideo;
            bool receiveAudio;

            std::string receiverType;   // Either ortf or hrtf

            int senderJitter;
            int receiverJitter;

            bool p2p;

            bool renderReverb;
            double reverbGain;
            bool renderISM;
            bool rawMode;
            double egoGain;

            std::vector<std::string> soundCardIds;
            std::string soundCardId;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(device_t, _id, userId, online, mac, name, canVideo, canAudio, canOv,
                                           sendVideo, sendAudio, receiveVideo, receiveAudio, receiverType, senderJitter,
                                           receiverJitter, p2p, renderReverb, reverbGain, renderISM, rawMode, egoGain,
                                           soundCardIds, soundCardId)

        struct stage_ov_server_t {
            std::string router;
            std::string ipv4;
            std::string ipv6;
            uint16_t port;
            uint32_t pin;
            double serverJitter;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(stage_ov_server_t, router, ipv4, ipv6, port, pin, serverJitter)

        struct stage_t {
            std::string _id;
            std::string name;
            std::vector<std::string> admins;
            std::string password;
            double width;
            double length;
            double height;
            double absorption;
            double damping;
            bool renderAmbient;
            std::string ambientSoundUrl;
            double ambientLevel;
            stage_ov_server_t ovServer;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(stage_t, _id, name, width, length, height, absorption, damping,
                                           renderAmbient, ambientSoundUrl, ambientLevel)

        struct group_t {
            std::string _id;
            std::string name;
            std::string color;
            std::string stageId;
            double volume;
            bool muted;
            double x;
            double y;
            double z;
            double rX;
            double rY;
            double rZ;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(group_t, _id, name, color, stageId, volume, muted, x, y, z, rX, rY, rZ)

        struct custom_group_t {
            std::string _id;
            std::string userId;
            std::string groupId;
            std::string stageId;
            double volume;
            bool muted;
            double x;
            double y;
            double z;
            double rX;
            double rY;
            double rZ;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(custom_group_t, _id, userId, groupId, stageId, volume, muted, x, y, z, rX,
                                           rY, rZ)

        struct stage_member_t {
            std::string _id;
            std::string groupId;
            std::string userId;
            bool online;
            bool isDirector;
            stage_device_id_t ovStageDeviceId;
            bool sendlocal;
            std::string stageId;
            double volume;
            bool muted;
            double x;
            double y;
            double z;
            double rX;
            double rY;
            double rZ;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(stage_member_t, _id, groupId, userId, online, isDirector, ovStageDeviceId,
                                           sendlocal, stageId, volume, muted, x, y, z, rX, rY, rZ)

        struct custom_stage_member_t {
            std::string _id;
            std::string userId;
            std::string stageMemberId;
            std::string stageId;
            double volume;
            bool muted;
            double x;
            double y;
            double z;
            double rX;
            double rY;
            double rZ;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(custom_stage_member_t, _id, userId, stageMemberId, stageId, volume, muted, x,
                                           y, z, rX, rY,
                                           rZ)

        struct soundcard_t {
            std::string _id;
            std::string deviceId;
            std::string name;
            bool isDefault;
            std::string driver; //'JACK' | 'ALSA' | 'ASIO' | 'WEBRTC'
            int sampleRate;
            std::vector<int> sampleRates;
            double periodSize;
            int numPeriods;
            double softwareLatency;

            int numInputChannels;
            int numOutputChannels;

            std::vector<int> inputChannels;
            std::vector<int> outputChannels;

            std::string userId;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(soundcard_t, _id, deviceId, name, isDefault, driver, sampleRate, sampleRates,
                                           periodSize, numPeriods, softwareLatency, numInputChannels, numOutputChannels,
                                           inputChannels, outputChannels, userId)

        struct ov_track_t {
            std::string _id;
            std::string soundCardId;
            int channel;
            std::string userId;
            std::string deviceId;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ov_track_t, _id, soundCardId, channel, userId, deviceId)

        struct remote_ov_track_t {
            std::string _id;
            std::string ovTrackId;
            std::string stageMemberId;
            int channel;
            bool online;
            std::string directivity;   // Will be omni or cardoid
            std::string userId;
            std::string stageId;
            double volume;
            bool muted;
            double x;
            double y;
            double z;
            double rX;
            double rY;
            double rZ;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(remote_ov_track_t, _id, ovTrackId, stageMemberId, channel, online,
                                           directivity, userId, stageId, volume, muted, x, y, z, rX, rY, rZ)

        struct custom_remote_ov_track_t {
            std::string _id;
            std::string stageMemberId;
            std::string remoteOvTrackId;
            std::string directivity;   // Will be omni or cardoid
            std::string userId;
            std::string stageId;
            double volume;
            bool muted;
            double x;
            double y;
            double z;
            double rX;
            double rY;
            double rZ;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(custom_remote_ov_track_t, _id, stageMemberId, remoteOvTrackId, directivity,
                                           userId, stageId, volume, muted, x, y, z, rX, rY, rZ)

        struct user_t {
            std::string _id;
            std::string name;
            std::string avatarUrl;
            std::string stageId;
            std::string stageMemberId;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(user_t, _id, name, avatarUrl, stageId, stageMemberId)
    } // namespace js
} // namespace ds

#endif