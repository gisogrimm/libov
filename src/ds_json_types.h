#ifndef DS_TYPES
#define DS_TYPES

#include "ov_types.h"
#include <boost/optional.hpp>
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

      std::string receiverType; // Either ortf or hrtf

      double senderJitter;
      double receiverJitter;

      bool p2p;

      bool renderReverb;
      double reverbGain;
      bool renderISM;
      bool rawMode;
      double egoGain;

      std::string soundCardName;
      std::vector<std::string> soundCardNames;
    };

    void to_json(nlohmann::json& j, const device_t& p)
    {
      j = nlohmann::json{{"_id", p._id},
                         {"userId", p.userId},
                         {"online", p.online},
                         {"mac", p.mac},
                         {"name", p.name},
                         {"canVideo", p.canVideo},
                         {"canAudio", p.canAudio},
                         {"canOv", p.canOv},
                         {"sendVideo", p.sendVideo},
                         {"sendAudio", p.sendAudio},
                         {"receiveVideo", p.receiveVideo},
                         {"receiveAudio", p.receiveAudio},
                         {"receiverType", p.receiverType},
                         {"senderJitter", p.senderJitter},
                         {"receiverJitter", p.receiverJitter},
                         {"p2p", p.p2p},
                         {"renderReverb", p.renderReverb},
                         {"reverbGain", p.reverbGain},
                         {"renderISM", p.renderISM},
                         {"rawMode", p.rawMode},
                         {"egoGain", p.egoGain},
                         {"soundCardNames", p.soundCardNames},
                         {"soundCardName", p.soundCardName}};
    }

    void from_json(const nlohmann::json& j, device_t& p)
    {
      p._id = j.at("_id").get<std::string>();
      p.userId = j.at("userId").get<std::string>();
      p.online = j.at("online").get<bool>();
      p.mac = j.at("mac").get<std::string>();
      p.name = j.at("name").get<std::string>();
      p.canVideo = j.at("canVideo").get<bool>();
      p.canAudio = j.at("canAudio").get<bool>();
      p.canOv = j.at("canOv").get<bool>();
      p.sendVideo = j.at("sendVideo").get<bool>();
      p.sendAudio = j.at("sendAudio").get<bool>();
      p.receiveVideo = j.at("receiveVideo").get<bool>();
      p.receiveAudio = j.at("receiveAudio").get<bool>();
      p.receiverType = j.at("receiverType").get<std::string>();
      p.senderJitter = j.at("senderJitter").get<double>();
      p.p2p = j.at("p2p").get<bool>();
      p.renderReverb = j.at("renderReverb").get<bool>();
      p.reverbGain = j.at("reverbGain").get<double>();
      p.renderISM = j.at("renderISM").get<bool>();
      p.rawMode = j.at("rawMode").get<bool>();
      p.egoGain = j.at("egoGain").get<double>();
      p.soundCardNames = j.at("soundCardNames").get<std::vector<std::string>>();

      // if we also allow "null" values, then we need to add an "is_string()"
      if(j.count("soundCardName") != 0) {
        p.soundCardName = j.at("soundCardName").get<std::string>();
      } else {
        p.soundCardName = "";
      }
    }

    struct stage_ov_server_t {
      std::string router;
      std::string ipv4;
      std::string ipv6;
      uint16_t port;
      uint32_t pin;
      double serverJitter;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(stage_ov_server_t, router, ipv4, ipv6,
                                       port, pin, serverJitter)

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

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(stage_t, _id, name, width, length,
                                       height, absorption, damping,
                                       renderAmbient, ambientSoundUrl,
                                       ambientLevel)

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

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(group_t, _id, name, color, stageId,
                                       volume, muted, x, y, z, rX, rY, rZ)

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

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(custom_group_t, _id, userId, groupId,
                                       stageId, volume, muted, x, y, z, rX, rY,
                                       rZ)

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

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(stage_member_t, _id, groupId, userId,
                                       online, isDirector, ovStageDeviceId,
                                       sendlocal, stageId, volume, muted, x, y,
                                       z, rX, rY, rZ)

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

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(custom_stage_member_t, _id, userId,
                                       stageMemberId, stageId, volume, muted, x,
                                       y, z, rX, rY, rZ)

    struct soundcard_t {
      std::string _id;
      std::string deviceId;
      std::string name;
      bool isDefault;
      std::string driver; //'JACK' | 'ALSA' | 'ASIO' | 'WEBRTC'
      double sampleRate;
      std::vector<double> sampleRates;
      unsigned int periodSize;
      unsigned int numPeriods;
      double softwareLatency;

      unsigned int numInputChannels;
      unsigned int numOutputChannels;

      std::vector<unsigned int> inputChannels;
      std::vector<unsigned int> outputChannels;

      std::string userId;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(soundcard_t, _id, deviceId, name,
                                       isDefault, driver, sampleRate,
                                       sampleRates, periodSize, numPeriods,
                                       softwareLatency, numInputChannels,
                                       numOutputChannels, inputChannels,
                                       outputChannels, userId)

    struct ov_track_t {
      std::string _id;
      std::string soundCardId;
      int channel;
      std::string userId;
      std::string deviceId;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ov_track_t, _id, soundCardId, channel,
                                       userId, deviceId)

    struct remote_ov_track_t {
      std::string _id;
      std::string ovTrackId;
      std::string stageMemberId;
      int channel;
      bool online;
      std::string directivity; // Will be omni or cardoid
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

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(remote_ov_track_t, _id, ovTrackId,
                                       stageMemberId, channel, online,
                                       directivity, userId, stageId, volume,
                                       muted, x, y, z, rX, rY, rZ)

    struct custom_remote_ov_track_t {
      std::string _id;
      std::string stageMemberId;
      std::string remoteOvTrackId;
      std::string directivity; // Will be omni or cardoid
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

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(custom_remote_ov_track_t, _id,
                                       stageMemberId, remoteOvTrackId,
                                       directivity, userId, stageId, volume,
                                       muted, x, y, z, rX, rY, rZ)

    struct user_t {
      std::string _id;
      std::string name;
      std::string avatarUrl;
      std::string stageId;
      std::string stageMemberId;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(user_t, _id, name, avatarUrl, stageId,
                                       stageMemberId)
  } // namespace json
} // namespace ds

#endif