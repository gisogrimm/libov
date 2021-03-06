#ifndef DS_TYPES
#define DS_TYPES

#include "ov_types.h"
#include <boost/optional.hpp>
#include <iostream>
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

    __unused inline void to_json(nlohmann::json& j, const device_t& p)
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
                         {"soundCardNames", p.soundCardNames}};
      if(!p.soundCardName.empty()) {
        j["soundCardName"] = p.soundCardName;
      }
    }

    __unused inline void from_json(const nlohmann::json& j, device_t& p)
    {
      j.at("_id").get_to(p._id);
      j.at("_id").get_to(p._id);
      j.at("userId").get_to(p.userId);
      j.at("online").get_to(p.online);
      j.at("mac").get_to(p.mac);
      j.at("name").get_to(p.name);
      j.at("canVideo").get_to(p.canVideo);
      j.at("canAudio").get_to(p.canAudio);
      j.at("canOv").get_to(p.canOv);
      j.at("sendVideo").get_to(p.sendVideo);
      j.at("sendAudio").get_to(p.sendAudio);
      j.at("receiveVideo").get_to(p.receiveVideo);
      j.at("receiveAudio").get_to(p.receiveAudio);
      j.at("receiverType").get_to(p.receiverType);
      j.at("senderJitter").get_to(p.senderJitter);
      j.at("p2p").get_to(p.p2p);
      j.at("renderReverb").get_to(p.renderReverb);
      j.at("reverbGain").get_to(p.reverbGain);
      j.at("renderISM").get_to(p.renderISM);
      j.at("rawMode").get_to(p.rawMode);
      j.at("egoGain").get_to(p.egoGain);
      j.at("soundCardNames").get_to(p.soundCardNames);
      // if we also allow "null" values, then we need to add an "is_string()"
      if(j.count("soundCardName") != 0) {
        j.at("soundCardName").get_to(p.soundCardName);
      } else {
        p.soundCardName = "";
      }
    }

    struct stage_ov_server_t {
      bool supported;
      std::string router;
      std::string ipv4;
      std::string ipv6;
      uint16_t port;
      uint32_t pin;
      double serverJitter;
    };

    __unused inline void to_json(nlohmann::json& j, const stage_ov_server_t& p)
    {
      j = nlohmann::json{{"router", p.router},
                         {"ipv4", p.ipv4},
                         {"port", p.port},
                         {"pin", p.pin}};
      if(!p.ipv6.empty()) {
        j["ipv6"] = p.ipv6;
      }
      if(p.serverJitter != -1) {
        j["serverJitter"] = p.serverJitter;
      }
    }
    __unused inline void from_json(const nlohmann::json& j,
                                   stage_ov_server_t& p)
    {
      p.supported = true;
      j.at("router").get_to(p.router);
      j.at("ipv4").get_to(p.ipv4);
      j.at("port").get_to(p.port);
      j.at("pin").get_to(p.pin);
      j.at("router").get_to(p.router);
      if(j.count("ipv6") != 0) {
        j.at("ipv6").get_to(p.ipv6);
      } else {
        p.ipv6 = "";
      }
      if(j.count("serverJitter") != 0) {
        j.at("serverJitter").get_to(p.serverJitter);
      } else {
        p.serverJitter = -1;
      }
    }

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

    __unused inline void to_json(nlohmann::json& j, const stage_t& p)
    {
      j = nlohmann::json{{"_id", p._id},
                         {"name", p.name},
                         {"admins", p.admins},
                         {"password", p.password},
                         {"width", p.width},
                         {"length", p.length},
                         {"height", p.height},
                         {"absorption", p.absorption},
                         {"damping", p.damping},
                         {"renderAmbient", p.renderAmbient},
                         {"ambientLevel", p.ambientLevel}};
      if(p.ovServer.supported) {
        j["ovServer"] = p.ovServer;
      }
      if(!p.ambientSoundUrl.empty()) {
        j["ambientSoundUrl"] = p.ambientSoundUrl;
      }
    }

    __unused inline void from_json(const nlohmann::json& j, stage_t& p)
    {
      j.at("_id").get_to(p._id);
      j.at("name").get_to(p.name);
      j.at("admins").get_to(p.admins);
      j.at("password").get_to(p.password);
      j.at("width").get_to(p.width);
      j.at("length").get_to(p.length);
      j.at("height").get_to(p.height);
      j.at("absorption").get_to(p.absorption);
      j.at("damping").get_to(p.damping);
      j.at("ambientLevel").get_to(p.ambientLevel);
      if(j.contains("renderAmbient")) {
        j.at("renderAmbient").get_to(p.renderAmbient);
      } else {
        p.renderAmbient = false;
      }
      if(j.contains("ambientSoundUrl")) {
        j.at("ambientSoundUrl").get_to(p.ambientSoundUrl);
      } else {
        p.ambientSoundUrl = "";
      }
      if(j.contains("ovServer")) {
        j.at("ovServer").get_to(p.ovServer);
      } else {
        p.ovServer.supported = false;
      }
    }

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

    __unused inline void to_json(nlohmann::json& j, const group_t& p)
    {
      j = nlohmann::json{{"_id", p._id},       {"name", p.name},
                         {"color", p.color},   {"stageId", p.stageId},
                         {"volume", p.volume}, {"muted", p.muted},
                         {"x", p.x},           {"y", p.y},
                         {"z", p.z},           {"rX", p.rX},
                         {"rY", p.rY},         {"rZ", p.rZ}};
    }

    __unused inline void from_json(const nlohmann::json& j, group_t& p)
    {
      j.at("_id").get_to(p._id);
      j.at("name").get_to(p.name);
      j.at("stageId").get_to(p.stageId);
      j.at("volume").get_to(p.volume);
      j.at("muted").get_to(p.muted);
      j.at("x").get_to(p.x);
      j.at("y").get_to(p.y);
      j.at("z").get_to(p.z);
      j.at("rX").get_to(p.rX);
      j.at("rY").get_to(p.rY);
      j.at("rZ").get_to(p.rZ);
      if(j.contains("color")) {
        j.at("color").get_to(p.color);
      } else {
        p.color = "";
      }
    }

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

    __unused inline void to_json(nlohmann::json& j, const custom_group_t& p)
    {
      j = nlohmann::json{{"_id", p._id},
                         {"userId", p.userId},
                         {"groupId", p.groupId},
                         {"stageId", p.stageId},
                         {"volume", p.volume},
                         {"muted", p.muted},
                         {"x", p.x},
                         {"y", p.y},
                         {"z", p.z},
                         {"rX", p.rX},
                         {"rY", p.rY},
                         {"rZ", p.rZ}};
    }

    __unused inline void from_json(const nlohmann::json& j, custom_group_t& p)
    {
      j.at("_id").get_to(p._id);
      j.at("userId").get_to(p.userId);
      j.at("groupId").get_to(p.groupId);
      j.at("stageId").get_to(p.stageId);
      j.at("volume").get_to(p.volume);
      j.at("muted").get_to(p.muted);
      j.at("x").get_to(p.x);
      j.at("y").get_to(p.y);
      j.at("z").get_to(p.z);
      j.at("rX").get_to(p.rX);
      j.at("rY").get_to(p.rY);
      j.at("rZ").get_to(p.rZ);
    }

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

    __unused inline void to_json(nlohmann::json& j, const stage_member_t& p)
    {
      j = nlohmann::json{{"_id", p._id},
                         {"groupId", p.groupId},
                         {"userId", p.userId},
                         {"online", p.online},
                         {"isDirector", p.isDirector},
                         {"ovStageDeviceId", p.ovStageDeviceId},
                         {"sendlocal", p.sendlocal},
                         {"stageId", p.stageId},
                         {"volume", p.volume},
                         {"muted", p.muted},
                         {"x", p.x},
                         {"y", p.y},
                         {"z", p.z},
                         {"rX", p.rX},
                         {"rY", p.rY},
                         {"rZ", p.rZ}};
    }

    __unused inline void from_json(const nlohmann::json& j, stage_member_t& p)
    {
      j.at("_id").get_to(p._id);
      j.at("groupId").get_to(p.groupId);
      j.at("userId").get_to(p.userId);
      j.at("online").get_to(p.online);
      j.at("isDirector").get_to(p.isDirector);
      j.at("ovStageDeviceId").get_to(p.ovStageDeviceId);
      j.at("sendlocal").get_to(p.sendlocal);
      j.at("stageId").get_to(p.stageId);
      j.at("volume").get_to(p.volume);
      j.at("muted").get_to(p.muted);
      j.at("x").get_to(p.x);
      j.at("y").get_to(p.y);
      j.at("z").get_to(p.z);
      j.at("rX").get_to(p.rX);
      j.at("rY").get_to(p.rY);
      j.at("rZ").get_to(p.rZ);
    }

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

    __unused inline void to_json(nlohmann::json& j,
                                 const custom_stage_member_t& p)
    {
      j = nlohmann::json{{"_id", p._id},
                         {"userId", p.userId},
                         {"stageMemberId", p.stageMemberId},
                         {"stageId", p.stageId},
                         {"volume", p.volume},
                         {"muted", p.muted},
                         {"x", p.x},
                         {"y", p.y},
                         {"z", p.z},
                         {"rX", p.rX},
                         {"rY", p.rY},
                         {"rZ", p.rZ}};
    }

    __unused inline void from_json(const nlohmann::json& j,
                                   custom_stage_member_t& p)
    {
      j.at("_id").get_to(p._id);
      j.at("userId").get_to(p.userId);
      j.at("stageMemberId").get_to(p.stageMemberId);
      j.at("stageId").get_to(p.stageId);
      j.at("volume").get_to(p.volume);
      j.at("muted").get_to(p.muted);
      j.at("x").get_to(p.x);
      j.at("y").get_to(p.y);
      j.at("z").get_to(p.z);
      j.at("rX").get_to(p.rX);
      j.at("rY").get_to(p.rY);
      j.at("rZ").get_to(p.rZ);
    }

    struct soundcard_t {
      std::string _id;
      std::string deviceId;
      std::string name;
      std::string label;
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

    __unused inline void to_json(nlohmann::json& j, const soundcard_t& p)
    {
      j = nlohmann::json{{"_id", p._id},
                         {"deviceId", p.deviceId},
                         {"name", p.name},
                         {"label", p.label},
                         {"isDefault", p.isDefault},
                         {"driver", p.driver},
                         {"sampleRate", p.sampleRate},
                         {"sampleRates", p.sampleRates},
                         {"periodSize", p.periodSize},
                         {"numPeriods", p.numPeriods},
                         {"numInputChannels", p.numInputChannels},
                         {"numOutputChannels", p.numOutputChannels},
                         {"inputChannels", p.inputChannels},
                         {"outputChannels", p.outputChannels},
                         {"userId", p.userId}};
      if(p.softwareLatency != -1) {
        j["softwareLatency"] = p.softwareLatency;
      }
    }

    __unused inline void from_json(const nlohmann::json& j, soundcard_t& p)
    {
      j.at("_id").get_to(p._id);
      j.at("deviceId").get_to(p.deviceId);
      j.at("name").get_to(p.name);
      j.at("label").get_to(p.label);
      j.at("isDefault").get_to(p.isDefault);
      j.at("driver").get_to(p.driver);
      j.at("sampleRate").get_to(p.sampleRate);
      j.at("sampleRates").get_to(p.sampleRates);
      j.at("periodSize").get_to(p.periodSize);
      j.at("numPeriods").get_to(p.numPeriods);
      j.at("numInputChannels").get_to(p.numInputChannels);
      j.at("numOutputChannels").get_to(p.numOutputChannels);
      j.at("inputChannels").get_to(p.inputChannels);
      j.at("outputChannels").get_to(p.outputChannels);
      j.at("userId").get_to(p.userId);

      if(j.contains("softwareLatency")) {
        j.at("softwareLatency").get_to(p.softwareLatency);
      } else {
        p.softwareLatency = -1;
      }
    }

    struct ov_track_t {
      std::string _id;
      std::string soundCardId;
      int channel;
      std::string userId;
      std::string deviceId;
    };

    __unused inline void to_json(nlohmann::json& j, const ov_track_t& p)
    {
      j = nlohmann::json{{"_id", p._id},
                         {"soundCardId", p.soundCardId},
                         {"channel", p.channel},
                         {"userId", p.userId},
                         {"deviceId", p.deviceId}};
    }

    __unused inline void from_json(const nlohmann::json& j, ov_track_t& p)
    {
      j.at("_id").get_to(p._id);
      j.at("soundCardId").get_to(p.deviceId);
      j.at("channel").get_to(p.channel);
      j.at("userId").get_to(p.userId);
      j.at("deviceId").get_to(p.deviceId);
    }

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

    __unused inline void to_json(nlohmann::json& j, const remote_ov_track_t& p)
    {
      j = nlohmann::json{{"_id", p._id},
                         {"ovTrackId", p.ovTrackId},
                         {"stageMemberId", p.stageMemberId},
                         {"channel", p.channel},
                         {"online", p.online},
                         {"directivity", p.directivity},
                         {"userId", p.userId},
                         {"stageId", p.stageId},
                         {"volume", p.volume},
                         {"muted", p.muted},
                         {"x", p.x},
                         {"y", p.y},
                         {"z", p.z},
                         {"rX", p.rX},
                         {"rY", p.rY},
                         {"rZ", p.rZ}};
    }

    __unused inline void from_json(const nlohmann::json& j,
                                   remote_ov_track_t& p)
    {
      j.at("_id").get_to(p._id);
      j.at("ovTrackId").get_to(p.ovTrackId);
      j.at("stageMemberId").get_to(p.stageMemberId);
      j.at("channel").get_to(p.channel);
      j.at("online").get_to(p.online);
      j.at("directivity").get_to(p.directivity);
      j.at("userId").get_to(p.userId);
      j.at("stageId").get_to(p.stageId);
      j.at("volume").get_to(p.volume);
      j.at("muted").get_to(p.muted);
      j.at("x").get_to(p.x);
      j.at("y").get_to(p.y);
      j.at("z").get_to(p.z);
      j.at("rX").get_to(p.rX);
      j.at("rY").get_to(p.rY);
      j.at("rZ").get_to(p.rZ);
    }

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

    __unused inline void to_json(nlohmann::json& j,
                                 const custom_remote_ov_track_t& p)
    {
      j = nlohmann::json{{"_id", p._id},
                         {"stageMemberId", p.stageMemberId},
                         {"remoteOvTrackId", p.remoteOvTrackId},
                         {"directivity", p.directivity},
                         {"userId", p.userId},
                         {"stageId", p.stageId},
                         {"volume", p.volume},
                         {"muted", p.muted},
                         {"x", p.x},
                         {"y", p.y},
                         {"z", p.z},
                         {"rX", p.rX},
                         {"rY", p.rY},
                         {"rZ", p.rZ}};
    }

    __unused inline void from_json(const nlohmann::json& j,
                                   custom_remote_ov_track_t& p)
    {
      j.at("_id").get_to(p._id);
      j.at("stageMemberId").get_to(p.stageMemberId);
      j.at("remoteOvTrackId").get_to(p.remoteOvTrackId);
      j.at("directivity").get_to(p.directivity);
      j.at("userId").get_to(p.userId);
      j.at("stageId").get_to(p.stageId);
      j.at("volume").get_to(p.volume);
      j.at("muted").get_to(p.muted);
      j.at("x").get_to(p.x);
      j.at("y").get_to(p.y);
      j.at("z").get_to(p.z);
      j.at("rX").get_to(p.rX);
      j.at("rY").get_to(p.rY);
      j.at("rZ").get_to(p.rZ);
    }

    struct user_t {
      std::string _id;
      std::string name;
      std::string avatarUrl;
      std::string stageId;
      std::string stageMemberId;
    };

    __unused inline void to_json(nlohmann::json& j, const user_t& p)
    {
      j = nlohmann::json{{"_id", p._id}, {"name", p.name}};
      if(!p.avatarUrl.empty()) {
        j["avatarUrl"] = p.avatarUrl;
      }
      if(!p.stageId.empty()) {
        j["stageId"] = p.stageId;
      }
      if(!p.stageMemberId.empty()) {
        j["stageMemberId"] = p.stageMemberId;
      }
    }

    __unused inline void from_json(const nlohmann::json& j, user_t& p)
    {
      j.at("_id").get_to(p._id);
      j.at("name").get_to(p.name);
      if(j.count("avatarUrl") != 0) {
        j.at("avatarUrl").get_to(p.avatarUrl);
      } else {
        p.avatarUrl = "";
      }
      if(j.count("stageId") != 0) {
        j.at("stageId").get_to(p.stageId);
      } else {
        p.stageId = "";
      }
      if(j.count("stageMemberId") != 0) {
        j.at("stageMemberId").get_to(p.stageMemberId);
      } else {
        p.stageMemberId = "";
      }
    }

  } // namespace json
} // namespace ds

#endif