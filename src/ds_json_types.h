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

inline void to_json(nlohmann::json &j, const device_t &p) {
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
  if (!p.soundCardName.empty()) {
    j["soundCardName"] = p.soundCardName;
  }
}

inline void from_json(const nlohmann::json &j, device_t &p) {
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
  if (j.count("soundCardName") != 0) {
    j.at("soundCardName").get_to(p.soundCardName);
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

inline void to_json(nlohmann::json &j, const stage_ov_server_t &p) {
  j = nlohmann::json{{"router", p.router},
                     {"ipv4", p.ipv4},
                     {"port", p.port},
                     {"pin", p.pin}};
  if (!p.ipv6.empty()) {
    j["ipv6"] = p.ipv6;
  }
  if (p.serverJitter != -1) {
    j["serverJitter"] = p.serverJitter;
  }
}
inline void from_json(const nlohmann::json &j, stage_ov_server_t &p) {
  j.at("router").get_to(p.router);
  j.at("ipv4").get_to(p.ipv4);
  j.at("port").get_to(p.port);
  j.at("pin").get_to(p.pin);
  j.at("router").get_to(p.router);
  if (j.count("ipv6") != 0) {
    j.at("ipv6").get_to(p.ipv6);
  } else {
    p.ipv6 = "";
  }
  if (j.count("serverJitter") != 0) {
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

inline void to_json(nlohmann::json &j, const stage_t &p) {
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
                     {"ambientLevel", p.ambientLevel},
                     {"ovServer", p.ovServer}};
  if (!p.ambientSoundUrl.empty()) {
    j["ambientSoundUrl"] = p.ambientSoundUrl;
  }
}

inline void from_json(const nlohmann::json &j, stage_t &p) {
  j.at("_id").get_to(p._id);
  j.at("name").get_to(p.name);
  j.at("admins").get_to(p.admins);
  j.at("password").get_to(p.password);
  j.at("width").get_to(p.width);
  j.at("length").get_to(p.length);
  j.at("height").get_to(p.height);
  j.at("absorption").get_to(p.absorption);
  j.at("damping").get_to(p.damping);
  j.at("renderAmbient").get_to(p.renderAmbient);
  j.at("ambientLevel").get_to(p.ambientLevel);
  j.at("ovServer").get_to(p.ovServer);
  if (j.count("ambientSoundUrl") != 0) {
    j.at("ambientSoundUrl").get_to(p.ambientSoundUrl);
  } else {
    p.ambientSoundUrl = "";
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

inline void to_json(nlohmann::json &j, const user_t &p) {
  j = nlohmann::json{{"_id", p._id},
                     {"name", p.name}};
  if (!p.avatarUrl.empty()) {
    j["avatarUrl"] = p.avatarUrl;
  }
  if (!p.stageId.empty()) {
    j["stageId"] = p.stageId;
  }
  if (!p.stageMemberId.empty()) {
    j["stageMemberId"] = p.stageMemberId;
  }
}

inline void from_json(const nlohmann::json &j, user_t &p) {
  j.at("_id").get_to(p._id);
  j.at("name").get_to(p.name);
  if (j.count("avatarUrl") != 0) {
    j.at("avatarUrl").get_to(p.avatarUrl);
  } else {
    p.avatarUrl = "";
  }
  if (j.count("stageId") != 0) {
    j.at("stageId").get_to(p.stageId);
  } else {
    p.stageId = "";
  }
  if (j.count("stageMemberId") != 0) {
    j.at("stageMemberId").get_to(p.stageMemberId);
  } else {
    p.stageMemberId = "";
  }
}

} // namespace json
} // namespace ds

#endif